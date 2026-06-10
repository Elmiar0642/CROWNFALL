#include "engine.h"

#include "dice.h"
#include "log.h"
#include "movegen.h"
#include "rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *motto;
    const char *emblem;
    const char *primary;
    const char *secondary;
    const char *asset;
} HouseSeed;

static const HouseSeed HOUSE_SEEDS[] = {
    {"House Albatross", "Beyond All Horizons", "Albatross", "#102a43", "#9fc5d9", "assets/houses/albatross.png"},
    {"House Dragon", "Reality Yields", "Dragon", "#8b1e1e", "#c4932f", "assets/houses/dragon.png"},
    {"House Crocodile", "Patience Consumes", "Crocodile", "#173d2b", "#b08a2c", "assets/houses/crocodile.png"},
    {"House Cobra", "One Strike Suffices", "Cobra", "#0f6b45", "#171717", "assets/houses/cobra.png"},
    {"House Owl", "Knowledge Devours", "Owl", "#4a2c6f", "#b8bec8", "assets/houses/owl.png"},
    {"House Eagle", "None Escape Our Sight", "Eagle", "#f2ead3", "#c9a227", "assets/houses/eagle.png"},
    {"House Wolf", "Together We Hunt", "Wolf", "#1f355c", "#aab2bd", "assets/houses/wolf.png"},
    {"House Lion", "By Strength We Reign", "Lion", "#101010", "#d6ad35", "assets/houses/lion.png"}
};

static const int MAP_2[] = {7, 1};
static const int MAP_4[] = {7, 5, 1, 6};
static const int MAP_8[] = {7, 5, 1, 6, 3, 2, 4, 0};

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

const char *cf_piece_role_name(CfPieceRole role) {
    switch (role) {
        case CF_ROLE_KING: return "King";
        case CF_ROLE_QUEEN_LEFT: return "Left Queen";
        case CF_ROLE_QUEEN_RIGHT: return "Right Queen";
        case CF_ROLE_PRINCE_LEFT: return "Left Crown Prince";
        case CF_ROLE_PRINCE_RIGHT: return "Right Crown Prince";
        case CF_ROLE_LOVE_INTEREST: return "Loyal Love Interest";
        case CF_ROLE_ROOK_LEFT: return "Left Rook";
        case CF_ROLE_ROOK_RIGHT: return "Right Rook";
        case CF_ROLE_KNIGHT_LEFT: return "Left Knight";
        case CF_ROLE_KNIGHT_RIGHT: return "Right Knight";
        case CF_ROLE_BISHOP_LEFT: return "Left Bishop";
        case CF_ROLE_BISHOP_RIGHT: return "Right Bishop";
        case CF_ROLE_PAWN_1: return "Pawn 1";
        case CF_ROLE_PAWN_2: return "Pawn 2";
        case CF_ROLE_PAWN_3: return "Pawn 3";
        case CF_ROLE_PAWN_4: return "Pawn 4";
        case CF_ROLE_PAWN_5: return "Pawn 5";
        case CF_ROLE_PAWN_6: return "Pawn 6";
        default: return "Unknown";
    }
}

const CfHouse *cf_get_house_info(const CfGame *game, int team_id) {
    if (!game || team_id < 0 || team_id >= game->house_count) return NULL;
    return &game->houses[team_id];
}

int cf_get_active_houses(const CfGame *game, const CfHouse **out, int max) {
    int i, n = 0;
    if (!game || !out || max <= 0) return 0;
    for (i = 0; i < game->house_count && n < max; i++) out[n++] = &game->houses[i];
    return n;
}

const char *cf_get_piece_identity(const CfGame *game, int piece_id) {
    if (!game || piece_id < 0 || piece_id >= game->piece_count) return "";
    return game->pieces[piece_id].identity;
}

const char *cf_get_player_identity(const CfGame *game, int player_id) {
    if (!game || player_id < 0 || player_id >= game->player_count) return "";
    return game->players[player_id].name;
}

