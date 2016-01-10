#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sqlite3.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"
#include "tm_crypto.h"
#include "tm_core_cmds.h"
#include "tm_ext_cmds.h"

#define TM_CACHE ".tmcache"
#define DEFAULT_FILE "TMakefile"

#ifndef TM_OPSYS
	#define TM_OPSYS "Unknown"
#endif

#ifndef TM_MACHINE_ARCH
	#define TM_MACHINE_ARCH "Unknown"
#endif

const char *DEFAULT_INC_PATH[] = {
	".",
	"/usr/lib/tmake/include",
	NULL
};

const char *DEFAULT_PKG_PATH[] = {
	".",
	"/usr/lib/tmake/packages",
	NULL
};

int file_exists(const char *filename)
{
	FILE *fp;

	if ((fp = fopen(filename, "r"))) {
		fclose(fp);
		return 1;
	}

	return 0;
}

void usage(int argc, char **argv)
{
	printf("usage: %s [options] [target]\n", argv[0]);
	printf("\n");
	printf("options:\n"
	" -f <TMakefile>    Process <TMakefile>\n"
	" -n                Display commands that would be executed without\n"
	"                   actually executing any commands.\n"
	" -I <path>         Add <path> to the list of directories to search\n"
	"                   for include files.\n"
	" -P <path>         Add <path> to the list of directories to search\n"
	"                   for TMake packages.\n"
	" -D <param>        Define <param> for the execution of TMakefile\n"
	" -V <var>          Display the value of variable <var> without\n"
	"                   executing any commands.\n"
	" -e                Initialize the values of parameters from corresponding\n"
	"                   environment variables.\n"
	" -u                Force update of target even if it is not out of date.\n"
	" -s                Only output errors, if anything at all.\n"
	" PARAM=VALUE       Set the parameter PARAM to VALUE.\n"
	);
	exit(1);
}

char *get(int arg, int argc, char **argv)
{
	if (arg < argc)
		return argv[arg];

	usage(argc, argv);
	return NULL;
}

static int register_search_paths(Jim_Interp *interp, target_list *more_inc, target_list *more_pkg)
{
	Jim_Obj *inc, *pkg;
	int i;

	inc = Jim_NewListObj(interp, NULL, 0);
	pkg = Jim_NewListObj(interp, NULL, 0);

	for (; more_inc; more_inc = more_inc->next) {
		Jim_Obj *elem = Jim_NewStringObj(interp, more_inc->name, strlen(more_inc->name));
		Jim_ListAppendElement(interp, inc, elem);
	}

	for (; more_pkg; more_pkg = more_pkg->next) {
		Jim_Obj *elem = Jim_NewStringObj(interp, more_pkg->name, strlen(more_pkg->name));
		Jim_ListAppendElement(interp, pkg, elem);
	}

	for (i = 0; DEFAULT_INC_PATH[i]; i++) {
		Jim_Obj *elem = NULL;
		elem = Jim_NewStringObj(interp, DEFAULT_INC_PATH[i], strlen(DEFAULT_INC_PATH[i]));
		Jim_ListAppendElement(interp, inc, elem);
	}

	for (i = 0; DEFAULT_PKG_PATH[i]; i++) {
		Jim_Obj *elem = NULL;
		elem = Jim_NewStringObj(interp, DEFAULT_PKG_PATH[i], strlen(DEFAULT_PKG_PATH[i]));
		Jim_ListAppendElement(interp, inc, elem);
	}

	Jim_SetGlobalVariableStr(interp, TM_INCLUDE_PATH, inc);
	Jim_SetGlobalVariableStr(interp, JIM_LIBPATH, pkg);

	return JIM_OK;
}

static target_list *updated_targets = NULL;

int update(sqlite3 *db, const char *tmfile, char *target)
{
	unsigned char digest[CRYPTO_HASH_SIZE];
	char newhash[CRYPTO_HASH_STRING_LENGTH];
	tm_rule *rule = NULL;
	const char *fmt = "INSERT OR REPLACE INTO TMCache (TMakefile, Target, Hash) VALUES (?, ?, ?)";
	sqlite3_stmt *stm = NULL;
	const char *stmtail;
	int sqlrc;
	
	rule = find_rule(target, tm_rules);
	
	if (!rule) {
		return (JIM_ERR);
	}

	updated_targets = target_cons(rule->target, updated_targets);

	if (rule->type == TM_EXPLICIT) {
		TM_CRYPTO_HASH_DATA(rule->recipe, digest);
	} else if (rule->type == TM_FILENAME) {
		TM_CRYPTO_HASH_FILE(target, digest);
	}

	TM_CRYPTO_HASH_TO_STRING(digest, newhash);

	sqlrc = sqlite3_prepare(db, fmt, -1, &stm, &stmtail);
	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "WARNING: Unable to update cache for target %s\n", target);
		return (JIM_ERR);
	}

	sqlite3_bind_text(stm, 1, tmfile, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stm, 2, target, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stm, 3, newhash, -1, SQLITE_TRANSIENT);
	sqlrc = sqlite3_step(stm);
	if (sqlrc != SQLITE_DONE) {
		fprintf(stderr, "WARNING: Error updating cache for target %s\n", target);
		sqlite3_finalize(stm);
		return (JIM_ERR);
	}
	sqlite3_finalize(stm);
	/*
	Adding this return to squeltch compilation warning.
	Cory should review this code
	*/
	return (JIM_OK);
}


