#include "engine.h"

#include "dice.h"
#include "log.h"
#include "movegen.h"
#include "rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *cf_piece_type_name(CfPieceType type) {
    switch (type) {
        case CF_PIECE_KING: return "King";
        case CF_PIECE_QUEEN: return "Queen";
        case CF_PIECE_PRINCE: return "Prince";
        case CF_PIECE_LOVE: return "Love Interest";
        case CF_PIECE_ROOK: return "Rook";
        case CF_PIECE_KNIGHT: return "Knight";
        case CF_PIECE_BISHOP: return "Bishop";
        case CF_PIECE_PAWN: return "Pawn";
        default: return "None";
    }
}

static CfPiece *add_piece(CfGame *g, int team, CfPieceType type, const char *label, int layer, int x, int y) {
    CfPiece *p;
    if (g->piece_count >= CF_MAX_PIECES) return NULL;
    p = &g->pieces[g->piece_count];
    memset(p, 0, sizeof(*p));
    p->id = g->piece_count;
    p->team = team;
    p->type = type;
    strncpy(p->label, label, sizeof(p->label) - 1);
    p->pos.layer = layer;
    p->pos.x = x;
    p->pos.y = y;
    p->alive = cf_board_is_playable(&g->board, p->pos);
    g->piece_count++;
    return p;
}

static void setup_players(CfGame *g) {
    int t;
    const char *roles[3] = {"King", "Left House", "Right House"};
    for (t = 0; t < g->config.team_count; t++) {
        int r;
        for (r = 0; r < 3; r++) {
            CfPlayer *p = &g->players[g->player_count];
            p->player_id = g->player_count;
            p->team = t;
            strncpy(p->role, roles[r], sizeof(p->role) - 1);
            if (g->config.player_names[p->player_id][0]) {
                strncpy(p->name, g->config.player_names[p->player_id], sizeof(p->name) - 1);
            } else {
                snprintf(p->name, sizeof(p->name), "Team%d %s", t + 1, roles[r]);
            }
            g->player_count++;
        }
    }
}

static void setup_team(CfGame *g, int team, int layer, int base_y) {
    int dir = base_y < g->board.size / 2 ? 1 : -1;
    int back = base_y;
    int pawns = base_y + dir;
    int cx = g->board.size / 2;
    add_piece(g, team, CF_PIECE_ROOK, "R1", layer, cx - 4, back);
    add_piece(g, team, CF_PIECE_KNIGHT, "N1", layer, cx - 3, back);
    add_piece(g, team, CF_PIECE_BISHOP, "B1", layer, cx - 2, back);
    add_piece(g, team, CF_PIECE_QUEEN, "Q1", layer, cx - 1, back);
    add_piece(g, team, CF_PIECE_KING, "K", layer, cx, back);
    add_piece(g, team, CF_PIECE_QUEEN, "Q2", layer, cx + 1, back);
    add_piece(g, team, CF_PIECE_BISHOP, "B2", layer, cx + 2, back);
    add_piece(g, team, CF_PIECE_KNIGHT, "N2", layer, cx + 3, back);
    add_piece(g, team, CF_PIECE_ROOK, "R2", layer, cx + 4, back);
    add_piece(g, team, CF_PIECE_PRINCE, "P1", layer, cx - 1, pawns);
    add_piece(g, team, CF_PIECE_LOVE, "L", layer, cx, pawns);
    add_piece(g, team, CF_PIECE_PRINCE, "P2", layer, cx + 1, pawns);
    add_piece(g, team, CF_PIECE_PAWN, "p1", layer, cx - 3, pawns);
    add_piece(g, team, CF_PIECE_PAWN, "p2", layer, cx - 2, pawns);
    add_piece(g, team, CF_PIECE_PAWN, "p3", layer, cx - 1, pawns + dir);
    add_piece(g, team, CF_PIECE_PAWN, "p4", layer, cx + 1, pawns + dir);
    add_piece(g, team, CF_PIECE_PAWN, "p5", layer, cx + 2, pawns);
    add_piece(g, team, CF_PIECE_PAWN, "p6", layer, cx + 3, pawns);
}

static void setup_pieces(CfGame *g) {
    setup_team(g, 0, 0, 0);
    setup_team(g, 1, 0, g->board.size - 1);
    if (g->config.team_count >= 4) {
        setup_team(g, 2, 0, 3);
        setup_team(g, 3, 0, g->board.size - 4);
    }
    if (g->config.team_count == 8) {
        setup_team(g, 4, 1, 0);
        setup_team(g, 5, 1, g->board.size - 1);
        setup_team(g, 6, 1, 3);
        setup_team(g, 7, 1, g->board.size - 4);
    }
}