static void set_house(CfHouse *h, int team, const HouseSeed *seed, int layer, const char *court) {
    memset(h, 0, sizeof(*h));
    h->team_id = team + 1;
    strncpy(h->house_name, seed->name, sizeof(h->house_name) - 1);
    strncpy(h->motto, seed->motto, sizeof(h->motto) - 1);
    strncpy(h->emblem, seed->emblem, sizeof(h->emblem) - 1);
    strncpy(h->primary_color, seed->primary, sizeof(h->primary_color) - 1);
    strncpy(h->secondary_color, seed->secondary, sizeof(h->secondary_color) - 1);
    strncpy(h->asset_path, seed->asset, sizeof(h->asset_path) - 1);
    h->default_layer = layer;
    strncpy(h->default_court, court, sizeof(h->default_court) - 1);
}

static bool json_string_value(const char *start, const char *key, char *out, size_t out_size) {
    char pattern[48];
    const char *p, *q;
    size_t n;
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(start, pattern);
    if (!p) return false;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return false;
    p = strchr(p, '"');
    if (!p) return false;
    q = strchr(p + 1, '"');
    if (!q) return false;
    n = (size_t)(q - p - 1);
    if (n >= out_size) n = out_size - 1;
    memcpy(out, p + 1, n);
    out[n] = '\0';
    return true;
}

static bool json_int_value(const char *start, const char *key, int *out) {
    char pattern[48];
    const char *p;
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(start, pattern);
    if (!p) return false;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return false;
    *out = atoi(p + 1);
    return true;
}

static void load_houses_config(CfGame *g) {
    FILE *f = fopen("config/houses.json", "r");
    char data[12000];
    size_t n;
    const char *p;
    if (!f) return;
    n = fread(data, 1, sizeof(data) - 1, f);
    fclose(f);
    data[n] = '\0';
    p = data;
    while ((p = strstr(p, "\"team_id\"")) != NULL) {
        int team = 0;
        CfHouse tmp;
        if (!json_int_value(p, "team_id", &team) || team < 1 || team > g->house_count) {
            p += 9;
            continue;
        }
        tmp = g->houses[team - 1];
        json_string_value(p, "house_name", tmp.house_name, sizeof(tmp.house_name));
        json_string_value(p, "motto", tmp.motto, sizeof(tmp.motto));
        json_string_value(p, "emblem", tmp.emblem, sizeof(tmp.emblem));
        json_string_value(p, "primary_color", tmp.primary_color, sizeof(tmp.primary_color));
        json_string_value(p, "secondary_color", tmp.secondary_color, sizeof(tmp.secondary_color));
        json_string_value(p, "asset_path", tmp.asset_path, sizeof(tmp.asset_path));
        json_string_value(p, "default_court", tmp.default_court, sizeof(tmp.default_court));
        json_int_value(p, "default_layer", &tmp.default_layer);
        g->houses[team - 1] = tmp;
        if (!g->config.team_names[team - 1][0]) strncpy(g->config.team_names[team - 1], tmp.house_name, CF_MAX_NAME - 1);
        p += 9;
    }
}

static void load_builtin_houses(CfGame *g) {
    static const char *courts[] = {"South", "East", "North", "West", "South", "East", "North", "West"};
    const int *map = g->config.team_count == 2 ? MAP_2 : g->config.team_count == 4 ? MAP_4 : MAP_8;
    int i;
    g->house_count = g->config.team_count;
    for (i = 0; i < g->house_count; i++) {
        int layer = i >= 4 ? 1 : 0;
        set_house(&g->houses[i], i, &HOUSE_SEEDS[map[i]], layer, courts[i]);
    }
    load_houses_config(g);
    if (g->config.team_count == 2) {
        set_house(&g->houses[0], 0, &HOUSE_SEEDS[7], 0, "South");
        set_house(&g->houses[1], 1, &HOUSE_SEEDS[1], 0, "North");
        strncpy(g->config.team_names[0], g->houses[0].house_name, CF_MAX_NAME - 1);
        strncpy(g->config.team_names[1], g->houses[1].house_name, CF_MAX_NAME - 1);
    }
    for (i = 0; i < g->house_count; i++) {
        if (!g->config.team_names[i][0]) strncpy(g->config.team_names[i], g->houses[i].house_name, CF_MAX_NAME - 1);
    }
}

