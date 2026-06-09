#include "api.h"

#include "dice.h"
#include "engine.h"
#include "replay.h"

#include <stdlib.h>

CfGame *cf_new_game(CfConfig config) { return cf_engine_new(&config); }
void cf_free_game(CfGame *game) { cf_engine_free(game); }
const CfGame *cf_get_state(CfGame *game) { return game; }
int cf_get_legal_moves(CfGame *game, int player_id, CfMove *moves, int max_moves) {
    int i, count = 0;
    if (!game || player_id < 0 || player_id >= game->player_count) return 0;
    for (i = 0; i < game->piece_count && count < max_moves; i++) {
        if (game->pieces[i].alive && game->pieces[i].team == game->players[player_id].team) {
            count += cf_engine_generate_moves(game, player_id, game->pieces[i].pos, moves + count, max_moves - count);
        }
    }
    return count;
}
void cf_roll_dice(CfGame *game, unsigned int seed_optional) {
    if (!game) return;
    if (seed_optional) cf_dice_seed(seed_optional);
    cf_roll_custom_dice(&game->die_a, &game->die_b, &game->dice_sum);
    game->dice_rolled = true;
}
bool cf_apply_move(CfGame *game, CfMove move) { char err[128]; return cf_engine_apply_move(game, &move, err, sizeof(err)); }
bool cf_undo_move(CfGame *game) { (void)game; return false; }
bool cf_save_snapshot(CfGame *game, const char *path) { return cf_engine_snapshot(game, path); }
CfGame *cf_load_snapshot(const char *path) { (void)path; return NULL; }
CfGame *cf_replay_log(const char *path) { return cf_replay_log_file(path); }
CfGame *cf_branch_from_log(const char *path, int turn_id) { return cf_branch_from_log_file(path, turn_id); }
void cf_register_agent_callback(int player_id, CfAgentCallback callback) { (void)player_id; (void)callback; }
