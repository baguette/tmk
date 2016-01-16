#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tmake.h"
#include "tm_target.h"
#include "tm_update.h"
#include "tm_core_cmds.h"
#include "tm_ext_cmds.h"


const char *DEFAULT_INC_PATH[] = {
	".",
	TM_PREFIX "/lib/tmake/include",
	NULL
};

const char *DEFAULT_PKG_PATH[] = {
	".",
	TM_PREFIX "/lib/tmake/packages",
	NULL
};

static void usage(const char *progname)
{
	printf("usage: %s [options] [target]\n", progname);
	printf("\n");
	printf("options:\n");
	printf(" -f <TMakefile>    Process <TMakefile>\n");
	printf(" -n                Display commands that would be executed without\n"
	       "                   actually executing any commands.\n");
	printf(" -I <path>         Add <path> to the list of directories to search\n"
	       "                   for include files.\n");
	printf(" -P <path>         Add <path> to the list of directories to search\n"
	       "                   for TMake packages.\n");
	printf(" -D <param>        Define <param> for the execution of TMakefile\n");
	printf(" -V <var>          Display the value of variable <var> without\n"
	       "                   executing any commands.\n");
	printf(" -e                Initialize the values of parameters from corresponding\n"
	       "                   environment variables.\n");
	printf(" -u                Force update of target even if it is not out of date.\n");
	printf(" -s                Only output errors, if anything at all.\n");
	printf(" PARAM=VALUE       Set the parameter PARAM to VALUE.\n");
	exit(1);
}

static char *get(int *arg, int argc, char **argv)
{
	if (strlen(argv[*arg]) > 2) {
		return &(argv[*arg][2]);
	}

	if ((*arg + 1) < argc) {
		*arg = *arg + 1;
		return argv[*arg];
	}

	usage(argv[0]);
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


int main(int argc, char **argv)
{
	const char *filename = DEFAULT_FILE;
	const char *goal = NULL;
	int silent = 0;
	int force_update = 0;
	int no_execute = 0;
	int env_lookup = 0;
	target_list *also_include = NULL;
	target_list *also_package = NULL;
	target_list *parameters = NULL;
	target_list *defines = NULL;

	int retval = EXIT_SUCCESS;
	tm_rule_list *sorted_rules = NULL;

	Jim_Interp *interp = NULL;
	sqlite3 *db = NULL;
	int sqlrc = 0;
	char *sqlerr = NULL;

	target_list *node = NULL;

	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'f':
					filename = get(&i, argc, argv);
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
					also_include = target_cons(get(&i, argc, argv), also_include);
					break;
				case 'P':
					also_package = target_cons(get(&i, argc, argv), also_package);
					break;
				case 'D':
					defines = target_cons(get(&i, argc, argv), defines);
					break;
				default:
					usage(argv[0]);
					break;
			}
		} else if (strstr(argv[i], "=")) {
			parameters = target_cons(argv[i], parameters);
		} else if (goal == NULL) {
			goal = argv[i];
		} else {
			usage(argv[0]);
		}
	}

	/* Create a Tcl interpreter */
	interp = Jim_CreateInterp();
	if (interp == NULL) {
		fprintf(stderr, "ERROR: Unable to create Tcl interpreter\n");
		return (EXIT_FAILURE);
	}

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
	free_target_list(also_include);
	free_target_list(also_package);

	/* Initialize commandline parameters */
	for (node = parameters; node; node = node->next) {
		char *var = strtok(node->name, "=");
		char *val = strtok(NULL, "\0");

		const char *fmt = "set {TM_PARAM(%s)} {%s}";
		int len = strlen(var) + strlen(val) + strlen(fmt) + 1;
		char *cmd = malloc(len);

		sprintf(cmd, fmt, var, val);
		wrap(interp, Jim_Eval(interp, cmd));

		free(cmd);
	}
	free_target_list(parameters);

	for (node = defines; node; node = node->next) {
		const char *fmt = "set {%s} 1";
		int len = strlen(node->name) + strlen(fmt) + 1;
		char *cmd = malloc(len);

		sprintf(cmd, fmt, node->name);
		wrap(interp, Jim_Eval(interp, cmd));

		free(cmd);
	}
	free_target_list(defines);

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

	if (!find_rule(goal, tm_rules)) {
		fprintf(stderr, "ERROR: No rule for goal %s\n", goal);
		exit(EXIT_FAILURE);
	}

	find_files(&tm_rules);

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
			"Hash         TEXT,"
		"CONSTRAINT OneFileTarget UNIQUE (TMakefile, Target) ON CONFLICT REPLACE"
		")",
		NULL, NULL, &sqlerr
	);

	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "ERROR: Unable to create database schema: %s", sqlerr);
		sqlite3_free(sqlerr);
		exit(EXIT_FAILURE);
	}

	update_rules(db, interp, filename, sorted_rules, force_update);

	if (!updated_targets) {
		printf("Target %s is up to date\n", goal);
	}

	free_rule_list(sorted_rules);
	free_rule_list(tm_rules);

	sqlrc = sqlite3_close(db);
	if (sqlrc != SQLITE_OK) {
		fprintf(stderr, "WARNING: Exiting while " TM_CACHE " database is busy\n");
	}

	/* Free the Tcl interpreter */
	Jim_FreeInterp(interp);

	return retval;
}
