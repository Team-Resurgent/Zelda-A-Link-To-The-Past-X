/* stdbool.h shim for Xbox XDK - includes our CRT shim first */
#ifndef STDBOOL_H_SHIM
#define STDBOOL_H_SHIM
#include "xdk_crt_shim.h"
#ifndef __cplusplus
#ifndef bool
#define bool  _Bool
#define true  1
#define false 0
#endif
#endif
#endif
