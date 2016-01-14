#ifndef TM_TARGET_H
#define TM_TARGET_H

#include "tmake.h"

#define TM_EXPLICIT 0
#define TM_IMPLICIT 1
#define TM_FILENAME 2
#define TM_UPDATED  3

#define TM_UNMARKED  0
#define TM_TEMPORARY 1
#define TM_PERMANENT 2

struct target_list;

typedef struct tm_rule {
	char *target;
	struct target_list *deps;
	char *recipe;
	unsigned char type;
	unsigned char mark;
} tm_rule;

typedef struct target_list {
	char *name;
	struct target_list *next;
} target_list;

typedef struct tm_rule_list {
	struct tm_rule *rule;
	struct tm_rule_list *next;
} tm_rule_list;

tm_rule *new_rule(const char *target, target_list *deps, const char *recipe);
tm_rule *new_filename(const char *target);

target_list *target_cons(const char *name, target_list *next);
tm_rule_list *rule_cons(tm_rule *rule, tm_rule_list *next);

target_list *target_list_copy(target_list *targets);

int target_exists(const char *target, target_list *targets);
tm_rule *find_rule(const char *name, tm_rule_list *rules);
tm_rule_list *find_rules(target_list *targets, tm_rule_list *rules);
void find_files(tm_rule_list **rules);

tm_rule_list *topsort(const char *target, tm_rule_list *rules);

tm_rule_list *rule_list_reverse(tm_rule_list *rules);
target_list *target_list_reverse(target_list *targets);

void free_rule(tm_rule *rule);
void free_target_list(target_list *targets);
void free_rule_list(tm_rule_list *rules);

char *target_list_to_string(target_list *targets);
void print_rule_list(tm_rule_list *rules);

extern char *tm_goal;
extern tm_rule_list *tm_rules;

#endif
