
#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"
#include "tm_core_cmds.h"

static int ruleCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	tm_rule *rule = NULL;
	target_list *deps = NULL;
	char *target = NULL;
	char *recipe = NULL;
	int i, numdeps;

	if (argc < 3 || argc > 4) {
		Jim_WrongNumArgs(interp, 2, argv, "rule target-list dep-list ?script?");
		return (JIM_ERR);
	}
	
	target = target_copy(Jim_String(argv[1]));
	
	if (argc == 4) {
		recipe = target_copy(Jim_String(argv[3]));
	}

	numdeps = Jim_ListLength(interp, argv[2]);
	for (i = 0; i < numdeps; i++) {
		Jim_Obj *dep = Jim_ListGetIndex(interp, argv[2], i);
		char *sdep = target_copy(Jim_String(dep));

		deps = target_cons(sdep, deps);
	}

	rule = new_rule(target, deps, recipe);
	tm_rules = rule_cons(rule, tm_rules);

	if (tm_goal == NULL) {
		tm_goal = target;
	}

	return (JIM_OK);
}

void tm_RegisterCoreCommands(Jim_Interp *interp)
{
	Jim_CreateCommand(interp, "rule", ruleCmd, NULL, NULL);
}