CfGame *cf_engine_new(const CfConfig *config) {
    CfGame *g = calloc(1, sizeof(*g));
    char fields[256];
    if (!g) return NULL;
    if (config) g->config = *config;
    if (g->config.team_count == 0) g->config.team_count = 2;
    cf_board_init(&g->board, g->config.team_count);
    cf_dice_seed(0);
    setup_players(g);
    setup_pieces(g);
    cf_log_open(g);
    snprintf(fields, sizeof(fields), "\"session_id\":\"%s\"", g->session_id);
    cf_log_event(g, "session_start", fields);
    snprintf(fields, sizeof(fields), "\"team_count\":%d,\"time_travel\":%s,\"playable_squares\":%d",
             g->config.team_count, g->config.time_travel ? "true" : "false", cf_board_playable_count(&g->board));
    cf_log_event(g, "config", fields);
    cf_engine_start_turn(g);
    return g;
}

void cf_engine_free(CfGame *game) {
    if (!game) return;
    cf_log_event(game, "session_end", "\"reason\":\"normal\"");
    cf_log_close(game);
    free(game);
}

void cf_engine_start_turn(CfGame *game) {
    char fields[256];
    CfPlayer *p;
    if (!game || game->player_count == 0) return;
    p = &game->players[game->current_player];
    game->dice_rolled = false;
    game->die_a = game->die_b = game->dice_sum = 0;
    snprintf(fields, sizeof(fields), "\"player_id\":%d,\"player\":\"%s\",\"team\":%d,\"role\":\"%s\"",
             p->player_id, p->name, p->team, p->role);
    cf_log_event(game, "turn_start", fields);
}

int cf_engine_piece_at(const CfGame *game, CfCoord c) {
    int i;
    if (!game || !cf_board_is_playable(&game->board, c)) return -1;
    for (i = 0; i < game->piece_count; i++) {
        const CfPiece *p = &game->pieces[i];
        if (p->alive && p->pos.layer == c.layer && p->pos.x == c.x && p->pos.y == c.y) return i;
    }
    return -1;
}

int cf_engine_generate_moves(CfGame *game, int player_id, CfCoord from, CfMove *moves, int max_moves) {
    int idx;
    char sq[16], fields[128];
    if (!game || player_id < 0 || player_id >= game->player_count) return 0;
    idx = cf_engine_piece_at(game, from);
    if (idx < 0 || game->pieces[idx].team != game->players[player_id].team) return 0;
    idx = cf_movegen_for_piece(game, &game->pieces[idx], moves, max_moves);
    cf_coord_to_string(&game->board, from, sq, sizeof(sq));
    snprintf(fields, sizeof(fields), "\"player_id\":%d,\"square\":\"%s\",\"count\":%d", player_id, sq, idx);
    cf_log_event(game, "legal_moves_generated", fields);
    return idx;
}

static void handle_capture(CfGame *g, CfPiece *attacker, CfPiece *target) {
    char fields[256];
    target->alive = false;
    snprintf(fields, sizeof(fields), "\"attacker\":%d,\"captured\":%d,\"piece\":\"%s\"",
             attacker->id, target->id, target->label);
    cf_log_event(g, "capture", fields);
    if (target->type == CF_PIECE_PRINCE) {
        g->bloodfall[target->team] = true;
        snprintf(fields, sizeof(fields), "\"team\":%d,\"prince_id\":%d", target->team, target->id);
        cf_log_event(g, "bloodfall", fields);
        cf_log_event(g, "widow_freeze", fields);
    }
}

bool cf_engine_apply_move(CfGame *game, const CfMove *move, char *err, int err_size) {
    int idx, cap;
    char a[16], b[16], fields[256];
    if (!game || !move) return false;
    idx = cf_engine_piece_at(game, move->from);
    if (idx < 0) {
        snprintf(err, (size_t)err_size, "no piece at source");
        return false;
    }
    cap = cf_engine_piece_at(game, move->to);
    cf_coord_to_string(&game->board, move->from, a, sizeof(a));
    cf_coord_to_string(&game->board, move->to, b, sizeof(b));
    snprintf(fields, sizeof(fields), "\"from\":\"%s\",\"to\":\"%s\",\"piece\":%d", a, b, game->pieces[idx].id);
    cf_log_event(game, "move_attempt", fields);
    if (cap >= 0 && !cf_rules_can_capture(game, &game->pieces[idx], &game->pieces[cap])) {
        snprintf(err, (size_t)err_size, "capture blocked by royal rule");
        return false;
    }
    if (cap >= 0) handle_capture(game, &game->pieces[idx], &game->pieces[cap]);
    game->pieces[idx].pos = move->to;
    cf_log_event(game, "move_success", fields);
    if (!cf_rules_repetition_legal(game)) {
        snprintf(err, (size_t)err_size, "fourth repetition is illegal");
        return false;
    }
    game->turn_id++;
    cf_engine_snapshot(game, "logs/snapshots/latest.json");
    cf_engine_next_turn(game);
    return true;
}