int was_updated(char *target)
{
	return target_exists(target, updated_targets);
}


int needs_update(sqlite3 *db, const char *tmfile, char *target)
{
	unsigned char digest[CRYPTO_HASH_SIZE];
	const char *oldhash = NULL;
	char newhash[CRYPTO_HASH_STRING_LENGTH];
	target_list *deps = NULL;
	tm_rule *rule = NULL;
	const char *fmt = "SELECT Hash FROM TMCache WHERE TMakefile = ? AND Target = ?";
	const char *stmtail = NULL;
	sqlite3_stmt *stm = NULL;
	int sqlrc;

	rule = find_rule(target, tm_rules);

	if (!rule) {
		goto yes;
	}

	for (deps = rule->deps; deps; deps = deps->next) {
		if (was_updated(deps->name)) {
			goto yes;
		}
	}

	sqlrc = sqlite3_prepare(db, fmt, -1, &stm, &stmtail);
	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "WARNING: Unable to prepare database statement.\n"
		                "         Assuming %s needs update.\n", target);
		return 1;
	}

	sqlite3_bind_text(stm, 1, tmfile, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stm, 2, target, -1, SQLITE_TRANSIENT);
	sqlrc = sqlite3_step(stm);
	if (sqlrc != SQLITE_ROW) {
		/* There was no row in the cache for this target, so update it */
		goto yes;
	}

	oldhash = (const char *)sqlite3_column_text(stm, 1);
	if (!oldhash) {
		fprintf(stderr, "WARNING: Error getting cache for target %s\n", target);
		goto yes;
	}

	if (rule->type == TM_EXPLICIT) {
		TM_CRYPTO_HASH_DATA(rule->recipe, digest);
	} else if (rule->type == TM_FILENAME) {
		TM_CRYPTO_HASH_FILE(target, digest);
	}

	if (strcmp(oldhash, newhash) == 0) {
		goto no;
	} else {
		goto yes;
	}

	no:
		sqlite3_finalize(stm);
		return 0;
	yes:
		sqlite3_finalize(stm);
		return 1;
}

target_list *need_update(sqlite3 *db, const char *tmfile, target_list *targets)
{
	target_list *oodate = NULL;

	for (; targets; targets = targets->next) {
		if (needs_update(db, tmfile, targets->name)) {
			oodate = target_cons(targets->name, oodate);
		}
	}

	return oodate;
}



void wrap(Jim_Interp *interp, int error) {
	if (error == JIM_ERR) {
		Jim_MakeErrorMessage(interp);
		fprintf(stderr, "%s\n", Jim_String(Jim_GetResult(interp)));
		Jim_FreeInterp(interp);
		exit(EXIT_FAILURE);
	}
}


void update_rules(sqlite3 *db, Jim_Interp *interp, const char *tmfile, tm_rule_list *sorted_rules, int force)
{
	if (!sorted_rules) {
		/* nothing to do */
		return;
	}

	for (; sorted_rules; sorted_rules = sorted_rules->next) {
		tm_rule *rule = sorted_rules->rule;
		if (rule->type == TM_EXPLICIT) {
			/* If the rule has a recipe and needs an update */
			if (rule->recipe
			&&  (force || needs_update(db, tmfile, rule->target))) {
				/* Execute the recipe for the current rule */
				const char *fmt = "recipe::%s {%s} {%s} {%s}";
				int len = 0;
				char *cmd = NULL;
				char *target = rule->target;
				char *inputs = target_list_to_string(rule->deps);
				target_list *oodate_deps = need_update(db, tmfile, rule->deps);
				char *oodate = target_list_to_string(oodate_deps);
				tm_rule_list *oodate_rules = find_rules(oodate_deps, tm_rules);

				update_rules(db, interp, tmfile, oodate_rules, force);

				printf("Making target %s:\n", rule->target);
				len = strlen(fmt) + strlen(target)*2 + strlen(inputs) + strlen(oodate) + 1;
				cmd = malloc(len);
				sprintf(cmd, fmt, target, target, inputs, oodate);
				wrap(interp, Jim_Eval(interp, cmd));
				free(cmd);
				free(oodate);
				free(inputs);
				update(db, tmfile, rule->target);
				rule->type = TM_UPDATED;
				printf("\n");
			} else {
				printf("Target %s is up to date\n\n", rule->target);
			}
		} else if (rule->type == TM_FILENAME) {
			/* Check that the file actually exists */
			if (!file_exists(rule->target)) {
				fprintf(stderr, "ERROR: Unable to find rule for target %s\n", rule->target);
				exit(EXIT_FAILURE);
			}
			if (force || needs_update(db, tmfile, rule->target)) {
				update(db, tmfile, rule->target);
				rule->type = TM_UPDATED;
			}
		}
	}
}


