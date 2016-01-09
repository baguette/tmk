#ifndef TM_CORE_CMDS_H
#define TM_CORE_CMDS_H

#define JIM_EMBEDDED
#include <jim.h>

#define TM_INCLUDE_PATH "TM_INCLUDE_PATH"

void tm_RegisterCoreCommands(Jim_Interp *interp);

#endif

