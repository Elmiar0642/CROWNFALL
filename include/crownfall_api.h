#ifndef CROWNFALL_API_H
#define CROWNFALL_API_H

#include <stdbool.h>

#include "../src/engine.h"

typedef void (*CfAgentCallback)(CfGame *game, int player_id);

CfGame *cf_new_game(CfConfig config);
void cf_free_game(CfGame *game);
const CfGame *cf_get_state(CfGame *game);
const char *cf_get_state_json(CfGame *game);
int cf_get_legal_moves(CfGame *game, int player_id, CfMove *moves, int max_moves);
const char *cf_get_legal_moves_json(CfGame *game, int player_id);
void cf_roll_dice(CfGame *game, unsigned int seed_optional);
bool cf_apply_move(CfGame *game, CfMove move);
bool cf_undo_move(CfGame *game);
bool cf_save_snapshot(CfGame *game, const char *path);
CfGame *cf_load_snapshot(const char *path);
CfGame *cf_replay_log(const char *path);
CfGame *cf_branch_from_log(const char *path, int turn_id);
CfGame *cf_branch_from_turn(const char *path, int turn_id);
void cf_register_agent_callback(int player_id, CfAgentCallback callback);

#endif