static CfPiece *add_piece(CfGame *g, int team, CfPieceType type, CfPieceRole role, const char *label, int layer, int x, int y) {
    CfPiece *p;
    const CfHouse *h;
    if (g->piece_count >= CF_MAX_PIECES) return NULL;
    p = &g->pieces[g->piece_count];
    memset(p, 0, sizeof(*p));
    p->id = g->piece_count;
    p->team = team;
    p->type = type;
    p->role = role;
    strncpy(p->label, label, sizeof(p->label) - 1);
    p->pos.layer = layer;
    p->pos.x = x;
    p->pos.y = y;
    p->alive = cf_board_is_playable(&g->board, p->pos);
    h = cf_get_house_info(g, team);
    snprintf(p->identity, sizeof(p->identity), "%s of %s", cf_piece_role_name(role), h ? h->house_name : "Unknown House");
    g->piece_count++;
    return p;
}

static void setup_players(CfGame *g) {
    int r;
    const char *roles[3] = {"King", "Left House", "Right House"};
    for (r = 0; r < 3; r++) {
        int t;
        for (t = 0; t < g->config.team_count; t++) {
            CfPlayer *p = &g->players[g->player_count];
            int config_index = t * 3 + r;
            p->player_id = g->player_count;
            p->team = t;
            strncpy(p->role, roles[r], sizeof(p->role) - 1);
            if (g->config.player_names[config_index][0]) {
                strncpy(p->name, g->config.player_names[config_index], sizeof(p->name) - 1);
            } else {
                const CfHouse *h = cf_get_house_info(g, t);
                snprintf(p->name, sizeof(p->name), "%.44s %.18s", h ? h->house_name : "House", roles[r]);
            }
            g->player_count++;
        }
    }
}

typedef enum {
    CF_COURT_SOUTH,
    CF_COURT_EAST,
    CF_COURT_NORTH,
    CF_COURT_WEST
} CfCourtDirection;

typedef struct {
    CfPieceType type;
    CfPieceRole role;
    const char *label;
    int col;
    int row;
} CfTemplatePiece;

static const CfTemplatePiece LEGION_TEMPLATE[] = {
    {CF_PIECE_ROOK, CF_ROLE_ROOK_LEFT, "RL", 0, 0}, {CF_PIECE_KNIGHT, CF_ROLE_KNIGHT_LEFT, "NL", 1, 0},
    {CF_PIECE_BISHOP, CF_ROLE_BISHOP_LEFT, "BL", 2, 0}, {CF_PIECE_QUEEN, CF_ROLE_QUEEN_LEFT, "QL", 3, 0},
    {CF_PIECE_KING, CF_ROLE_KING, "K", 4, 0}, {CF_PIECE_QUEEN, CF_ROLE_QUEEN_RIGHT, "QR", 5, 0},
    {CF_PIECE_BISHOP, CF_ROLE_BISHOP_RIGHT, "BR", 6, 0}, {CF_PIECE_KNIGHT, CF_ROLE_KNIGHT_RIGHT, "NR", 7, 0},
    {CF_PIECE_ROOK, CF_ROLE_ROOK_RIGHT, "RR", 8, 0},
    {CF_PIECE_PAWN, CF_ROLE_PAWN_1, "p1", 1, 1}, {CF_PIECE_PAWN, CF_ROLE_PAWN_2, "p2", 2, 1},
    {CF_PIECE_PRINCE, CF_ROLE_PRINCE_LEFT, "PL", 3, 1}, {CF_PIECE_LOVE, CF_ROLE_LOVE_INTEREST, "L", 4, 1},
    {CF_PIECE_PRINCE, CF_ROLE_PRINCE_RIGHT, "PR", 5, 1}, {CF_PIECE_PAWN, CF_ROLE_PAWN_3, "p3", 6, 1},
    {CF_PIECE_PAWN, CF_ROLE_PAWN_4, "p4", 7, 1},
    {CF_PIECE_PAWN, CF_ROLE_PAWN_5, "p5", 3, 2}, {CF_PIECE_PAWN, CF_ROLE_PAWN_6, "p6", 5, 2}
};

static void rotate_template_coord(CfCourtDirection court, int col, int row, int *x, int *y) {
    switch (court) {
        case CF_COURT_SOUTH:
            *x = 3 + col;
            *y = row;
            break;
        case CF_COURT_NORTH:
            *x = 3 + col;
            *y = 14 - row;
            break;
        case CF_COURT_EAST:
            *x = 14 - row;
            *y = 3 + col;
            break;
        case CF_COURT_WEST:
            *x = row;
            *y = 3 + col;
            break;
    }
}

