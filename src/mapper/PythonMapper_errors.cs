using System;
using System.Runtime.InteropServices;

using Microsoft.Scripting.Runtime;

using IronPython.Hosting;
using IronPython.Modules;
using IronPython.Runtime;
using IronPython.Runtime.Exceptions;
using IronPython.Runtime.Operations;
using IronPython.Runtime.Types;

using Ironclad.Structs;

namespace Ironclad
{
    public partial class PythonMapper : PythonApi
    {
        internal void
        PrintToStdErr(object obj)
        {
            object stderr = this.python.SystemState.Get__dict__()["stderr"];
            PythonOps.PrintWithDest(this.scratchContext, stderr, obj);
        }


        public override void
        PyErr_Print()
        {
            if (this.LastException == null)
            {
                throw new Exception("Fatal error: called PyErr_Print without an actual error to print.");
            }
            this.PrintToStdErr(this.LastException);
            this.LastException = null;
        }

        public override int
        PyErr_WarnEx(IntPtr arg0, string arg1, int arg2)
        {
            // int PyErr_WarnEx(PyObject *category, char *message, int stacklevel)�
            this.PrintToStdErr(arg1);
            return 0;
        }

        private IntPtr
        StoreTyped(PythonExceptions.BaseException exc)
        {
            IntPtr ptr = this.allocator.Alloc((uint)Marshal.SizeOf(typeof(PyObject)));
            CPyMarshal.WriteIntField(ptr, typeof(PyObject), "ob_refcnt", 1);
            object type_ = PythonCalls.Call(Builtin.type, new object[] { exc });
            CPyMarshal.WritePtrField(ptr, typeof(PyObject), "ob_type", this.Store(type_));
            this.map.Associate(ptr, exc);
            return ptr;
        }
    }
}
