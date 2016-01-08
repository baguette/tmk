#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"
#include "tm_core_cmds.h"

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

int main(int argc, char **argv)
{
	const char *filename = DEFAULT_FILE;
	int retval = EXIT_SUCCESS;
	tm_rule_list *sorted_rules = NULL;
	tm_rule_list *current_rules = NULL;

	Jim_Interp *interp = NULL;

	/* Create a Tcl interpreter */
	interp = Jim_CreateInterp();
	assert(interp != NULL && "couldn't create interpreter");

	/* Register the core Tcl commands */
	Jim_RegisterCoreCommands(interp);
	tm_RegisterCoreCommands(interp);

	/* Initialize any static extensions */
	Jim_InitStaticExtensions(interp);

	/* TMakefile evaluation */
	if (file_exists(filename)) {
		Jim_EvalFile(interp, filename);
	} else {
		fprintf(stderr, "ERROR: Could not open %s for reading\n", filename);
		retval = EXIT_FAILURE;
	}

	sorted_rules = topsort(tm_goal, tm_rules);

	current_rules = sorted_rules;
	while (current_rules) {
		printf("Making target %s:\n", current_rules->rule->target);
		if (current_rules->rule->recipe) {
			Jim_Eval(interp, current_rules->rule->recipe);
		}
		current_rules = current_rules->next;
	}

	/* Free the Tcl interpreter */
	Jim_FreeInterp(interp);

	return retval;
}

