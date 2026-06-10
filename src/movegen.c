#include "movegen.h"

#include "rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int add_move(CfGame *game, const CfPiece *piece, CfCoord to, CfMove *moves, int max_moves) {
    int cap;
    char a[16], b[16];
    if (!cf_board_is_playable(&game->board, to) || max_moves <= 0) return 0;
    cap = cf_engine_piece_at(game, to);
    if (cap >= 0) {
        if (!cf_rules_can_capture(game, piece, &game->pieces[cap])) return 0;
    }
    moves[0].from = piece->pos;
    moves[0].to = to;
    moves[0].piece_id = piece->id;
    moves[0].capture_id = cap >= 0 ? game->pieces[cap].id : -1;
    cf_coord_to_string(&game->board, piece->pos, a, sizeof(a));
    cf_coord_to_string(&game->board, to, b, sizeof(b));
    snprintf(moves[0].notation, sizeof(moves[0].notation), "%s-%s", a, b);
    return 1;
}

static int slide(CfGame *game, const CfPiece *piece, int dx, int dy, CfMove *moves, int max_moves) {
    int count = 0;
    int dist;
    int limit = game->dice_rolled ? game->dice_sum : 1;
    for (dist = 1; dist <= limit && count < max_moves; dist++) {
        CfCoord to = {piece->pos.layer, piece->pos.x + dx * dist, piece->pos.y + dy * dist};
        int occ;
        if (!cf_board_is_playable(&game->board, to)) break;
        occ = cf_engine_piece_at(game, to);
        if (occ >= 0) {
            count += add_move(game, piece, to, moves + count, max_moves - count);
            break;
        }
        count += add_move(game, piece, to, moves + count, max_moves - count);
    }
    return count;
}

static void team_forward_delta(const CfGame *game, int team, int *dx, int *dy) {
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

static bool pawn_on_start_row(const CfGame *game, const CfPiece *piece) {
    if (game->config.team_count == 2) return (piece->team == 0 && piece->pos.y == 1) || (piece->team == 1 && piece->pos.y == 7);
    switch (piece->team % 4) {
        case 0: return piece->pos.y == 1;
        case 1: return piece->pos.x == 13;
        case 2: return piece->pos.y == 13;
        case 3: return piece->pos.x == 1;
    }
    return false;
}

int cf_movegen_for_piece(CfGame *game, const CfPiece *piece, CfMove *moves, int max_moves) {
    int count = 0;
    int dirs8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    int knights[8][2] = {{1,2},{2,1},{-1,2},{-2,1},{1,-2},{2,-1},{-1,-2},{-2,-1}};
    int i;
    if (!game || !piece || !piece->alive || piece->frozen || !moves || max_moves <= 0) return 0;

    if (piece->type == CF_PIECE_KING || piece->type == CF_PIECE_LOVE) {
        for (i = 0; i < 8 && count < max_moves; i++) {
            CfCoord to = {piece->pos.layer, piece->pos.x + dirs8[i][0], piece->pos.y + dirs8[i][1]};
            count += add_move(game, piece, to, moves + count, max_moves - count);
        }
    } else if (piece->type == CF_PIECE_QUEEN) {
        for (i = 0; i < 8 && count < max_moves; i++) count += slide(game, piece, dirs8[i][0], dirs8[i][1], moves + count, max_moves - count);
    } else if (piece->type == CF_PIECE_ROOK) {
        for (i = 0; i < 4 && count < max_moves; i++) count += slide(game, piece, dirs8[i][0], dirs8[i][1], moves + count, max_moves - count);
    } else if (piece->type == CF_PIECE_BISHOP) {
        for (i = 4; i < 8 && count < max_moves; i++) count += slide(game, piece, dirs8[i][0], dirs8[i][1], moves + count, max_moves - count);
    } else if (piece->type == CF_PIECE_PRINCE) {
        for (i = 0; i < 4 && count < max_moves; i++) count += slide(game, piece, dirs8[i][0], dirs8[i][1], moves + count, max_moves - count);
        for (i = 0; i < 8 && count < max_moves; i++) {
            CfCoord to = {piece->pos.layer, piece->pos.x + knights[i][0], piece->pos.y + knights[i][1]};
            count += add_move(game, piece, to, moves + count, max_moves - count);
        }
    } else if (piece->type == CF_PIECE_KNIGHT) {
        for (i = 0; i < 8 && count < max_moves; i++) {
            CfCoord to = {piece->pos.layer, piece->pos.x + knights[i][0], piece->pos.y + knights[i][1]};
            count += add_move(game, piece, to, moves + count, max_moves - count);
        }
    } else if (piece->type == CF_PIECE_PAWN) {
        int fx, fy;
        team_forward_delta(game, piece->team, &fx, &fy);
        CfCoord one = {piece->pos.layer, piece->pos.x + fx, piece->pos.y + fy};
        if (cf_board_is_playable(&game->board, one) && cf_engine_piece_at(game, one) < 0) {
            count += add_move(game, piece, one, moves + count, max_moves - count);
            if (pawn_on_start_row(game, piece)) {
                CfCoord two = {piece->pos.layer, piece->pos.x + 2 * fx, piece->pos.y + 2 * fy};
                if (cf_board_is_playable(&game->board, two) && cf_engine_piece_at(game, two) < 0) {
                    count += add_move(game, piece, two, moves + count, max_moves - count);
                }
            }
        }
        for (i = -1; i <= 1; i += 2) {
            CfCoord cap = fx ? (CfCoord){piece->pos.layer, piece->pos.x + fx, piece->pos.y + i}
                             : (CfCoord){piece->pos.layer, piece->pos.x + i, piece->pos.y + fy};
            int occ = cf_engine_piece_at(game, cap);
            if (occ >= 0 && game->pieces[occ].team != piece->team) {
                count += add_move(game, piece, cap, moves + count, max_moves - count);
            }
        }
    }
    return count;
}
