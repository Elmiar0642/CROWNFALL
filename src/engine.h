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
#define CF_MAX_HOUSE_TEXT 128

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

typedef enum {
    CF_ROLE_NONE,
    CF_ROLE_KING,
    CF_ROLE_QUEEN_LEFT,
    CF_ROLE_QUEEN_RIGHT,
    CF_ROLE_PRINCE_LEFT,
    CF_ROLE_PRINCE_RIGHT,
    CF_ROLE_LOVE_INTEREST,
    CF_ROLE_ROOK_LEFT,
    CF_ROLE_ROOK_RIGHT,
    CF_ROLE_KNIGHT_LEFT,
    CF_ROLE_KNIGHT_RIGHT,
    CF_ROLE_BISHOP_LEFT,
    CF_ROLE_BISHOP_RIGHT,
    CF_ROLE_PAWN_1,
    CF_ROLE_PAWN_2,
    CF_ROLE_PAWN_3,
    CF_ROLE_PAWN_4,
    CF_ROLE_PAWN_5,
    CF_ROLE_PAWN_6
} CfPieceRole;

typedef struct {
    int team_id;
    char house_name[CF_MAX_NAME];
    char motto[CF_MAX_HOUSE_TEXT];
    char emblem[CF_MAX_NAME];
    char primary_color[16];
    char secondary_color[16];
    char asset_path[160];
    int default_layer;
    char default_court[16];
} CfHouse;

typedef struct {
    int id;
    int team;
    CfPieceType type;
    CfPieceRole role;
    char label[4];
    char identity[CF_MAX_HOUSE_TEXT];
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

typedef void (*CfLogSink)(void *user, const char *event, const char *json_fields);

typedef struct {
    CfConfig config;
    CfHouse houses[CF_MAX_TEAMS];
    int house_count;
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
    CfLogSink log_sink;
    void *log_sink_user;
    char repetition[CF_MAX_HISTORY][512];
    int repetition_count[CF_MAX_HISTORY];
    int repetition_len;
} CfGame;

const char *cf_piece_type_name(CfPieceType type);
const char *cf_piece_role_name(CfPieceRole role);
const CfHouse *cf_get_house_info(const CfGame *game, int team_id);
int cf_get_active_houses(const CfGame *game, const CfHouse **out, int max);
const char *cf_get_piece_identity(const CfGame *game, int piece_id);
const char *cf_get_player_identity(const CfGame *game, int player_id);
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
