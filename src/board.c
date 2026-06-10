#include "board.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void clear_board(CfBoard *board) {
    int l, y, x;
    for (l = 0; l < CF_MAX_LAYERS; l++) {
        for (y = 0; y < CF_MAX_SIZE; y++) {
            for (x = 0; x < CF_MAX_SIZE; x++) {
                board->playable[l][y][x] = false;
            }
        }
    }
}

void cf_board_init(CfBoard *board, int team_count) {
    int l, y, x;
    clear_board(board);
    board->layers = 1;
    board->size = 9;
    board->layout = CF_LAYOUT_2T;

    if (team_count == 4 || team_count == 6 || team_count == 8) {
        board->size = 15;
        board->layout = (team_count == 6) ? CF_LAYOUT_6T_WIP : CF_LAYOUT_4T;
    }
    if (team_count == 8) {
        board->layers = 2;
        board->layout = CF_LAYOUT_8T;
    }

    for (l = 0; l < board->layers; l++) {
        for (y = 0; y < board->size; y++) {
            for (x = 0; x < board->size; x++) {
                bool corner = false;
                if (board->size == 15) {
                    corner = (x < 3 && y < 3) || (x >= 12 && y < 3) ||
                             (x < 3 && y >= 12) || (x >= 12 && y >= 12);
                }
                board->playable[l][y][x] = !corner;
            }
        }
    }
}

void board_init_2team(CfBoard *board) {
    cf_board_init(board, 2);
}

void board_init_4team(CfBoard *board) {
    cf_board_init(board, 4);
}

void board_init_8team(CfBoard *board) {
    cf_board_init(board, 8);
}

bool board_is_valid_square(const CfBoard *board, CfCoord c) {
    return cf_board_is_playable(board, c);
}

void board_rotate_template(int court_direction, int col, int row, int *x, int *y) {
    if (!x || !y) return;
    switch (court_direction) {
        case 1:
            *x = 14 - row;
            *y = 3 + col;
            break;
        case 2:
            *x = 3 + col;
            *y = 14 - row;
            break;
        case 3:
            *x = row;
            *y = 3 + col;
            break;
        default:
            *x = 3 + col;
            *y = row;
            break;
    }
}

bool cf_board_is_playable(const CfBoard *board, CfCoord c) {
    if (!board || c.layer < 0 || c.layer >= board->layers) return false;
    if (c.x < 0 || c.x >= board->size || c.y < 0 || c.y >= board->size) return false;
    return board->playable[c.layer][c.y][c.x];
}

bool cf_parse_coord(const CfBoard *board, const char *text, CfCoord *out) {
    const char *p = text;
    int layer = 0;
    int x, y;
    char file;
    if (!board || !text || !out) return false;
    while (isspace((unsigned char)*p)) p++;
    if ((p[0] == 'L' || p[0] == 'l') && isdigit((unsigned char)p[1]) && p[2] == ':') {
        layer = p[1] - '1';
        p += 3;
    }
    if (!isalpha((unsigned char)p[0])) return false;
    file = (char)tolower((unsigned char)p[0]);
    x = file - 'a';
    p++;
    if (!isdigit((unsigned char)*p)) return false;
    y = atoi(p) - 1;
    out->layer = layer;
    out->x = x;
    out->y = y;
    return cf_board_is_playable(board, *out);
}

void cf_coord_to_string(const CfBoard *board, CfCoord c, char *out, int out_size) {
    if (!out || out_size <= 0) return;
    if (board && board->layers > 1) {
        snprintf(out, (size_t)out_size, "L%d:%c%d", c.layer + 1, 'a' + c.x, c.y + 1);
    } else {
        snprintf(out, (size_t)out_size, "%c%d", 'a' + c.x, c.y + 1);
    }
}

int cf_board_playable_count(const CfBoard *board) {
    int l, y, x, count = 0;
    if (!board) return 0;
    for (l = 0; l < board->layers; l++) {
        for (y = 0; y < board->size; y++) {
            for (x = 0; x < board->size; x++) {
                if (board->playable[l][y][x]) count++;
            }
        }
    }
    return count;
}
