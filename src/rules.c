#include "rules.h"

#include <stdio.h>
#include <string.h>

bool cf_rules_can_capture(const CfGame *game, const CfPiece *attacker, const CfPiece *target) {
    int i;
    if (!game || !attacker || !target || !target->alive) return false;
    if (attacker->team == target->team) return false;
    if (target->type == CF_PIECE_KING) return false;
    if (attacker->type == CF_PIECE_PRINCE && target->type == CF_PIECE_PRINCE) return false;
    if (attacker->type == CF_PIECE_LOVE &&
        (target->type == CF_PIECE_KING || target->type == CF_PIECE_QUEEN || target->type == CF_PIECE_PRINCE)) return false;
    if (attacker->type == CF_PIECE_PRINCE && target->type == CF_PIECE_QUEEN) {
        for (i = 0; i < game->mercy_count; i++) {
            if (game->mercies[i].pact &&
                game->mercies[i].queen_id == target->id &&
                game->mercies[i].prince_id == attacker->id) return false;
        }
    }
    return true;
}

static int abs_i(int v) { return v < 0 ? -v : v; }

static int sign_i(int v) { return (v > 0) - (v < 0); }

static void pawn_forward_delta(const CfGame *game, int team, int *dx, int *dy) {
    *dx = 0;
    *dy = 0;
    if (game->config.team_count == 2) {
        *dy = team == 0 ? 1 : -1;
        return;
    }
    switch (team % 4) {
        case 0: *dy = 1; break;
        case 1: *dx = -1; break;
        case 2: *dy = -1; break;
        case 3: *dx = 1; break;
    }
}

static bool slide_controls(const CfGame *game, const CfPiece *piece, CfCoord target, int dx, int dy, int limit) {
    int dist;
    for (dist = 1; dist <= limit; dist++) {
        CfCoord c = {piece->pos.layer, piece->pos.x + dx * dist, piece->pos.y + dy * dist};
        int occ;
        if (!cf_board_is_playable(&game->board, c)) return false;
        if (c.layer == target.layer && c.x == target.x && c.y == target.y) return true;
        occ = cf_engine_piece_at(game, c);
        if (occ >= 0) return false;
    }
    return false;
}

static bool piece_controls_square(const CfGame *game, const CfPiece *piece, CfCoord target) {
    int dx, dy, adx, ady, i;
    int dirs8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    int knights[8][2] = {{1,2},{2,1},{-1,2},{-2,1},{1,-2},{2,-1},{-1,-2},{-2,-1}};
    if (!game || !piece || !piece->alive || piece->pos.layer != target.layer) return false;
    dx = target.x - piece->pos.x;
    dy = target.y - piece->pos.y;
    adx = abs_i(dx);
    ady = abs_i(dy);
    if (piece->type == CF_PIECE_KING || piece->type == CF_PIECE_LOVE) return adx <= 1 && ady <= 1 && (adx || ady);
    if (piece->type == CF_PIECE_KNIGHT) return (adx == 1 && ady == 2) || (adx == 2 && ady == 1);
    if (piece->type == CF_PIECE_PAWN) {
        int fx, fy;
        pawn_forward_delta(game, piece->team, &fx, &fy);
        if (fx) return dx == fx && ady == 1;
        return dy == fy && adx == 1;
    }
    if (piece->type == CF_PIECE_PRINCE) {
        if ((adx == 1 && ady == 2) || (adx == 2 && ady == 1)) return true;
        if ((dx == 0) != (dy == 0)) return slide_controls(game, piece, target, sign_i(dx), sign_i(dy), 7);
        return false;
    }
    if (piece->type == CF_PIECE_ROOK) {
        if ((dx == 0) == (dy == 0)) return false;
        return slide_controls(game, piece, target, sign_i(dx), sign_i(dy), 7);
    }
    if (piece->type == CF_PIECE_BISHOP) {
        if (adx != ady || adx == 0) return false;
        return slide_controls(game, piece, target, sign_i(dx), sign_i(dy), 7);
    }
    if (piece->type == CF_PIECE_QUEEN) {
        for (i = 0; i < 8; i++) {
            if (slide_controls(game, piece, target, dirs8[i][0], dirs8[i][1], 7)) return true;
        }
    }
    (void)knights;
    return false;
}

bool cf_rules_square_controlled_by_enemy(const CfGame *game, CfCoord square, int team) {
    int i;
    if (!game) return false;
    for (i = 0; i < game->piece_count; i++) {
        const CfPiece *p = &game->pieces[i];
        if (p->alive && p->team != team && piece_controls_square(game, p, square)) return true;
    }
    return false;
}

bool cf_rules_team_in_check(const CfGame *game, int team) {
    int i;
    if (!game) return false;
    for (i = 0; i < game->piece_count; i++) {
        const CfPiece *p = &game->pieces[i];
        if (p->alive && p->team == team && p->type == CF_PIECE_KING) {
            return cf_rules_square_controlled_by_enemy(game, p->pos, team);
        }
    }
    return false;
}

bool cf_rules_move_preserves_king_safety(CfGame *game, const CfMove *move) {
    int idx, cap;
    CfCoord old_pos;
    bool cap_alive = false;
    bool safe;
    if (!game || !move) return false;
    idx = cf_engine_piece_at(game, move->from);
    if (idx < 0) return false;
    cap = cf_engine_piece_at(game, move->to);
    if (cap >= 0 && !cf_rules_can_capture(game, &game->pieces[idx], &game->pieces[cap])) return false;
    old_pos = game->pieces[idx].pos;
    if (cap >= 0) {
        cap_alive = game->pieces[cap].alive;
        game->pieces[cap].alive = false;
    }
    game->pieces[idx].pos = move->to;
    safe = !cf_rules_team_in_check(game, game->pieces[idx].team);
    game->pieces[idx].pos = old_pos;
    if (cap >= 0) game->pieces[cap].alive = cap_alive;
    return safe;
}

bool cf_rules_repetition_legal(CfGame *game) {
    char key[512];
    int i, p, n = 0;
    if (!game) return true;
    n += snprintf(key + n, sizeof(key) - (size_t)n, "p%d|", game->current_player);
    for (p = 0; p < game->piece_count && n < (int)sizeof(key) - 16; p++) {
        const CfPiece *pc = &game->pieces[p];
        if (pc->alive) {
            n += snprintf(key + n, sizeof(key) - (size_t)n, "%d:%d:%d:%d;",
                          pc->id, pc->pos.layer, pc->pos.x, pc->pos.y);
        }
    }
    for (i = 0; i < game->repetition_len; i++) {
        if (strcmp(game->repetition[i], key) == 0) {
            game->repetition_count[i]++;
            return game->repetition_count[i] <= 3;
        }
    }
    if (game->repetition_len < CF_MAX_HISTORY) {
        strncpy(game->repetition[game->repetition_len], key, sizeof(game->repetition[0]) - 1);
        game->repetition_count[game->repetition_len] = 1;
        game->repetition_len++;
    }
    return true;
}