static void setup_team_2(CfGame *g, int team, int layer, int back_y) {
    int i;
    int forward = back_y == 0 ? 1 : -1;
    for (i = 0; i < (int)(sizeof(LEGION_TEMPLATE) / sizeof(LEGION_TEMPLATE[0])); i++) {
        int x = LEGION_TEMPLATE[i].col;
        int y = back_y + LEGION_TEMPLATE[i].row * forward;
        add_piece(g, team, LEGION_TEMPLATE[i].type, LEGION_TEMPLATE[i].role, LEGION_TEMPLATE[i].label, layer, x, y);
    }
}

static void board_place_legion(CfGame *g, int team, int layer, CfCourtDirection court) {
    int i;
    for (i = 0; i < (int)(sizeof(LEGION_TEMPLATE) / sizeof(LEGION_TEMPLATE[0])); i++) {
        int x = 0, y = 0;
        rotate_template_coord(court, LEGION_TEMPLATE[i].col, LEGION_TEMPLATE[i].row, &x, &y);
        add_piece(g, team, LEGION_TEMPLATE[i].type, LEGION_TEMPLATE[i].role, LEGION_TEMPLATE[i].label, layer, x, y);
    }
}

static void setup_pieces(CfGame *g) {
    if (g->config.team_count == 2) {
        setup_team_2(g, 0, 0, 0);
        setup_team_2(g, 1, 0, 8);
        return;
    }
    board_place_legion(g, 0, 0, CF_COURT_SOUTH);
    board_place_legion(g, 1, 0, CF_COURT_EAST);
    board_place_legion(g, 2, 0, CF_COURT_NORTH);
    board_place_legion(g, 3, 0, CF_COURT_WEST);
    if (g->config.team_count == 8) {
        board_place_legion(g, 4, 1, CF_COURT_SOUTH);
        board_place_legion(g, 5, 1, CF_COURT_EAST);
        board_place_legion(g, 6, 1, CF_COURT_NORTH);
        board_place_legion(g, 7, 1, CF_COURT_WEST);
    }
}

bool cf_engine_player_controls_piece(const CfPlayer *player, const CfPiece *piece) {
    if (!player || !piece || player->team != piece->team) return false;
    if (piece->type == CF_PIECE_ROOK || piece->type == CF_PIECE_KNIGHT ||
        piece->type == CF_PIECE_BISHOP || piece->type == CF_PIECE_PAWN) return true;
    if (strcmp(player->role, "King") == 0) {
        return piece->role == CF_ROLE_KING || piece->role == CF_ROLE_LOVE_INTEREST;
    }
    if (strcmp(player->role, "Left House") == 0) {
        return piece->role == CF_ROLE_QUEEN_LEFT || piece->role == CF_ROLE_PRINCE_LEFT;
    }
    if (strcmp(player->role, "Right House") == 0) {
        return piece->role == CF_ROLE_QUEEN_RIGHT || piece->role == CF_ROLE_PRINCE_RIGHT;
    }
    return false;
}

