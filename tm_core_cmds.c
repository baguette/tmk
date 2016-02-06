#include <stdio.h>
#include <string.h>
#include <unistd.h>   /* TODO: I doubt this works on Windows... */

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
	const char *recipe = NULL;
	int i, numtargs, numdeps;
	const char *fmt = "proc recipe::%s {TARGET INPUTS OODATE} { \
	%s\
	}";
	int len = 0;
	char *cmd = NULL;
	int ret = JIM_ERR;

	/* check to make sure we got the information we need */
	if (argc < 3 || argc > 4) {
		Jim_WrongNumArgs(interp, 2, argv, "rule target-list dep-list ?script?");
		return (JIM_ERR);
	}

	/* If we've got a recipe, store it */
	if (argc == 4) {
		recipe = Jim_String(argv[3]);
	}

	/* Perform variable substitution in the dependency lists */
	if (Jim_SubstObj(interp, argv[2], &deps_subst, 0) != JIM_OK) {
		return (JIM_ERR);
	}

	/* Get all the dependencies and store them in a dep list */
	numdeps = Jim_ListLength(interp, deps_subst);
	for (i = 0; i < numdeps; i++) {
		Jim_Obj *dep = Jim_ListGetIndex(interp, deps_subst, i);
		const char *sdep = Jim_String(dep);

		deps = target_cons(sdep, deps);
	}

	/* Perform variable substitution in the target list */
	if (Jim_SubstObj(interp, argv[1], &target_subst, 0) != JIM_OK) {
		return (JIM_ERR);
	}

	numtargs = Jim_ListLength(interp, target_subst);

	if (numtargs == 0) {
		fprintf(stderr, "ERROR: no targets specified for rule\n");
		free(deps);
		return (JIM_ERR);
	}

	/* For each target in the list, create a rule and a proc */
	for (i = 0; i < numtargs; i++) {
		Jim_Obj *target_obj = Jim_ListGetIndex(interp, target_subst, i);
		const char *target = Jim_String(target_obj);

		/* check if there's already a rule for this target */
		rule = find_rule(target, tm_rules);
		if (rule) {
			/* if so, overwrite it with the new rule */
			free_target_list(rule->deps);
			free(rule->recipe);
			rule->deps = target_list_copy(deps);
			if (recipe) {
				rule->recipe = malloc(strlen(recipe) + 1);
				strcpy(rule->recipe, recipe);
			} else {
				rule->recipe = NULL;
			}
			rule->type = TM_EXPLICIT;
		} else {
			/* or else create a new rule */
			rule = new_rule(target, deps, recipe);
			tm_rules = rule_cons(rule, tm_rules);
			free_rule(rule);
		}

		/* Do we need to set the default goal? */
		if (tm_goal == NULL) {
			const char *fmt_set = "set TM_CURRENT_GOAL %s";

			tm_goal = malloc(strlen(target) + 1);
			strcpy(tm_goal, target);

			len = strlen(fmt_set) + strlen(target) + 1;
			cmd = malloc(len);
			sprintf(cmd, fmt_set, target);
			ret = Jim_Eval(interp, cmd);
			free(cmd);
			if (ret != (JIM_OK)) {
				goto error;
			}
		}

		/* Create a proc representing this rule */
		len = strlen(fmt) + strlen(target) + (recipe ? strlen(recipe) : 0) + 1;
		cmd = malloc(len);
		sprintf(cmd, fmt, target, recipe ? recipe : "");
		ret = Jim_Eval(interp, cmd);
		free(cmd);
		if (ret != (JIM_OK)) {
			goto error;
		}
	}

	free_target_list(deps);
	return (JIM_OK);

	error:
	free_target_list(deps);
	return ret;
}

