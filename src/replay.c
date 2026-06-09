#include "replay.h"

#include <stdio.h>

CfGame *cf_replay_log_file(const char *path) {
    (void)path;
    /* TODO(replay): parse JSONL events and reconstruct state deterministically. */
    return NULL;
}

CfGame *cf_branch_from_log_file(const char *path, int turn_id) {
    (void)path;
    (void)turn_id;
    /* TODO(time-travel): replay through selected turn into a new session log. */
    return NULL;
}
