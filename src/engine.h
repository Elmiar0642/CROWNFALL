#ifndef CROWNFALL_ENGINE_H
#define CROWNFALL_ENGINE_H

#include "board.h"

#include <stdbool.h>
#include <stdio.h>

#define CF_MAX_TEAMS 8
#define CF_MAX_PLAYERS 24
#define CF_MAX_PIECES 160
#define CF_MAX_NAME 64
#define CF_MAX_MOVES 256
#define CF_MAX_PACTS 256
#define CF_MAX_HISTORY 512

typedef enum {
    CF_PIECE_NONE,
    CF_PIECE_KING,
    CF_PIECE_QUEEN,
    CF_PIECE_PRINCE,
    CF_PIECE_LOVE,
    CF_PIECE_ROOK,
    CF_PIECE_KNIGHT,
    CF_PIECE_BISHOP,
    CF_PIECE_PAWN
} CfPieceType;

typedef struct {
    int id;
    int team;
    CfPieceType type;
    char label[4];
    CfCoord pos;
    bool alive;
    bool widow;
    bool frozen;
} CfPiece;

typedef struct {
    CfCoord from;
    CfCoord to;
    int piece_id;
    int capture_id;
    char notation[32];
} CfMove;

typedef struct {
    int player_id;
    int team;
    char role[16];
    char name[CF_MAX_NAME];
} CfPlayer;

typedef struct {
    int team_count;
    bool time_travel;
    char mode[16];
    char team_names[CF_MAX_TEAMS][CF_MAX_NAME];
    char player_names[CF_MAX_PLAYERS][CF_MAX_NAME];
} CfConfig;

typedef struct {
    int queen_id;
    int prince_id;
    int count;
    bool pact;
} CfMercy;

typedef struct {
    CfConfig config;
    CfBoard board;
    CfPiece pieces[CF_MAX_PIECES];
    int piece_count;
    CfPlayer players[CF_MAX_PLAYERS];
    int player_count;
    int current_player;
    int turn_id;
    int die_a;
    int die_b;
    int dice_sum;
    bool dice_rolled;
    bool bloodfall[CF_MAX_TEAMS];
    CfMercy mercies[CF_MAX_PACTS];
    int mercy_count;
    char session_id[64];
    char log_path[256];
    FILE *log_file;
    char repetition[CF_MAX_HISTORY][512];
    int repetition_count[CF_MAX_HISTORY];
    int repetition_len;
} CfGame;

const char *cf_piece_type_name(CfPieceType type);
CfGame *cf_engine_new(const CfConfig *config);
void cf_engine_free(CfGame *game);
void cf_engine_start_turn(CfGame *game);
int cf_engine_piece_at(const CfGame *game, CfCoord c);
int cf_engine_generate_moves(CfGame *game, int player_id, CfCoord from, CfMove *moves, int max_moves);
bool cf_engine_apply_move(CfGame *game, const CfMove *move, char *err, int err_size);
void cf_engine_print_board(const CfGame *game, FILE *out);
void cf_engine_print_state(const CfGame *game, FILE *out);
void cf_engine_next_turn(CfGame *game);
bool cf_engine_snapshot(CfGame *game, const char *path);
bool cf_engine_branch(CfGame *game, int turn_id, char *out_path, int out_size);

#endif
