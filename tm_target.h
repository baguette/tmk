#ifndef TM_TARGET_H
#define TM_TARGET_H

#define TM_UNMARKED 0
#define TM_TEMPORARY 1
#define TM_PERMANENT 2

struct target_list;

typedef struct tm_rule {
	char *target;
	struct target_list *deps;
	char *recipe;
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

tm_rule *new_rule(char *target, target_list *deps, char *recipe);

target_list *target_cons(char *name, target_list *next);
tm_rule_list *rule_cons(tm_rule *rule, tm_rule_list *next);

tm_rule *find_rule(char *name, tm_rule_list *rules);

tm_rule_list *topsort(char *target, tm_rule_list *rules);

char *target_copy(const char *target);

void free_rule(tm_rule *rule);
void free_target_list(target_list *targets);
void deep_free_target_list(target_list *targets);
void free_rule_list(tm_rule_list *rules);
void deep_free_rule_list(tm_rule_list *rules);

void print_rule_list(tm_rule_list *rules);

extern char *tm_goal;
extern tm_rule_list *tm_rules;

#endif
