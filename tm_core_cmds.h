#ifndef TM_CORE_CMDS_H
#define TM_CORE_CMDS_H

#define JIM_EMBEDDED
#include <jim.h>

#include "tmake.h"

void tm_RegisterCoreCommands(Jim_Interp *interp);

#endif

