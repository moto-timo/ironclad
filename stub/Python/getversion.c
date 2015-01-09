
/* Return the full version string. */

#include "Python.h"

#include "patchlevel.h"

const char *
Py_GetVersion(void)
{
    /* match cpython */
    static char version[] = "2.7.8 (default, Jun 30 2014, 16:03:49) [MSC v.1500 32 bit (Intel)]";
    return version;
}
