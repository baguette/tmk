#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tm_target.h"
#include "tm_core_cmds.h"

/* The goal target of the current execution */
char *tm_goal = NULL;

/* All the rules defined by the TMakefile */
tm_rule_list *tm_rules = NULL;


/* Create a new explicit rule.
 * Makes copies of all the arguments to store in the rule.
 */
tm_rule *new_rule(const char *target, target_list *deps, const char *recipe)
{
	tm_rule *rule = malloc(sizeof(tm_rule));
	rule->target = malloc(strlen(target) + 1);
	if (recipe)
		rule->recipe = malloc(strlen(recipe) + 1);
	
	strcpy(rule->target, target);
	rule->deps = target_list_copy(deps);
	if (recipe)
		strcpy(rule->recipe, recipe);
	else
		rule->recipe = NULL;
	rule->type = TM_EXPLICIT;
	rule->mark = TM_UNMARKED;
	rule->always_oodate = 0;

	return rule;
}

/* Create a new filename rule.
 * Makes a copy of its argument.
 */
tm_rule *new_filename(const char *target)
{
	tm_rule *rule = new_rule(target, NULL, NULL);

	rule->type = TM_FILENAME;

	return rule;
}

/* Make a copy of a rule.
 */
tm_rule *rule_copy(tm_rule *rule)
{
	tm_rule *copy;

	if (!rule) {
		return NULL;
	}

	copy = malloc(sizeof(tm_rule));

	if (rule->target) {
		copy->target = malloc(strlen(rule->target) + 1);
		strcpy(copy->target, rule->target);
	} else {
		copy->target = NULL;
	}
	if (rule->deps) {
		copy->deps = target_list_copy(rule->deps);
	} else {
		copy->deps = NULL;
	}
	if (rule->recipe) {
		copy->recipe = malloc(strlen(rule->recipe) + 1);
		strcpy(copy->recipe, rule->recipe);
	} else {
		copy->recipe = NULL;
	}
	copy->type = rule->type;
	copy->mark = rule->mark;

	return copy;
}

/* Free a rule */
void free_rule(tm_rule *rule)
{
	if (!rule) return;
	free(rule->target);
	free_target_list(rule->deps);
	free(rule->recipe);
	free(rule);
}



/* Takes the name of a target and another target list and
 * returns a target list with the target at the head.
 * Target lists are NULL-terminated.
 *
 * e.g., to make a one-element list:
 *     target_cons("foo", NULL);
 */
target_list *target_cons(const char *name, target_list *next)
{
	target_list *targets = malloc(sizeof(target_list));

	targets->name = malloc(strlen(name) + 1);

	strcpy(targets->name, name);
	targets->next = next;

	return targets;
}

/* Frees a target list.
 */
void free_target_list(target_list *targets)
{
	while (targets) {
		target_list *node = targets->next;
		free(targets->name);
		free(targets);
		targets = node;
	}
}

/* Copy a target list.
 */
target_list *target_list_copy(target_list *targets)
{
	target_list *rev;
	target_list *copy;

	if (!targets) return NULL;
	
	rev = target_list_reverse(targets);
	copy = target_list_reverse(rev);
	free_target_list(rev);
	return copy;
}


/* Take a rule and another rule list and return
 * a rule list with the rule at the head.
 * Rule lists are NULL-terminated.
 */
tm_rule_list *rule_cons(tm_rule *rule, tm_rule_list *next)
{
	tm_rule_list *rules = malloc(sizeof(tm_rule_list));

	rules->rule = rule_copy(rule);
	rules->next = next;
	
	return rules;
}

/* Frees a rule list
 */
void free_rule_list(tm_rule_list *rules)
{
	while (rules) {
		tm_rule_list *node = rules->next;
		if (rules->rule)
			free_rule(rules->rule);
		free(rules);
		rules = node;
	}
}


/* Return 1 if the target is in targets, else return 0.
 */
int target_exists(const char *target, target_list *targets)
{
	for (; targets; targets = targets->next) {
		if (strcmp(targets->name, target) == 0) {
			return 1;
		}
	}

	return 0;
}


/* Take a target and a rule list and return the rule associated
 * with that target if it exists, otherwise return NULL.
 */
tm_rule *find_rule(const char *target, tm_rule_list *rules)
{
	tm_rule_list *node = rules;
	tm_rule *rule = NULL;

	while (node) {
		if (strcmp(node->rule->target, target) == 0) {
			rule = node->rule;
			break;
		}

		node = node->next;
	}

	return rule;
}


/* Take a list of targets and a pointer to a rule list.
 * If no rule for a target is found in the rule list, add
 * a new TM_FILENAME rule to the rule list.
 */
