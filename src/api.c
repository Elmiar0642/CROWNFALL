#include "api.h"

#include "dice.h"
#include "engine.h"
#include "replay.h"

#include <stdio.h>
#include <stdlib.h>

CfGame *cf_new_game(CfConfig config) { return cf_engine_new(&config); }
void cf_free_game(CfGame *game) { cf_engine_free(game); }
const CfGame *cf_get_state(CfGame *game) { return game; }
const char *cf_get_state_json(CfGame *game) {
    static char json[32768];
    int i, n = 0;
    if (!game) return "{}";
    n += snprintf(json + n, sizeof(json) - (size_t)n,
                  "{\"session_id\":\"%s\",\"turn_id\":%d,\"team_count\":%d,\"current_player\":%d,\"dice\":{\"a\":%d,\"b\":%d,\"sum\":%d,\"rolled\":%s},\"houses\":[",
                  game->session_id, game->turn_id, game->config.team_count, game->current_player,
                  game->die_a, game->die_b, game->dice_sum, game->dice_rolled ? "true" : "false");
    for (i = 0; i < game->house_count && n < (int)sizeof(json) - 256; i++) {
        CfHouse *h = &game->houses[i];
        n += snprintf(json + n, sizeof(json) - (size_t)n,
                      "%s{\"team_id\":%d,\"house_name\":\"%s\",\"motto\":\"%s\",\"court\":\"%s\",\"layer\":%d}",
                      i ? "," : "", h->team_id, h->house_name, h->motto, h->default_court, h->default_layer + 1);
    }
    n += snprintf(json + n, sizeof(json) - (size_t)n, "],\"pieces\":[");
    for (i = 0; i < game->piece_count && n < (int)sizeof(json) - 128; i++) {
        CfPiece *p = &game->pieces[i];
        if (!p->alive) continue;
        n += snprintf(json + n, sizeof(json) - (size_t)n,
                      "%s{\"id\":%d,\"team_id\":%d,\"house_name\":\"%s\",\"label\":\"%s\",\"role\":\"%s\",\"identity\":\"%s\",\"layer\":%d,\"x\":%d,\"y\":%d}",
                      n && json[n - 1] != '[' ? "," : "", p->id, p->team + 1, game->houses[p->team].house_name,
                      p->label, cf_piece_role_name(p->role), p->identity, p->pos.layer, p->pos.x, p->pos.y);
    }
    snprintf(json + n, sizeof(json) - (size_t)n, "]}");
    return json;
}
int cf_get_active_houses_api(CfGame *game, const CfHouse **out, int max) { return cf_get_active_houses(game, out, max); }
const char *cf_get_piece_identity_api(CfGame *game, int piece_id) { return cf_get_piece_identity(game, piece_id); }
const char *cf_get_player_identity_api(CfGame *game, int player_id) { return cf_get_player_identity(game, player_id); }
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
const char *cf_get_legal_moves_json(CfGame *game, int player_id) {
    static char json[8192];
    CfMove moves[CF_MAX_MOVES];
    int i, n = 0;
    int count = cf_get_legal_moves(game, player_id, moves, CF_MAX_MOVES);
    n += snprintf(json + n, sizeof(json) - (size_t)n, "[");
    for (i = 0; i < count && n < (int)sizeof(json) - 64; i++) {
        n += snprintf(json + n, sizeof(json) - (size_t)n, "%s\"%s\"", i ? "," : "", moves[i].notation);
    }
    snprintf(json + n, sizeof(json) - (size_t)n, "]");
    return json;
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
CfGame *cf_branch_from_turn(const char *path, int turn_id) { return cf_branch_from_log_file(path, turn_id); }
void cf_register_agent_callback(int player_id, CfAgentCallback callback) { (void)player_id; (void)callback; }
