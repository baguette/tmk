#ifndef TM_UPDATE_H
#define TM_UPDATE_H

#include <sqlite3.h>

#define JIM_EMBEDDED
#include <jim.h>

#include "tm_target.h"

extern target_list *updated_targets;


int file_exists(const char *filename);

void wrap(Jim_Interp *interp, int error);

int update(sqlite3 *db, const char *tmfile, const char *target);
int was_updated(const char *target);
int needs_update(sqlite3 *db, const char *tmfile, const char *target);
target_list *need_update(sqlite3 *db, const char *tmfile, target_list *targets);

void update_rules(sqlite3 *db,
                  Jim_Interp *interp,
                  const char *tmfile,
                  tm_rule_list *sorted_rules,
                  int force,
                  int silence);

#endif
