#ifndef CROWNFALL_REPLAY_H
#define CROWNFALL_REPLAY_H

#include "engine.h"

CfGame *cf_replay_log_file(const char *path);
CfGame *cf_branch_from_log_file(const char *path, int turn_id);

#endif
