#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"
#include "tm_core_cmds.h"
#include "tm_ext_cmds.h"

#define DEFAULT_FILE "TMakefile"

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

void wrap(Jim_Interp *interp, int error)
{
	if (error == JIM_ERR) {
		Jim_MakeErrorMessage(interp);
		fprintf(stderr, "%s\n", Jim_String(Jim_GetResult(interp)));
		Jim_FreeInterp(interp);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	const char *filename = DEFAULT_FILE;
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
		} else if (filename == DEFAULT_FILE) {
			filename = argv[i];
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
	wrap(interp, Jim_tm_ext_cmdsInit(interp));

	/* Initialize commandline parameters */
	while (parameters) {
		char *var = strtok(parameters->name, "=");
		char *val = strtok(NULL, "\0");

		char buf[1024];

		snprintf(buf, 1024, "set {TM_PARAM(%s)} {%s}", var, val);
		buf[1023] = '\0';

		wrap(interp, Jim_Eval(interp, buf));
		
		parameters = parameters->next;
	}

	/* TMakefile evaluation */
	if (file_exists(filename)) {
		wrap(interp, Jim_EvalFile(interp, filename));
	} else {
		fprintf(stderr, "ERROR: Could not open %s for reading\n", filename);
		retval = EXIT_FAILURE;
	}

	sorted_rules = topsort(tm_goal, tm_rules);

	while (sorted_rules) {
		printf("Making target %s:\n", sorted_rules->rule->target);
		if (sorted_rules->rule->recipe) {
			wrap(interp, Jim_Eval(interp, sorted_rules->rule->recipe));
		}
		sorted_rules = sorted_rules->next;
	}

	/* Free the Tcl interpreter */
	Jim_FreeInterp(interp);

	return retval;
}

