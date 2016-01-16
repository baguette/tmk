#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tmake.h"
#include "tm_target.h"
#include "tm_crypto.h"
#include "tm_update.h"


target_list *updated_targets = NULL;


int file_exists(const char *filename)
{
	FILE *fp;

	if ((fp = fopen(filename, "r"))) {
		fclose(fp);
		return 1;
	}

	return 0;
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


int update(sqlite3 *db, const char *tmfile, const char *target)
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
	} else {
		fprintf(stderr, "WARNING: Unknown rule type %d\n"
		                "         Don't know how to update cache for %s\n", rule->type, target);
		return (JIM_ERR);
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
	Adding this return to squelch compilation warning.
	Cory should review this code
	*/
	return (JIM_OK);
}


int was_updated(const char *target)
{
	return target_exists(target, updated_targets);
}


int needs_update(sqlite3 *db, const char *tmfile, const char *target)
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

	oldhash = (const char *)sqlite3_column_text(stm, 0);
	if (!oldhash) {
		fprintf(stderr, "WARNING: Error getting cache for target %s\n", target);
		goto yes;
	}

	if (rule->type == TM_EXPLICIT) {
		TM_CRYPTO_HASH_DATA(rule->recipe, digest);
	} else if (rule->type == TM_FILENAME) {
		TM_CRYPTO_HASH_FILE(target, digest);
	} else {
		fprintf(stderr, "WARNING: Unexpected rule type %d\n"
		                "         Assuming %s needs update.\n", rule->type, target);
		goto yes;
	}
	TM_CRYPTO_HASH_TO_STRING(digest, newhash);

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


void update_rules(sqlite3 *db,
                  Jim_Interp *interp,
                  const char *tmfile,
                  tm_rule_list *sorted_rules,
                  int force)
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
				const char *target = rule->target;
				char *inputs = target_list_to_string(rule->deps);
				target_list *oodate_deps = need_update(db, tmfile, rule->deps);
				char *oodate = target_list_to_string(oodate_deps);
				tm_rule_list *oodate_rules = find_rules(oodate_deps, tm_rules);

				if (was_updated(target)) {
					goto done;
				}

				update_rules(db, interp, tmfile, oodate_rules, force);

				printf("Making target %s:\n", rule->target);
				len = strlen(fmt) + strlen(target)*2 + strlen(inputs) + strlen(oodate) + 1;
				cmd = malloc(len);
				sprintf(cmd, fmt, target, target, inputs, oodate);
				wrap(interp, Jim_Eval(interp, cmd));

				update(db, tmfile, rule->target);
				printf("\n");

				done:
				free(cmd);
				free(oodate);
				free(inputs);
				free_rule_list(oodate_rules);
				free_target_list(oodate_deps);

				rule->type = TM_UPDATED;
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

