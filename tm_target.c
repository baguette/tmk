#include <stdio.h>
#include <string.h>

#include "tm_target.h"

tm_rule *new_rule(const char *target, target_list *deps, const char *recipe)
{
	tm_rule *rule = malloc(sizeof(tm_rule));
	
	rule->target = target;
	rule->deps = deps;
	rule->recipe = recipe;

	return rule;
}

void free_rule(tm_rule *rule)
{
	free(rule->target);
	free_target_list(rule->deps);
	free(rule->recipe);
	free(rule);
}

target_list *target_cons(const char *name, target_list *next)
{
	target_list *targets = malloc(sizeof(target_list));

	targets->name = name;
	targets->next = next;

	return targets;
}

void free_target_list(target_list *targets)
{
	if (targets == NULL)
		return;

	free(targets->name);
	free_target_list(targets->next);
}

tm_rule_list *rule_cons(tm_rule *rule, tm_rule_list *next)
{
	tm_rule_list *rules = malloc(sizeof(tm_rule_list));

	rules->rule = rule;
	rules->next = next;
	
	return rules;
}

void free_rule_list(tm_rule_list *rules)
{
	if (rules == NULL)
		return;
	
	free_rule(rules->rule);
	free_rule_list(rules->next);
}

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
int topsort_visit(tm_rule rule, tm_rule_list *rules, tm_rule_list **sorted)
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
			int n = topsort_visit(deps->rule, rules, sorted);
			if (n < 0)
				return n;
			deps = deps->next;
		}

		rule->mark = TM_PERMANENT;
		*sorted = rule_cons(rule, *sorted);
	}

	return 0;
}

tm_rule_list *topsort(const char *target, tm_rule_list *rules)
{
	tm_rule_list *sorted = NULL;
	tm_rule rule = find_rule(target, rules);

	if (rule == NULL)
		return NULL;
	
	for (;;) {
		int found = 0;
		tm_rule_list *node = rules;

		while (node) {
			if (node->rule->mark = TM_UNMARKED) {
				found = 1;
				topsort_visit(node->rule, rules, &sorted);
			}

			node = node->next;
		}

		if (!found)
			break;
	}

	return sorted;
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