static void find_files_from_deps(target_list *targets, tm_rule_list **rules)
{
	for (; targets; targets = targets->next) {
		tm_rule *rule = find_rule(targets->name, *rules);

		if (!rule) {
			rule = new_filename(targets->name);
			*rules = rule_cons(rule, *rules);
			free_rule(rule);
		}
	}
}

/* Scour through all the dependencies in a rule list and allocate new
 * filename rules for things deemed to be files.
 * Updates the rule list with the new rules.
 */
void find_files(tm_rule_list **rules)
{
	tm_rule_list *node;

	for (node = *rules; node; node = node->next) {
		find_files_from_deps(node->rule->deps, rules);
	}
}


/* Take a list of targets and a list of rules and return
 * a list of the rules associated with those targets.
 */
tm_rule_list *find_rules(target_list *targets, tm_rule_list *rules)
{
	tm_rule_list *mapped = NULL;
	target_list *node = targets;

	while (node) {
		tm_rule *rule = find_rule(node->name, rules);
		if (rule)
			mapped = rule_cons(rule, mapped);
		node = node->next;
	}

	return mapped;
}


/* Return a new rule list representing the reverse of another rule list
 */
tm_rule_list *rule_list_reverse(tm_rule_list *rules)
{
	tm_rule_list *rev = NULL;
	tm_rule_list *node = rules;

	for (; node; node = node->next) {
		rev = rule_cons(node->rule, rev);
	}

	return rev;
}

/* Return a new target list representing the reverse of another target list
 */
target_list *target_list_reverse(target_list *targets)
{
	target_list *rev = NULL;
	target_list *node = targets;

	for (; node; node = node->next) {
		rev = target_cons(node->name, rev);
	}

	return rev;
}


/* Tarjan's algorithm for topological sorting */
static int topsort_visit(tm_rule *rule, tm_rule_list *rules, tm_rule_list **sorted)
{
	if (rule->mark == TM_TEMPORARY) {
		fprintf(stderr, "ERROR:  Cycle detected in dependency graph\n");
		return -1;
	}

	if (rule->mark == TM_UNMARKED) {
		tm_rule_list *deps = NULL;
		tm_rule_list *node = NULL;

		rule->mark = TM_TEMPORARY;
		deps = find_rules(rule->deps, rules);
		
		for (node = deps; node; node = node->next) {
			int n = 0;

			if (node->rule)
				n = topsort_visit(node->rule, rules, sorted);

			if (n < 0) {
				free_rule_list(deps);
				return n;
			}
		}
		free_rule_list(deps);

		rule->mark = TM_PERMANENT;
		*sorted = rule_cons(rule, *sorted);
	}

	return 0;
}

/* Perform a topological sort of the dependency graph to reach target.
 * Also checks for cycles in the dependency graph.
 */
tm_rule_list *topsort(const char *target, tm_rule_list *rules)
{
	tm_rule_list *sorted = NULL;
	tm_rule_list *rev = NULL;
	tm_rule *rule = find_rule(target, rules);

	if (rule == NULL)
		return NULL;
	
	if (topsort_visit(rule, rules, &sorted) < 0) {
		return NULL;
	}

	rev = rule_list_reverse(sorted);
	free_rule_list(sorted);
	return rev;
}


/* Return a string representation of a target list.
 * Consists of all the targets in the list separated by spaces.
 */
char *target_list_to_string(target_list *targets)
{
	target_list *node = targets;
	int len = 1;    /* for NUL terminator */
	char *str = NULL;
	char *p = NULL;

	if (!targets) {
		str = malloc(1);
		str[0] = '\0';
		return str;
	}

	for (node = targets; node; node = node->next) {
		len += strlen(node->name) + 1;
	}

	str = malloc(len);
	p = str;
	for (node = targets; node; node = node->next) {
		p += sprintf(p, "%s ", node->name);
	}
	*(p-1) = '\0';   /* get rid of the trailing space */

	return str;
}


/* Print a rule list to stdout.
 */
void print_rule_list(tm_rule_list *rules)
{
	tm_rule_list *node;
	
	printf("{\n");
	for (node = rules; node; node = node->next) {
		target_list *deps;

		printf("\t%s: ", node->rule->target);
		for (deps = node->rule->deps; deps; deps = deps->next) {
			printf("%s ", deps->name);
		}
		switch (node->rule->type) {
			case TM_EXPLICIT:
				printf("(explicit)");
				break;
			case TM_IMPLICIT:
				printf("(implicit)");
				break;
			case TM_FILENAME:
				printf("(filename)");
				break;
			default:
				printf("(UNKNOWN)");
				break;
		}
		if (node->rule->recipe) {
			printf("(recipe)");
		}
		printf("\n");
	}
	printf("}\n");
}

