#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"
#include "tm_crypto.h"
#include "tm_core_cmds.h"
#include "tm_ext_cmds.h"

#define TM_CACHE ".tmcache"
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

static target_list *updated_targets = NULL;

int update(char *target)
{
	char *cachefile = NULL;
	unsigned char digest[CRYPTO_HASH_SIZE];
	char newhash[CRYPTO_HASH_STRING_LENGTH];
	tm_rule *rule = NULL;

	cachefile = malloc(strlen(TM_CACHE) + strlen(target) + 2);
	sprintf(cachefile, TM_CACHE "/%s", target);

	rule = find_rule(target, tm_rules);
	
	if (!rule) {
		return (JIM_ERR);
	}

	updated_targets = target_cons(rule->target, updated_targets);

	if (rule->type == TM_EXPLICIT) {
		FILE *fp = NULL;

		TM_CRYPTO_HASH_DATA(rule->recipe, digest);
		TM_CRYPTO_HASH_TO_STRING(digest, newhash);

		fp = fopen(cachefile, "w");
		fwrite(newhash, 1, strlen(newhash), fp);
		fclose(fp);
	} else if (rule->type == TM_FILENAME) {
		/* assume it's a file */
		FILE *fp = NULL;

		TM_CRYPTO_HASH_FILE(target, digest);
		TM_CRYPTO_HASH_TO_STRING(digest, newhash);

		fp = fopen(cachefile, "w");
		fwrite(newhash, 1, strlen(newhash), fp);
		fclose(fp);
	}

	free(cachefile);

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


int needs_update(char *target)
{
	char *cachefile = NULL;
	unsigned char digest[CRYPTO_HASH_SIZE];
	char oldhash[CRYPTO_HASH_STRING_LENGTH];
	char newhash[CRYPTO_HASH_STRING_LENGTH];
	target_list *deps = NULL;
	tm_rule *rule = NULL;

	cachefile = malloc(strlen(TM_CACHE) + strlen(target) + 2);
	sprintf(cachefile, TM_CACHE "/%s", target);

	rule = find_rule(target, tm_rules);

	if (!rule) {
		goto yes;
	}

	for (deps = rule->deps; deps; deps = deps->next) {
		if (was_updated(deps->name)) {
			goto yes;
		}
	}

	if (rule->type == TM_EXPLICIT) {
		if (file_exists(cachefile)) {
			FILE *fp = NULL;

			fp = fopen(cachefile, "r");
			fread(oldhash, 1, CRYPTO_HASH_STRING_LENGTH, fp);
			fclose(fp);
			oldhash[CRYPTO_HASH_STRING_LENGTH-1] = '\0';

			TM_CRYPTO_HASH_DATA(rule->recipe, digest);
			TM_CRYPTO_HASH_TO_STRING(digest, newhash);

			if (strcmp(oldhash, newhash) == 0) {
				goto no;
			} else {
				goto yes;
			}
		} else {
			goto yes;
		}
	} else if (rule->type == TM_FILENAME) {
		if (file_exists(cachefile)) {
			FILE *fp = NULL;

			fp = fopen(cachefile, "r");
			fread(oldhash, 1, CRYPTO_HASH_STRING_LENGTH, fp);
			fclose(fp);
			oldhash[CRYPTO_HASH_STRING_LENGTH-1] = '\0';

			TM_CRYPTO_HASH_FILE(target, digest);
			TM_CRYPTO_HASH_TO_STRING(digest, newhash);

			if (strcmp(oldhash, newhash) == 0) {
				goto no;
			} else {
				goto yes;
			}
		} else {
			goto yes;
		}
	}

	no:
		free(cachefile);
		return 0;
	yes:
		free(cachefile);
		return 1;
}

target_list *need_update(target_list *targets)
{
	target_list *oodate = NULL;

	for (targets = targets; targets; targets = targets->next) {
		if (needs_update(targets->name)) {
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


void update_rules(Jim_Interp *interp, tm_rule_list *sorted_rules, int force)
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
			&&  (force || needs_update(rule->target))) {
				/* Execute the recipe for the current rule */
				const char *fmt = "recipe::%s {%s} {%s} {%s}";
				int len = 0;
				char *cmd = NULL;
				char *target = rule->target;
				char *inputs = target_list_to_string(rule->deps);
				target_list *oodate_deps = need_update(rule->deps);
				char *oodate = target_list_to_string(oodate_deps);
				tm_rule_list *oodate_rules = find_rules(oodate_deps, tm_rules);

				update_rules(interp, oodate_rules, force);

				printf("Making target %s:\n", rule->target);
				len = strlen(fmt) + strlen(target)*2 + strlen(inputs) + strlen(oodate) + 1;
				cmd = malloc(len);
				sprintf(cmd, fmt, target, target, inputs, oodate);
				wrap(interp, Jim_Eval(interp, cmd));
				free(cmd);
				free(oodate);
				free(inputs);
				update(rule->target);
				rule->type = TM_UPDATED;
			} else {
				printf("Target %s is up to date\n", rule->target);
			}
		} else if (rule->type == TM_FILENAME) {
			/* Check that the file actually exists */
			if (!file_exists(rule->target)) {
				fprintf(stderr, "ERROR: Unable to find rule for target %s", rule->target);
				exit(EXIT_FAILURE);
			}
			update(rule->target);
			rule->type = TM_UPDATED;
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
	wrap(interp, Jim_tm_ext_cmdsInit(interp));

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

	/* TMakefile evaluation */
	if (file_exists(filename)) {
		wrap(interp, Jim_EvalFile(interp, filename));
	} else {
		fprintf(stderr, "ERROR: Could not open %s for reading\n", filename);
		retval = EXIT_FAILURE;
	}

	wrap(interp, Jim_Eval(interp, "file mkdir " TM_CACHE));

	goal = goal ? goal : tm_goal;
	sorted_rules = topsort(goal, tm_rules);

	if (!sorted_rules) {
		fprintf(stderr, "ERROR: Could not find rule to make %s\n", goal);
		exit(EXIT_FAILURE);
	}

	update_rules(interp, sorted_rules, force_update);

	/* Free the Tcl interpreter */
	Jim_FreeInterp(interp);

	return retval;
}