CfGame *cf_engine_new(const CfConfig *config) {
    CfGame *g = calloc(1, sizeof(*g));
    char fields[512];
    int i;
    if (!g) return NULL;
    if (config) g->config = *config;
    if (g->config.team_count == 0) g->config.team_count = 2;
    load_builtin_houses(g);
    cf_board_init(&g->board, g->config.team_count);
    cf_dice_seed(0);
    setup_players(g);
    setup_pieces(g);
    cf_log_open(g);
    snprintf(fields, sizeof(fields), "\"version\":\"0.2.0\",\"log_path\":\"%s\"", g->log_path);
    cf_log_event(g, "session_start", fields);
    snprintf(fields, sizeof(fields), "\"configured_team_count\":%d,\"time_travel\":%s,\"playable_squares\":%d",
             g->config.team_count, g->config.time_travel ? "true" : "false", cf_board_playable_count(&g->board));
    cf_log_event(g, "config", fields);
    for (i = 0; i < g->config.team_count; i++) {
        char name[CF_MAX_NAME * 2];
        cf_json_escape(g->config.team_names[i], name, sizeof(name));
        snprintf(fields, sizeof(fields), "\"registered_team_id\":%d,\"registered_house_name\":\"%s\",\"motto\":\"%s\",\"emblem\":\"%s\",\"court\":\"%s\",\"layer\":%d",
                 i + 1, name, g->houses[i].motto, g->houses[i].emblem, g->houses[i].default_court, g->houses[i].default_layer + 1);
        cf_log_event(g, "team_registered", fields);
    }
    for (i = 0; i < g->player_count; i++) {
        char name[CF_MAX_NAME * 2];
        cf_json_escape(g->players[i].name, name, sizeof(name));
        snprintf(fields, sizeof(fields), "\"player_id\":%d,\"registered_team_id\":%d,\"registered_house_name\":\"%s\",\"registered_player_role\":\"%s\",\"registered_player_name\":\"%s\"",
                 g->players[i].player_id, g->players[i].team + 1, g->houses[g->players[i].team].house_name, g->players[i].role, name);
        cf_log_event(g, "player_registered", fields);
    }
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
    char fields[768];
    CfPlayer *p;
    if (!game || game->player_count == 0) return;
    p = &game->players[game->current_player];
    game->dice_rolled = false;
    game->die_a = game->die_b = game->dice_sum = 0;
    snprintf(fields, sizeof(fields), "\"player_id\":%d,\"human_readable_summary\":\"%s %s turn started.\"",
             p->player_id, game->houses[p->team].house_name, p->role);
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
    CfMove pseudo[CF_MAX_MOVES];
    int i, count = 0, pseudo_count;
    char sq[16], fields[512];
    if (!game || player_id < 0 || player_id >= game->player_count) return 0;
    if (!game->dice_rolled) return 0;
    idx = cf_engine_piece_at(game, from);
    if (idx < 0 || !cf_engine_player_controls_piece(&game->players[player_id], &game->pieces[idx])) return 0;
    pseudo_count = cf_movegen_for_piece(game, &game->pieces[idx], pseudo, CF_MAX_MOVES);
    for (i = 0; i < pseudo_count && count < max_moves; i++) {
        if (cf_rules_move_preserves_king_safety(game, &pseudo[i])) moves[count++] = pseudo[i];
    }
    cf_coord_to_string(&game->board, from, sq, sizeof(sq));
    snprintf(fields, sizeof(fields), "\"player_id\":%d,\"square\":\"%s\",\"count\":%d,\"human_readable_summary\":\"Legal moves shown for %s.\"",
             player_id, sq, count, sq);
    cf_log_event(game, "legal_moves_generated", fields);
    return count;
}

static void handle_capture(CfGame *g, CfPiece *attacker, CfPiece *target) {
    char fields[768];
    target->alive = false;
    snprintf(fields, sizeof(fields), "\"attacker\":%d,\"captured\":%d,\"captured_team_id\":%d,\"captured_house_name\":\"%s\",\"piece_id\":%d,\"piece_role\":\"%s\",\"human_readable_summary\":\"%s captured %s.\"",
             attacker->id, target->id, target->team + 1, g->houses[target->team].house_name, target->id,
             cf_piece_role_name(target->role), attacker->identity, target->identity);
    cf_log_event(g, "capture", fields);
    if (target->type == CF_PIECE_PRINCE) {
        g->bloodfall[target->team] = true;
        snprintf(fields, sizeof(fields), "\"fallen_team_id\":%d,\"fallen_house_name\":\"%s\",\"prince_id\":%d,\"piece_role\":\"%s\",\"human_readable_summary\":\"Bloodfall: %s has fallen.\"",
                 target->team + 1, g->houses[target->team].house_name, target->id, cf_piece_role_name(target->role), target->identity);
        cf_log_event(g, "bloodfall", fields);
        cf_log_event(g, "widow_freeze", fields);
    }
}

static void log_illegal_move(CfGame *game, const char *reason) {
    char fields[256];
    snprintf(fields, sizeof(fields), "\"reason\":\"%s\",\"human_readable_summary\":\"Illegal move: %s.\"",
             reason ? reason : "unknown", reason ? reason : "unknown");
    cf_log_event(game, "illegal_move", fields);
}