/* Provide tm_crypto functionality to Jim Tcl */
static int sha1sumCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char hash[CRYPTO_HASH_STRING_LENGTH];
	unsigned char digest[CRYPTO_HASH_SIZE];
	const char *subcmd = NULL;
	const char *string = NULL;

	if (argc != 3) {
		Jim_WrongNumArgs(interp, 2, argv, "sha1sum \"file\"|\"string\" <filename>|<string>");
		return (JIM_ERR);
	}

	subcmd = Jim_String(argv[1]);
	string = Jim_String(argv[2]);

	if (strcmp(subcmd, "file") == 0) {
		TM_CRYPTO_HASH_FILE(string, digest);
	} else if (strcmp(subcmd, "string") == 0) {
		TM_CRYPTO_HASH_DATA(string, digest);
	} else {
		Jim_WrongNumArgs(interp, 2, argv, "Invalid subcommand for sha1sum (should be \"file\" or \"string\")");
		return (JIM_ERR);
	}

	TM_CRYPTO_HASH_TO_STRING(digest, hash);

	Jim_SetResultString(interp, hash, CRYPTO_HASH_STRING_LENGTH);

	return (JIM_OK);
}


static int targetCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *target = NULL;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "target name");
		return (JIM_ERR);
	}

	target = Jim_String(argv[1]);

	if (find_rule(target, tm_rules)) {
		Jim_SetResultInt(interp, 1);
	} else {
		Jim_SetResultInt(interp, 0);
	}

	return (JIM_OK);
}

static int commandsCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *target = NULL;
	tm_rule *rule = NULL;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "commands target");
		return (JIM_ERR);
	}
	
	target = Jim_String(argv[1]);
	rule = find_rule(target, tm_rules);

	if (rule && rule->recipe) {
		Jim_SetResultInt(interp, 1);
	} else {
		Jim_SetResultInt(interp, 0);
	}

	return (JIM_OK);
}

/**
 * Searches along a of paths for the given package.
 *
 * Returns the allocated path to the package file if found,
 * or NULL if not found.
 */
static char *tm_FindPackage(Jim_Interp *interp, Jim_Obj *prefixListObj, const char *pkgName)
{
	int i;
	int prefixc = Jim_ListLength(interp, prefixListObj);
	
	for (i = 0; i < prefixc; i++) {
		Jim_Obj *prefixObjPtr = Jim_ListGetIndex(interp, prefixListObj, i);
		const char *prefix = Jim_String(prefixObjPtr);
		int len = strlen(prefix) + strlen(pkgName) + 2;
		char *buf = Jim_Alloc(len);
		
		if (strcmp(prefix, ".") == 0) {
			sprintf(buf, "%s", pkgName);
		} else {
			sprintf(buf, "%s/%s", prefix, pkgName);
		}
		
		if (access(buf, R_OK) == 0) {
			return buf;
		}

		Jim_Free(buf);
	}
	return NULL;
}

static int includeCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	int ret = JIM_ERR;
	const char *filename = NULL;
	Jim_Obj *incpath = Jim_GetGlobalVariableStr(interp, TM_INCLUDE_PATH, JIM_NONE);

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "include filename");
		return (JIM_ERR);
	}
	
	filename = Jim_String(argv[1]);

	if (incpath) {
		char *path = NULL;
		path = tm_FindPackage(interp, incpath, filename);
		if (path) {
			Jim_IncrRefCount(incpath);
			ret = Jim_EvalFileGlobal(interp, path);
			Jim_DecrRefCount(interp, incpath);
		}
		Jim_Free(path);
	} else {
		return JIM_ERR;
	}

	return ret;
}


void tm_RegisterCoreCommands(Jim_Interp *interp)
{
	Jim_CreateCommand(interp, "rule", ruleCmd, NULL, NULL);
	Jim_CreateCommand(interp, "include", includeCmd, NULL, NULL);
	Jim_CreateCommand(interp, "target", targetCmd, NULL, NULL);
	Jim_CreateCommand(interp, "commands", commandsCmd, NULL, NULL);
	Jim_CreateCommand(interp, "sha1sum", sha1sumCmd, NULL, NULL);
}
