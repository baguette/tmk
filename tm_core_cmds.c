#include <stdio.h>
#include <string.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"
#include "tm_core_cmds.h"
#include "tm_crypto.h"

static int ruleCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	tm_rule *rule = NULL;
	target_list *deps = NULL;
	Jim_Obj *target_subst, *deps_subst;
	char *target = NULL;
	char *recipe = NULL;
	int i, numdeps;
	const char *fmt = "proc recipe::%s {TARGET INPUTS OODATE} { \
	%s\
	}";
	int len = 0;
	char *cmd = NULL;

	if (argc < 3 || argc > 4) {
		Jim_WrongNumArgs(interp, 2, argv, "rule target-list dep-list ?script?");
		return (JIM_ERR);
	}

	Jim_SubstObj(interp, argv[1], &target_subst, 0);
	target = target_copy(Jim_String(target_subst));

	if (argc == 4) {
		recipe = target_copy(Jim_String(argv[3]));
	}

	Jim_SubstObj(interp, argv[2], &deps_subst, 0);
	numdeps = Jim_ListLength(interp, deps_subst);
	for (i = 0; i < numdeps; i++) {
		Jim_Obj *dep = Jim_ListGetIndex(interp, deps_subst, i);
		char *sdep = target_copy(Jim_String(dep));

		deps = target_cons(sdep, deps);
	}

	rule = new_rule(target, deps, recipe);
	tm_rules = rule_cons(rule, tm_rules);

	if (tm_goal == NULL) {
		const char *fmt_set = "set TM_CURRENT_GOAL %s";

		tm_goal = target;

		len = strlen(fmt_set) + strlen(target) + 1;
		cmd = malloc(len);
		sprintf(cmd, fmt_set, target);
		Jim_Eval(interp, cmd);
		free(cmd);
	}

	len = strlen(fmt) + strlen(target) + (recipe ? strlen(recipe) : 0) + 1;
	cmd = malloc(len);
	sprintf(cmd, fmt, target, recipe ? recipe : "");
	Jim_Eval(interp, cmd);
	free(cmd);

	return (JIM_OK);
}

/* Crypto hash of string interface to jimtcl */
static int cryptoHashString(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char hash[CRYPTO_HASH_STRING_LENGTH];
	unsigned char digest[CRYPTO_HASH_SIZE];
	const char *string = NULL;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "hash-string string");
		return (JIM_ERR);
	}

	string = Jim_String(argv[1]);

	TM_CRYPTO_HASH_DATA(string, digest);
	TM_CRYPTO_HASH_TO_STRING(digest, hash);

	Jim_SetResultString(interp, hash, CRYPTO_HASH_STRING_LENGTH);

	return (JIM_OK);
}


/* Crypto hash of file interface to jimtcl */

static int cryptoHashFile(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char hash[CRYPTO_HASH_STRING_LENGTH];
	unsigned char digest[CRYPTO_HASH_SIZE];
	const char *filename = NULL;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "hash-file filename");
		return (JIM_ERR);
	}

	filename = Jim_String(argv[1]);

	TM_CRYPTO_HASH_FILE(filename, digest);
	TM_CRYPTO_HASH_TO_STRING(digest, hash);

	Jim_SetResultString(interp, hash, CRYPTO_HASH_STRING_LENGTH);

	return (JIM_OK);
}

static int targetCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char *target = NULL;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "target name");
		return (JIM_ERR);
	}

	target = target_copy(Jim_String(argv[1]));

	if (target_exists(target, get_targets(tm_rules))) {
		Jim_SetResultInt(interp, 1);
	} else {
		Jim_SetResultInt(interp, 0);
	}

	free(target);

	return (JIM_OK);
}


void tm_RegisterCoreCommands(Jim_Interp *interp)
{
	Jim_CreateCommand(interp, "rule", ruleCmd, NULL, NULL);
	Jim_CreateCommand(interp, "target", targetCmd, NULL, NULL);
	Jim_CreateCommand(interp, "hash-string", cryptoHashString, NULL, NULL);
	Jim_CreateCommand(interp, "hash-file", cryptoHashFile, NULL, NULL);
}