void cf_engine_next_turn(CfGame *game) {
    if (!game || game->player_count == 0) return;
    game->current_player = (game->current_player + 1) % game->player_count;
    cf_engine_start_turn(game);
}

void cf_engine_print_board(const CfGame *game, FILE *out) {
    int l, y, x;
    if (!game) return;
    for (l = 0; l < game->board.layers; l++) {
        if (game->board.layers > 1) fprintf(out, "Layer L%d\n", l + 1);
        for (y = game->board.size - 1; y >= 0; y--) {
            fprintf(out, "%2d ", y + 1);
            for (x = 0; x < game->board.size; x++) {
                CfCoord c = {l, x, y};
                int p = cf_engine_piece_at(game, c);
                if (!cf_board_is_playable(&game->board, c)) fprintf(out, " ##");
                else if (p >= 0) fprintf(out, " %c%d", game->pieces[p].label[0], game->pieces[p].team + 1);
                else fprintf(out, " ..");
            }
            fprintf(out, "\n");
        }
        fprintf(out, "   ");
        for (x = 0; x < game->board.size; x++) fprintf(out, " %c ", 'a' + x);
        fprintf(out, "\n\n");
    }
}

void cf_engine_print_state(const CfGame *game, FILE *out) {
    const CfPlayer *p;
    if (!game) return;
    p = &game->players[game->current_player];
    fprintf(out, "Session: %s\nTurn: %d\nPlayer: %s (team %d, %s)\nDice: %d + %d = %d%s\nLog: %s\n",
            game->session_id, game->turn_id, p->name, p->team + 1, p->role,
            game->die_a, game->die_b, game->dice_sum, game->dice_rolled ? "" : " (not rolled)", game->log_path);
    if (game->board.layout == CF_LAYOUT_6T_WIP) fprintf(out, "Notice: 6-team dynamic layout is WIP; using experimental cross board.\n");
}

bool cf_engine_snapshot(CfGame *game, const char *path) {
    FILE *f;
    int i;
    if (!game || !path) return false;
    f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "{\"session_id\":\"%s\",\"turn_index\":%d,\"current_player\":%d,\"dice\":{\"a\":%d,\"b\":%d,\"sum\":%d,\"rolled\":%s},\"board\":[",
            game->session_id, game->turn_id, game->current_player, game->die_a, game->die_b,
            game->dice_sum, game->dice_rolled ? "true" : "false");
    for (i = 0; i < game->piece_count; i++) {
        CfPiece *p = &game->pieces[i];
        fprintf(f, "%s{\"id\":%d,\"team\":%d,\"type\":\"%s\",\"label\":\"%s\",\"alive\":%s,\"layer\":%d,\"x\":%d,\"y\":%d}",
                i ? "," : "", p->id, p->team, cf_piece_type_name(p->type), p->label,
                p->alive ? "true" : "false", p->pos.layer, p->pos.x, p->pos.y);
    }
    fprintf(f, "],\"pacts\":[],\"mercy_counts\":[],\"bloodfall\":[");
    for (i = 0; i < game->config.team_count; i++) fprintf(f, "%s%s", i ? "," : "", game->bloodfall[i] ? "true" : "false");
    fprintf(f, "],\"love_interest_states\":[],\"repetition_table\":%d}\n", game->repetition_len);
    fclose(f);
    return true;
}

bool cf_engine_branch(CfGame *game, int turn_id, char *out_path, int out_size) {
    char fields[128];
    if (!game || !game->config.time_travel) return false;
    snprintf(fields, sizeof(fields), "\"parent_session_id\":\"%s\",\"branch_from_turn\":%d", game->session_id, turn_id);
    cf_log_event(game, "branch_created", fields);
    snprintf(out_path, (size_t)out_size, "logs/branch_from_%s_turn_%d.jsonl", game->session_id, turn_id);
    return true;
}
