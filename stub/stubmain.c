

#include "Python.h"

// not included by Python.h, but contain useful declarations/definitions
#include "Python-ast.h"
#include "symtable.h"
#include "structmember.h"
#include "frameobject.h"
#include "pygetopt.h"
#include "pythread.h"
#include "abstract.h"
#include "token.h"
#include "osdefs.h"

// prototypes for managed functions which could be called from C code
#include "_extra_functions.generated.h"

// definitions for missing data
#include "ironclad-data.c"

#define DLLEXPORT __declspec(dllexport)

#pragma clang attribute push (DLLEXPORT, apply_to=function)

// alternative C implementations of various functions
#include "ironclad-functions.c"

// init function
#include "stubinit.generated.c"

#pragma clang attribute pop

// miscellaneous holes filled
#ifdef _MSC_VER
#include <windows.h>

// _fltused (floating-point used) undefined (Clang/msvcr100 issue only?)
#include <stdint.h>
int32_t _fltused = 0;

// __acrt_iob_func used by MSVCRT 14.0+ headers but not in 10.0
#if _MSC_VER < 1900
extern FILE* __iob_func(void);
FILE* __cdecl __acrt_iob_func(unsigned fd)
{
    return &__iob_func()[fd];
}
#endif

// and this lot is copied (with tiny changes) from PC/nt_dl.c, to enable exactly what the following comment describes
// In CPython 3.4 it has been conditionally disabled (for __MSC_VER >= 1600, see PC/pyconfig.h)
// and removed completely in 3.9 (https://github.com/python/cpython/issues/83734)
// Disabling the dangling else warning (rather than fixing it) to keep the code the same as in CPython
#pragma clang diagnostic ignored "-Wdangling-else"

// Windows "Activation Context" work:
// Our .pyd extension modules are generally built without a manifest (ie,
// those included with Python and those built with a default distutils.
// This requires we perform some "activation context" magic when loading our
// extensions.  In summary:
// * As our DLL loads we save the context being used.
// * Before loading our extensions we re-activate our saved context.
// * After extension load is complete we restore the old context.
// As an added complication, this magic only works on XP or later - we simply
// use the existence (or not) of the relevant function pointers from kernel32.
// See bug 4566 (http://python.org/sf/4566) for more details.
// In Visual Studio 2010, side by side assemblies are no longer used.

typedef BOOL (WINAPI * PFN_GETCURRENTACTCTX)(HANDLE *);
typedef BOOL (WINAPI * PFN_ACTIVATEACTCTX)(HANDLE, ULONG_PTR *);
typedef BOOL (WINAPI * PFN_DEACTIVATEACTCTX)(DWORD, ULONG_PTR);
typedef void (WINAPI * PFN_ADDREFACTCTX)(HANDLE);
typedef void (WINAPI * PFN_RELEASEACTCTX)(HANDLE);

// locals and function pointers for this activation context magic.
static HANDLE PyWin_DLLhActivationContext = NULL; // one day it might be public
static PFN_GETCURRENTACTCTX pfnGetCurrentActCtx = NULL;
static PFN_ACTIVATEACTCTX pfnActivateActCtx = NULL;
static PFN_DEACTIVATEACTCTX pfnDeactivateActCtx = NULL;
static PFN_ADDREFACTCTX pfnAddRefActCtx = NULL;
static PFN_RELEASEACTCTX pfnReleaseActCtx = NULL;

void _LoadActCtxPointers()
{
	HINSTANCE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	if (hKernel32)
		pfnGetCurrentActCtx = (PFN_GETCURRENTACTCTX) GetProcAddress(hKernel32, "GetCurrentActCtx");
	// If we can't load GetCurrentActCtx (ie, pre XP) , don't bother with the rest.
	if (pfnGetCurrentActCtx) {
		pfnActivateActCtx = (PFN_ACTIVATEACTCTX) GetProcAddress(hKernel32, "ActivateActCtx");
		pfnDeactivateActCtx = (PFN_DEACTIVATEACTCTX) GetProcAddress(hKernel32, "DeactivateActCtx");
		pfnAddRefActCtx = (PFN_ADDREFACTCTX) GetProcAddress(hKernel32, "AddRefActCtx");
		pfnReleaseActCtx = (PFN_RELEASEACTCTX) GetProcAddress(hKernel32, "ReleaseActCtx");
	}
}

DLLEXPORT
ULONG_PTR _Py_ActivateActCtx()
{
	ULONG_PTR ret = 0;
	if (PyWin_DLLhActivationContext && pfnActivateActCtx)
		if (!(*pfnActivateActCtx)(PyWin_DLLhActivationContext, &ret)) {
			printf("Python failed to activate the activation context before loading a DLL\n");
			ret = 0; // no promise the failing function didn't change it!
		}
	return ret;
}

DLLEXPORT
void _Py_DeactivateActCtx(ULONG_PTR cookie)
{
	if (cookie && pfnDeactivateActCtx)
		if (!(*pfnDeactivateActCtx)(0, cookie))
			printf("Python failed to de-activate the activation context\n");
}

BOOL	WINAPI	DllMain (HINSTANCE hInst,
						DWORD dw_reason_for_call,
						LPVOID lpReserved)
{
	switch (dw_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// capture our activation context for use when loading extensions.
			_LoadActCtxPointers();
			if (pfnGetCurrentActCtx && pfnAddRefActCtx)
				if ((*pfnGetCurrentActCtx)(&PyWin_DLLhActivationContext)) {
					(*pfnAddRefActCtx)(PyWin_DLLhActivationContext);
				}
				else {
					printf("Python failed to load the default activation context\n");
					return FALSE;
				}
			break;

		case DLL_PROCESS_DETACH:
			if (pfnReleaseActCtx)
				(*pfnReleaseActCtx)(PyWin_DLLhActivationContext);
			break;
	}
	return TRUE;
}
#endif  // _MSC_VER
