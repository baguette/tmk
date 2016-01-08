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
	rule->mark = TM_UNMARKED;

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


int target_exists(char *target, tm_rule_list *rules)
{
	tm_rule_list *node = rules;

	while (node) {
		if (strcmp(node->rule->target, target) == 0) {
			return 1;
		}
		node = node->next;
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

tm_rule_list *find_rules(target_list *targets, tm_rule_list *rules)
{
	tm_rule_list *mapped = NULL;
	target_list *node = targets;

	while (node) {
		mapped = rule_cons(find_rule(node->name, rules), mapped);
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
		
		while (deps) {
			int n;

			n = topsort_visit(deps->rule, rules, sorted);

			if (n < 0)
				return n;
			deps = deps->next;
		}

		rule->mark = TM_PERMANENT;
		*sorted = rule_cons(rule, *sorted);
	}

	return 0;
}

tm_rule_list *rule_list_reverse(tm_rule_list *rules)
{
	tm_rule_list *rev = NULL;
	tm_rule_list *node = rules;

	while (node) {
		rev = rule_cons(node->rule, rev);
		node = node->next;
	}

	return rev;
}

target_list *target_list_reverse(target_list *targets)
{
	target_list *rev = NULL;
	target_list *node = targets;

	while (node) {
		rev = target_cons(node->name, rev);
		node = node->next;
	}

	return rev;
}

tm_rule_list *topsort(char *target, tm_rule_list *rules)
{
	tm_rule_list *sorted = NULL;
	tm_rule_list *rev = NULL;
	tm_rule *rule = find_rule(target, rules);

	if (rule == NULL)
		return NULL;
	
	for (;;) {
		int found = 0;
		tm_rule_list *node = rules;

		while (node) {
			if (node->rule->mark == TM_UNMARKED) {
				found = 1;
				topsort_visit(node->rule, rules, &sorted);
			}

			node = node->next;
		}

		if (!found)
			break;
	}

	rev = rule_list_reverse(sorted);
	free_rule_list(sorted);
	return rev;
}

char *target_copy(const char *target)
{
	int len = strlen(target);
	char *ret = malloc(len);

	strcpy(ret, target);

	return ret;
}

void print_rule_list(tm_rule_list *rules)
{
	tm_rule_list *node = rules;
	
	while (node) {
		printf("%s ", node->rule->target);
		node = node->next;
	}

	printf("\n");
}