int main(int argc, char **argv)
{
	const char *filename = DEFAULT_FILE;
	char *goal = NULL;
	int silent = 0;
	int force_update = 0;
	int no_execute = 0;
	int env_lookup = 0;
	target_list *also_include = NULL;
	target_list *also_package = NULL;
	target_list *parameters = NULL;

	int retval = EXIT_SUCCESS;
	tm_rule_list *sorted_rules = NULL;

	Jim_Interp *interp = NULL;
	sqlite3 *db = NULL;
	int sqlrc = 0;
	char *sqlerr = NULL;

	int i;
	
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'f':
					filename = get(++i, argc, argv);
					break;
				case 's':
					silent = 1;
					break;
				case 'n':
					no_execute = 1;
					break;
				case 'u':
					force_update = 1;
					break;
				case 'e':
					env_lookup = 1;
					break;
				case 'I':
					also_include = target_cons(get(++i, argc, argv), also_include);
					break;
				case 'P':
					also_package = target_cons(get(++i, argc, argv), also_package);
					break;
				default:
					usage(argc, argv);
					break;
			}
		} else if (strstr(argv[i], "=")) {
			parameters = target_cons(argv[i], parameters);
		} else if (goal == NULL) {
			goal = argv[i];
		} else {
			usage(argc, argv);
		}
	}

	/* Create a Tcl interpreter */
	interp = Jim_CreateInterp();
	assert(interp != NULL && "couldn't create interpreter");

	/* Register the core Tcl commands */
	Jim_RegisterCoreCommands(interp);
	tm_RegisterCoreCommands(interp);

	/* Initialize any static extensions */
	Jim_InitStaticExtensions(interp);

	/* Initialize the Tcl TMake commands */
	wrap(interp, Jim_Eval(interp, "set TM_OPSYS " TM_OPSYS));
	wrap(interp, Jim_Eval(interp, "set TM_MACHINE_ARCH " TM_MACHINE_ARCH));
	wrap(interp, Jim_tm_ext_cmdsInit(interp));

	register_search_paths(interp, also_include, also_package);

	/* Initialize commandline parameters */
	while (parameters) {
		char *var = strtok(parameters->name, "=");
		char *val = strtok(NULL, "\0");

		char *buf = NULL;
		int len = strlen(var) + strlen(val) + strlen("set {TM_PARAM()} {}") + 1;
		buf = malloc(len);
		sprintf(buf, "set {TM_PARAM(%s)} {%s}", var, val);

		wrap(interp, Jim_Eval(interp, buf));

		free(buf);
		parameters = parameters->next;
	}

	if (env_lookup) {
		wrap(interp, Jim_Eval(interp, "set TM_ENV_LOOKUP 1"));
	}

	if (no_execute) {
		wrap(interp, Jim_Eval(interp, "set TM_NO_EXECUTE 1"));
	}

	if (silent) {
		wrap(interp, Jim_Eval(interp, "set TM_SILENT_MODE 1"));
	}

	goal = goal ? goal : tm_goal;

	if (goal) {
		char *fmt = "set TM_CURRENT_GOAL %s";
		char *cmd = NULL;
		
		cmd = malloc(strlen(fmt) + strlen(goal) + 1);
		sprintf(cmd, fmt, goal);
		wrap(interp, Jim_Eval(interp, cmd));
	}

	/* TMakefile evaluation */
	if (file_exists(filename)) {
		wrap(interp, Jim_EvalFile(interp, filename));
	} else {
		fprintf(stderr, "ERROR: Could not open %s for reading\n", filename);
		retval = EXIT_FAILURE;
	}

	goal = goal ? goal : tm_goal;

	if (!target_exists(goal, get_targets(tm_rules))) {
		fprintf(stderr, "ERROR: goal target %s not defined\n", goal);
		exit(EXIT_FAILURE);
	}

	sorted_rules = topsort(goal, tm_rules);

	if (!sorted_rules) {
		fprintf(stderr, "ERROR: Could not find rule to make %s\n", goal);
		exit(EXIT_FAILURE);
	}

	sqlrc = sqlite3_open(TM_CACHE, &db);
	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "ERROR: Unable to open " TM_CACHE " database\n");
		exit(EXIT_FAILURE);
	}

	sqlrc = sqlite3_exec(db,
		"CREATE TABLE IF NOT EXISTS TMCache ("
			"TMakefile    TEXT,"
			"Target       TEXT,"
			"Hash         TEXT"
		")",
		NULL, NULL, &sqlerr
	);

	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "ERROR: Unable to create database schema: %s", sqlerr);
		sqlite3_free(sqlerr);
		exit(EXIT_FAILURE);
	}

	update_rules(db, interp, filename, sorted_rules, force_update);

	sqlrc = sqlite3_close(db);
	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "WARNING: Exiting while " TM_CACHE " database is busy\n");
	}

	/* Free the Tcl interpreter */
	Jim_FreeInterp(interp);

	return retval;
}