bool cf_engine_apply_move(CfGame *game, const CfMove *move, char *err, int err_size) {
    int idx, cap;
    char a[16], b[16], fields[768];
    if (!game || !move) return false;
    idx = cf_engine_piece_at(game, move->from);
    if (idx < 0) {
        snprintf(err, (size_t)err_size, "no piece at source");
        log_illegal_move(game, "no piece at source");
        return false;
    }
    if (!game->dice_rolled) {
        snprintf(err, (size_t)err_size, "roll dice before moving");
        log_illegal_move(game, "roll dice before moving");
        return false;
    }
    if (!cf_engine_player_controls_piece(&game->players[game->current_player], &game->pieces[idx])) {
        snprintf(err, (size_t)err_size, "piece is not controlled by current player");
        log_illegal_move(game, "piece is not controlled by current player");
        return false;
    }
    if (!cf_rules_move_preserves_king_safety(game, move)) {
        snprintf(err, (size_t)err_size, "move leaves king in check");
        log_illegal_move(game, "move leaves king in check");
        return false;
    }
    cap = cf_engine_piece_at(game, move->to);
    cf_coord_to_string(&game->board, move->from, a, sizeof(a));
    cf_coord_to_string(&game->board, move->to, b, sizeof(b));
    snprintf(fields, sizeof(fields), "\"piece_id\":%d,\"piece_role\":\"%s\",\"from\":\"%s\",\"to\":\"%s\",\"human_readable_summary\":\"%s attempted %s to %s.\"",
             game->pieces[idx].id, cf_piece_role_name(game->pieces[idx].role), a, b, game->pieces[idx].identity, a, b);
    cf_log_event(game, "move_attempt", fields);
    if (cap >= 0 && !cf_rules_can_capture(game, &game->pieces[idx], &game->pieces[cap])) {
        snprintf(err, (size_t)err_size, "capture blocked by royal rule");
        log_illegal_move(game, "capture blocked by royal rule");
        return false;
    }
    if (cap >= 0) handle_capture(game, &game->pieces[idx], &game->pieces[cap]);
    game->pieces[idx].pos = move->to;
    snprintf(fields, sizeof(fields), "\"piece_id\":%d,\"piece_role\":\"%s\",\"from\":\"%s\",\"to\":\"%s\",\"human_readable_summary\":\"%s moved %s to %s.\"",
             game->pieces[idx].id, cf_piece_role_name(game->pieces[idx].role), a, b, game->pieces[idx].identity, a, b);
    cf_log_event(game, "move_success", fields);
    if (!cf_rules_repetition_legal(game)) {
        snprintf(err, (size_t)err_size, "fourth repetition is illegal");
        log_illegal_move(game, "fourth repetition is illegal");
        return false;
    }
    {
        int t;
        for (t = 0; t < game->config.team_count; t++) {
            if (t != game->pieces[idx].team && cf_rules_team_in_check(game, t)) {
                snprintf(fields, sizeof(fields), "\"checked_team_id\":%d,\"checked_house_name\":\"%s\",\"human_readable_summary\":\"Check: %s King is in check.\"",
                         t + 1, game->houses[t].house_name, game->houses[t].house_name);
                cf_log_event(game, "check", fields);
            }
        }
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
                if (!cf_board_is_playable(&game->board, c)) fprintf(out, " ####");
                else if (p >= 0) {
                    char s[16];
                    const CfPiece *piece = &game->pieces[p];
                    if (piece->role == CF_ROLE_QUEEN_LEFT) snprintf(s, sizeof(s), "Q%dL", piece->team + 1);
                    else if (piece->role == CF_ROLE_QUEEN_RIGHT) snprintf(s, sizeof(s), "Q%dR", piece->team + 1);
                    else if (piece->role == CF_ROLE_PRINCE_LEFT) snprintf(s, sizeof(s), "P%dL", piece->team + 1);
                    else if (piece->role == CF_ROLE_PRINCE_RIGHT) snprintf(s, sizeof(s), "P%dR", piece->team + 1);
                    else if (piece->type == CF_PIECE_PAWN) snprintf(s, sizeof(s), "%s", piece->label);
                    else snprintf(s, sizeof(s), "%s%d", piece->label, piece->team + 1);
                    fprintf(out, " %4s", s);
                } else fprintf(out, "   ..");
            }
            fprintf(out, "\n");
        }
        fprintf(out, "   ");
        for (x = 0; x < game->board.size; x++) fprintf(out, "  %c  ", 'a' + x);
        fprintf(out, "\n\n");
    }
}

void cf_engine_print_state(const CfGame *game, FILE *out) {
    const CfPlayer *p;
    if (!game) return;
    p = &game->players[game->current_player];
    fprintf(out, "Session: %s\nTurn: %d\nHouse: %s\nPlayer: %s (%s)\nDice: %d + %d = %d%s\nLog: %s\n",
            game->session_id, game->turn_id, game->houses[p->team].house_name, p->name, p->role,
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
