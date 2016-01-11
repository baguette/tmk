#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tm_target.h"
#include "tm_core_cmds.h"

char *tm_goal = NULL;
tm_rule_list *tm_rules = NULL;

tm_rule *new_rule(char *target, target_list *deps, char *recipe)
{
	tm_rule *rule = malloc(sizeof(tm_rule));
	
	rule->target = target;
	rule->deps = deps;
	rule->recipe = recipe;
	rule->type = TM_EXPLICIT;
	rule->mark = TM_UNMARKED;

	return rule;
}

tm_rule *new_filename(char *target)
{
	tm_rule *rule = new_rule(target, NULL, NULL);

	rule->type = TM_FILENAME;

	return rule;
}

void free_rule(tm_rule *rule)
{
	free(rule->target);
	free_target_list(rule->deps);
	free(rule->recipe);
	free(rule);
}

target_list *target_cons(char *name, target_list *next)
{
	target_list *targets = malloc(sizeof(target_list));

	targets->name = name;
	targets->next = next;

	return targets;
}

void deep_free_target_list(target_list *targets)
{
	if (targets == NULL)
		return;

	free(targets->name);
	free_target_list(targets->next);
	free(targets);
}

void free_target_list(target_list *targets)
{
	if (targets == NULL)
		return;
	
	free_target_list(targets->next);
	free(targets);
}

tm_rule_list *rule_cons(tm_rule *rule, tm_rule_list *next)
{
	tm_rule_list *rules = malloc(sizeof(tm_rule_list));

	rules->rule = rule;
	rules->next = next;
	
	return rules;
}

void deep_free_rule_list(tm_rule_list *rules)
{
	if (rules == NULL)
		return;
	
	free_rule(rules->rule);
	free_rule_list(rules->next);
	free(rules);
}

void free_rule_list(tm_rule_list *rules)
{
	if (rules == NULL)
		return;
	
	free_rule_list(rules->next);
	free(rules);
}


int target_exists(char *target, target_list *targets)
{
	for (; targets; targets = targets->next) {
		if (strcmp(targets->name, target) == 0) {
			return 1;
		}
	}

	return 0;
}


tm_rule *find_rule(char *target, tm_rule_list *rules)
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

tm_rule *find_rule_or_file(char *target, tm_rule_list *rules)
{
	tm_rule *rule = find_rule(target, rules);
	
	if (!rule) {
		rule = new_filename(target);
	}

	return rule;
}

tm_rule_list *find_rules(target_list *targets, tm_rule_list *rules)
{
	tm_rule_list *mapped = NULL;
	target_list *node = targets;

	while (node) {
		mapped = rule_cons(find_rule_or_file(node->name, rules), mapped);
		node = node->next;
	}

	return mapped;
}


/* Tarjan's algorithm */
static int topsort_visit(tm_rule *rule, tm_rule_list *rules, tm_rule_list **sorted)
{
	if (rule->mark == TM_TEMPORARY) {
		fprintf(stderr, "ERROR:  Cycle detected in dependency graph\n");
		return -1;
	}

	if (rule->mark == TM_UNMARKED) {
		tm_rule_list *deps = NULL;

		rule->mark = TM_TEMPORARY;
		deps = find_rules(rule->deps, rules);
		
		for (; deps; deps = deps->next) {
			int n;

			n = topsort_visit(deps->rule, rules, sorted);

			if (n < 0) {
				free_rule_list(deps);
				return n;
			}
		}

		rule->mark = TM_PERMANENT;
		*sorted = rule_cons(rule, *sorted);
		free_rule_list(deps);
	}

	return 0;
}

tm_rule_list *rule_list_reverse(tm_rule_list *rules)
{
	tm_rule_list *rev = NULL;
	tm_rule_list *node = rules;

	for (; node; node = node->next) {
		rev = rule_cons(node->rule, rev);
	}

	return rev;
}

target_list *target_list_reverse(target_list *targets)
{
	target_list *rev = NULL;
	target_list *node = targets;

	for (; node; node = node->next) {
		rev = target_cons(node->name, rev);
	}

	return rev;
}

tm_rule_list *topsort(char *target, tm_rule_list *rules)
{
	tm_rule_list *sorted = NULL;
	tm_rule_list *rev = NULL;
	tm_rule *rule = find_rule_or_file(target, rules);

	if (rule == NULL)
		return NULL;
	
	topsort_visit(rule, rules, &sorted);

	rev = rule_list_reverse(sorted);
	free_rule_list(sorted);
	return rev;
}

char *target_copy(const char *target)
{
	int len = strlen(target);
	char *ret = malloc(len + 1);

	strcpy(ret, target);

	return ret;
}

target_list *get_targets(tm_rule_list *rules)
{
	target_list *targets = NULL;
	tm_rule_list *node = NULL;

	for (node = rules; node; node = node->next) {
		targets = target_cons(node->rule->target, targets);
	}

	return targets;
}

char *target_list_to_string(target_list *targets)
{
	target_list *node = targets;
	int len = 0;
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

