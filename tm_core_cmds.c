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

	len = strlen(fmt) + strlen(target) + strlen(recipe) + 1;
	cmd = malloc(len);
	sprintf(cmd, fmt, target, recipe);
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


void tm_RegisterCoreCommands(Jim_Interp *interp)
{
	Jim_CreateCommand(interp, "rule", ruleCmd, NULL, NULL);
	Jim_CreateCommand(interp, "hash-string", cryptoHashString, NULL, NULL);
	Jim_CreateCommand(interp, "hash-file", cryptoHashFile, NULL, NULL);
}
