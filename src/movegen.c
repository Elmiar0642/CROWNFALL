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

static int team_forward(const CfGame *game, int team) {
    if (game->config.team_count == 2) return team == 0 ? 1 : -1;
    if (team == 0 || team == 1) return 1;
    if (team == 2 || team == 3) return -1;
    return team % 2 == 0 ? 1 : -1;
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
        int f = team_forward(game, piece->team);
        CfCoord one = {piece->pos.layer, piece->pos.x, piece->pos.y + f};
        if (cf_board_is_playable(&game->board, one) && cf_engine_piece_at(game, one) < 0) {
            count += add_move(game, piece, one, moves + count, max_moves - count);
            if ((piece->team == 0 && piece->pos.y <= 1) || (piece->team != 0 && piece->pos.y >= game->board.size - 2)) {
                CfCoord two = {piece->pos.layer, piece->pos.x, piece->pos.y + 2 * f};
                if (cf_board_is_playable(&game->board, two) && cf_engine_piece_at(game, two) < 0) {
                    count += add_move(game, piece, two, moves + count, max_moves - count);
                }
            }
        }
        for (i = -1; i <= 1; i += 2) {
            CfCoord cap = {piece->pos.layer, piece->pos.x + i, piece->pos.y + f};
            int occ = cf_engine_piece_at(game, cap);
            if (occ >= 0 && game->pieces[occ].team != piece->team) {
                count += add_move(game, piece, cap, moves + count, max_moves - count);
            }
        }
    }
    return count;
}
