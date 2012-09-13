/* exelibstamp.pre will be copied and modified to exelibstamp.c when you run make.
 * make will modify REPLACE_ME_WITH_MAJOR_VERSION with Makefile.in FF_VERSION,
 * and modify REPLACE_ME_WITH_MINOR_VERSION with Makefile.in FF_REVISION
 *
 * If you need to modify this file, edit the .pre version since the .c version will
 * be overwritten during the next make process.
 */

#include "fontforge.h"
#include "baseviews.h"
#include "libffstamp.h"			/* FontForge version# date time stamps */

struct library_version_configuration exe_library_version_configuration = {
    REPLACE_ME_WITH_MAJOR_VERSION,
    REPLACE_ME_WITH_MINOR_VERSION,
    LibFF_ModTime,			/* Seconds since 1970 (standard unix time) */
    LibFF_ModTime_Str,			/* Version date (in char string format)    */
    LibFF_VersionDate,			/* Version as long value, Year, month, day */
    sizeof(struct library_version_configuration),
    sizeof(struct splinefont),
    sizeof(struct splinechar),
    sizeof(struct fontviewbase),
    sizeof(struct charviewbase),
    sizeof(struct cvcontainer),
    1,
    1,

#ifdef _NO_PYTHON
    0,
#else
    1,
#endif
    0xff		/* Not currently defined */
};
