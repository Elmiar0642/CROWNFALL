#include "gui.h"

#ifdef CROWNFALL_HAVE_GTK
#include "dice.h"
#include "log.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUI_MAX_LAYER_AREAS 2

typedef struct {
    CfGame *game;
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *mode_combo;
    GtkWidget *team_combo;
    GtkWidget *time_check;
    GtkTextBuffer *team_buffer;
    GtkTextBuffer *player_buffer;
    GtkWidget *turn_label;
    GtkWidget *dice_label;
    GtkWidget *status_label;
    GtkWidget *branch_button;
    GtkWidget *filter_combo;
    GtkWidget *layer_box[GUI_MAX_LAYER_AREAS];
    GtkWidget *layer_area[GUI_MAX_LAYER_AREAS];
    GtkTextBuffer *log_buffer;
    GtkTextBuffer *help_buffer;
    bool selected;
    CfCoord selected_coord;
    CfMove legal_moves[CF_MAX_MOVES];
    int legal_count;
    bool has_last_move;
    CfCoord last_from;
    CfCoord last_to;
} GuiCtx;

typedef struct {
    const char *name;
    const char *motto;
    const char *emblem;
    const char *primary;
    const char *secondary;
} GuiHouse;

static const GuiHouse GUI_HOUSES[] = {
    {"House Albatross", "Beyond All Horizons", "Albatross", "#102a43", "#9fc5d9"},
    {"House Dragon", "Reality Yields", "Dragon", "#8b1e1e", "#c4932f"},
    {"House Crocodile", "Patience Consumes", "Crocodile", "#173d2b", "#b08a2c"},
    {"House Cobra", "One Strike Suffices", "Cobra", "#0f6b45", "#171717"},
    {"House Owl", "Knowledge Devours", "Owl", "#4a2c6f", "#b8bec8"},
    {"House Eagle", "None Escape Our Sight", "Eagle", "#f2ead3", "#c9a227"},
    {"House Wolf", "Together We Hunt", "Wolf", "#1f355c", "#aab2bd"},
    {"House Lion", "By Strength We Reign", "Lion", "#101010", "#d6ad35"}
};

static const int GUI_MAP_2[] = {7, 1};
static const int GUI_MAP_4[] = {7, 5, 1, 6};
static const int GUI_MAP_8[] = {7, 5, 1, 6, 3, 2, 4, 0};

static const char *RANDOM_NAMES[] = {
    "Aurelian", "Veyra", "Kaelen", "Myrr", "Osric", "Selene", "Thane",
    "Ilyra", "Corvin", "Maera", "Draven", "Eryndor", "Nyx", "Vael",
    "Soren", "Elara", "Arctus", "Neria", "Oryn", "Lyssa"
};

static void gui_log(GuiCtx *ctx, const char *text) {
    GtkTextIter end;
    if (!ctx || !ctx->log_buffer || !text) return;
    gtk_text_buffer_get_end_iter(ctx->log_buffer, &end);
    gtk_text_buffer_insert(ctx->log_buffer, &end, text, -1);
    gtk_text_buffer_insert(ctx->log_buffer, &end, "\n", -1);
}

static bool json_summary(const char *json, char *out, int out_size) {
    const char *key = "\"human_readable_summary\":\"";
    const char *p = strstr(json ? json : "", key);
    const char *q;
    int n = 0;
    if (!p || !out || out_size <= 0) return false;
    p += strlen(key);
    q = p;
    while (*q && !(*q == '"' && (q == p || q[-1] != '\\'))) q++;
    while (p < q && n < out_size - 1) {
        if (*p == '\\' && p + 1 < q) p++;
        out[n++] = *p++;
    }
    out[n] = '\0';
    return true;
}

static void gui_log_event_sink(void *user, const char *event, const char *json_fields) {
    GuiCtx *ctx = user;
    char summary[512];
    if (json_summary(json_fields, summary, sizeof(summary))) gui_log(ctx, summary);
    else if (event) {
        snprintf(summary, sizeof(summary), "%s", event);
        gui_log(ctx, summary);
    }
}

static void load_existing_log(GuiCtx *ctx) {
    FILE *f;
    char line[2048], summary[512];
    if (!ctx || !ctx->game || !ctx->game->log_path[0]) return;
    f = fopen(ctx->game->log_path, "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        if (json_summary(line, summary, sizeof(summary))) gui_log(ctx, summary);
        else if (strstr(line, "\"event\":\"session_start\"")) gui_log(ctx, "Session started");
        else if (strstr(line, "\"event\":\"config\"")) gui_log(ctx, "Configuration loaded");
        else if (strstr(line, "\"event\":\"player_registered\"")) gui_log(ctx, "Player registered");
    }
    fclose(f);
}

static void set_status(GuiCtx *ctx, const char *text) {
    if (ctx && ctx->status_label) gtk_label_set_text(GTK_LABEL(ctx->status_label), text ? text : "");
    if (text && text[0]) gui_log(ctx, text);
}

static void set_status_only(GuiCtx *ctx, const char *text) {
    if (ctx && ctx->status_label) gtk_label_set_text(GTK_LABEL(ctx->status_label), text ? text : "");
}

static int selected_team_count(GuiCtx *ctx) {
    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->team_combo));
    return active == 1 ? 4 : active == 2 ? 6 : active == 3 ? 8 : 2;
}

static const GuiHouse *gui_house_for_slot(int teams, int slot) {
    const int *map = teams == 2 ? GUI_MAP_2 : teams == 4 ? GUI_MAP_4 : GUI_MAP_8;
    if (slot < 0 || slot >= teams || slot >= 8) return &GUI_HOUSES[0];
    return &GUI_HOUSES[map[slot]];
}

static void hex_color(const char *hex, double *r, double *g, double *b) {
    unsigned int rv = 120, gv = 120, bv = 120;
    if (hex && hex[0] == '#') sscanf(hex + 1, "%02x%02x%02x", &rv, &gv, &bv);
    *r = rv / 255.0;
    *g = gv / 255.0;
    *b = bv / 255.0;
}

static char *buffer_text(GtkTextBuffer *buffer) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void copy_lines_to_names(char *text, char names[][CF_MAX_NAME], int max_names, const char *prefix) {
    int i = 0;
    char *line = strtok(text, "\n\r");
    (void)prefix;
    while (line && i < max_names) {
        while (*line == ' ' || *line == '\t') line++;
        if (*line) strncpy(names[i], line, CF_MAX_NAME - 1);
        i++;
        line = strtok(NULL, "\n\r");
    }
}

static void fill_default_names(GuiCtx *ctx) {
    int i;
    int teams = selected_team_count(ctx);
    GString *houses = g_string_new("");
    GString *players = g_string_new("");
    for (i = 0; i < teams; i++) {
        const GuiHouse *h = gui_house_for_slot(teams, i);
        g_string_append_printf(houses, "%s\n", h->name);
        g_string_append_printf(players, "%s King\n%s Left House\n%s Right House\n", h->name, h->name, h->name);
    }
    gtk_text_buffer_set_text(ctx->team_buffer, houses->str, -1);
    gtk_text_buffer_set_text(ctx->player_buffer, players->str, -1);
    g_string_free(houses, TRUE);
    g_string_free(players, TRUE);
    set_status(ctx, "House defaults loaded.");
}

static void fill_random_names(GuiCtx *ctx) {
    int i;
    int teams = selected_team_count(ctx);
    GString *team = g_string_new("");
    GString *players = g_string_new("");
    srand(1);
    for (i = 0; i < teams; i++) {
        const GuiHouse *h = gui_house_for_slot(teams, i);
        const char *short_name = h->name + 6;
        g_string_append_printf(team, "%s\n", h->name);
        g_string_append_printf(players, "%s of %s\n", RANDOM_NAMES[rand() % 20], h->name);
        g_string_append_printf(players, "%s, Left Prince of %s\n", RANDOM_NAMES[rand() % 20], h->name);
        g_string_append_printf(players, "%s, Right Prince of %s\n", RANDOM_NAMES[rand() % 20], h->name);
        (void)short_name;
    }
    gtk_text_buffer_set_text(ctx->team_buffer, team->str, -1);
    gtk_text_buffer_set_text(ctx->player_buffer, players->str, -1);
    g_string_free(team, TRUE);
    g_string_free(players, TRUE);
    set_status(ctx, "Deterministic House names loaded.");
}

static void on_default_names(GtkButton *button, gpointer data) {
    (void)button;
    fill_default_names((GuiCtx *)data);
}

static void on_random_names(GtkButton *button, gpointer data) {
    (void)button;
    fill_random_names((GuiCtx *)data);
}

static void refresh_labels(GuiCtx *ctx) {
    char buf[256];
    CfPlayer *p;
    int i;
    if (!ctx->game) return;
    p = &ctx->game->players[ctx->game->current_player];
    snprintf(buf, sizeof(buf), "Turn %d | %s | %s | %s", ctx->game->turn_id, ctx->game->houses[p->team].house_name, p->role, p->name);
    gtk_label_set_text(GTK_LABEL(ctx->turn_label), buf);
    snprintf(buf, sizeof(buf), "Dice: %d + %d = %d%s", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum,
             ctx->game->dice_rolled ? "" : " (roll before sliding)");
    gtk_label_set_text(GTK_LABEL(ctx->dice_label), buf);
    gtk_widget_set_visible(ctx->layer_box[1], ctx->game->board.layers > 1);
    for (i = 0; i < GUI_MAX_LAYER_AREAS; i++) gtk_widget_queue_draw(ctx->layer_area[i]);
}

static bool move_targets(CfMove *m, CfCoord c) {
    return m->to.layer == c.layer && m->to.x == c.x && m->to.y == c.y;
}

static bool coord_equal(CfCoord a, CfCoord b) {
    return a.layer == b.layer && a.x == b.x && a.y == b.y;
}

static void draw_centered_text(cairo_t *cr, double x, double y, const char *text, double size) {
    cairo_text_extents_t ext;
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, size);
    cairo_text_extents(cr, text, &ext);
    cairo_move_to(cr, x - ext.width / 2 - ext.x_bearing, y - ext.height / 2 - ext.y_bearing);
    cairo_show_text(cr, text);
}

static void piece_label(const CfPiece *piece, char *out, int out_size) {
    if (piece->role == CF_ROLE_QUEEN_LEFT) snprintf(out, (size_t)out_size, "Q%dL", piece->team + 1);
    else if (piece->role == CF_ROLE_QUEEN_RIGHT) snprintf(out, (size_t)out_size, "Q%dR", piece->team + 1);
    else if (piece->role == CF_ROLE_PRINCE_LEFT) snprintf(out, (size_t)out_size, "P%dL", piece->team + 1);
    else if (piece->role == CF_ROLE_PRINCE_RIGHT) snprintf(out, (size_t)out_size, "P%dR", piece->team + 1);
    else if (piece->type == CF_PIECE_PAWN) snprintf(out, (size_t)out_size, "%s", piece->label);
    else snprintf(out, (size_t)out_size, "%s%d", piece->label, piece->team + 1);
}

static void draw_house_zone(GuiCtx *ctx, cairo_t *cr, int layer, int team, double pad, double cell) {
    const CfHouse *h = cf_get_house_info(ctx->game, team);
    double r, g, b, x = 0, y = 0, w = 0, ht = 0;
    char title[96];
    if (!h || h->default_layer != layer) return;
    if (ctx->game->board.size == 9) {
        x = pad;
        y = pad + (team == 0 ? 6 : 0) * cell;
        w = 9 * cell;
        ht = 3 * cell;
    } else if (strcmp(h->default_court, "South") == 0) {
        x = pad + 3 * cell; y = pad + 12 * cell; w = 9 * cell; ht = 3 * cell;
    } else if (strcmp(h->default_court, "North") == 0) {
        x = pad + 3 * cell; y = pad; w = 9 * cell; ht = 3 * cell;
    } else if (strcmp(h->default_court, "East") == 0) {
        x = pad + 12 * cell; y = pad + 3 * cell; w = 3 * cell; ht = 9 * cell;
    } else {
        x = pad; y = pad + 3 * cell; w = 3 * cell; ht = 9 * cell;
    }
    hex_color(h->primary_color, &r, &g, &b);
    cairo_set_source_rgba(cr, r, g, b, 0.18);
    cairo_rectangle(cr, x + 4, y + 4, w - 8, ht - 8);
    cairo_stroke(cr);
    hex_color(h->secondary_color, &r, &g, &b);
    cairo_set_source_rgba(cr, r, g, b, 0.95);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, x + 1.5, y + 1.5, w - 3, ht - 3);
    cairo_stroke(cr);
    cairo_set_source_rgb(cr, 0.96, 0.95, 0.90);
    snprintf(title, sizeof(title), "%s", h->house_name);
    draw_centered_text(cr, x + w / 2, y + ht / 2 - 9, title, cell * 0.18);
    draw_centered_text(cr, x + w / 2, y + ht / 2 + 9, h->motto, cell * 0.12);
}

static gboolean on_board_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    GuiCtx *ctx = data;
    int layer = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "layer-id"));
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);
    int x, y, i;
    double pad = 28.0;
    double board_px;
    double cell;
    if (!ctx->game) return FALSE;
    board_px = (w < h ? w : h) - pad * 2.0;
    if (board_px < 1.0) return FALSE;
    cell = board_px / ctx->game->board.size;
    cairo_set_source_rgb(cr, 0.08, 0.09, 0.11);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.88, 0.88, 0.84);
    draw_centered_text(cr, w / 2.0, 14.0, layer == 0 ? "Layer L1: Lion, Eagle, Dragon, Wolf" : "Layer L2: Cobra, Crocodile, Owl, Albatross", 12.0);
    for (y = 0; y < ctx->game->board.size; y++) {
        for (x = 0; x < ctx->game->board.size; x++) {
            CfCoord c = {layer, x, y};
            double sx = pad + x * cell;
            double sy = pad + (ctx->game->board.size - 1 - y) * cell;
            bool legal = false;
            for (i = 0; i < ctx->legal_count; i++) if (move_targets(&ctx->legal_moves[i], c)) legal = true;
            if (!cf_board_is_playable(&ctx->game->board, c)) cairo_set_source_rgb(cr, 0.02, 0.02, 0.025);
            else if (ctx->game->board.size == 15 && (x < 3 || x > 11 || y < 3 || y > 11)) cairo_set_source_rgb(cr, 0.18, 0.22, 0.25);
            else cairo_set_source_rgb(cr, ((x + y) % 2) ? 0.68 : 0.78, ((x + y) % 2) ? 0.70 : 0.78, ((x + y) % 2) ? 0.62 : 0.68);
            cairo_rectangle(cr, sx, sy, cell, cell);
            cairo_fill(cr);
            if (ctx->selected && coord_equal(ctx->selected_coord, c)) {
                cairo_set_source_rgb(cr, 0.1, 0.45, 1.0);
                cairo_set_line_width(cr, 3.0);
                cairo_rectangle(cr, sx + 2, sy + 2, cell - 4, cell - 4);
                cairo_stroke(cr);
            } else if (legal) {
                cairo_set_source_rgba(cr, 0.2, 0.8, 0.25, 0.45);
                cairo_arc(cr, sx + cell / 2, sy + cell / 2, cell * 0.22, 0, 6.28318);
                cairo_fill(cr);
            } else if (ctx->has_last_move && (coord_equal(ctx->last_from, c) || coord_equal(ctx->last_to, c))) {
                cairo_set_source_rgba(cr, 1.0, 0.8, 0.1, 0.35);
                cairo_rectangle(cr, sx + 2, sy + 2, cell - 4, cell - 4);
                cairo_fill(cr);
            }
            cairo_set_source_rgb(cr, 0.16, 0.16, 0.16);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, sx, sy, cell, cell);
            cairo_stroke(cr);
        }
    }
    for (i = 0; i < ctx->game->house_count; i++) draw_house_zone(ctx, cr, layer, i, pad, cell);
    for (y = 0; y < ctx->game->board.size; y++) {
        for (x = 0; x < ctx->game->board.size; x++) {
            CfCoord c = {layer, x, y};
            double sx = pad + x * cell;
            double sy = pad + (ctx->game->board.size - 1 - y) * cell;
            int p = cf_engine_piece_at(ctx->game, c);
            if (p >= 0) {
                char label[8];
                const CfHouse *hinfo = cf_get_house_info(ctx->game, ctx->game->pieces[p].team);
                double rr = 0.05, gg = 0.05, bb = 0.05;
                piece_label(&ctx->game->pieces[p], label, sizeof(label));
                if (hinfo) hex_color(hinfo->primary_color, &rr, &gg, &bb);
                cairo_set_source_rgb(cr, rr, gg, bb);
                cairo_arc(cr, sx + cell / 2, sy + cell / 2, cell * 0.38, 0, 6.28318);
                cairo_fill(cr);
                cairo_set_source_rgb(cr, 0.94, 0.93, 0.86);
                draw_centered_text(cr, sx + cell / 2, sy + cell / 2, label, cell * 0.24);
            }
        }
    }
    cairo_set_source_rgb(cr, 0.85, 0.85, 0.82);
    for (x = 0; x < ctx->game->board.size; x++) {
        char s[16];
        snprintf(s, sizeof(s), "%c", 'a' + x);
        draw_centered_text(cr, pad + x * cell + cell / 2, pad + board_px + 12, s, 10.0);
    }
    for (y = 0; y < ctx->game->board.size; y++) {
        char s[16];
        snprintf(s, sizeof(s), "%d", y + 1);
        draw_centered_text(cr, pad - 12, pad + (ctx->game->board.size - 1 - y) * cell + cell / 2, s, 10.0);
    }
    return FALSE;
}

static bool screen_to_coord(GuiCtx *ctx, GtkWidget *widget, double sx, double sy, CfCoord *out) {
    int layer = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "layer-id"));
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);
    double pad = 28.0;
    double board_px;
    double cell;
    int x, y;
    if (!ctx->game) return false;
    board_px = (w < h ? w : h) - pad * 2.0;
    cell = board_px / ctx->game->board.size;
    x = (int)((sx - pad) / cell);
    y = ctx->game->board.size - 1 - (int)((sy - pad) / cell);
    out->layer = layer;
    out->x = x;
    out->y = y;
    return cf_board_is_playable(&ctx->game->board, *out);
}

static bool move_needs_dice(const CfPiece *piece) {
    return piece->type == CF_PIECE_QUEEN || piece->type == CF_PIECE_ROOK ||
           piece->type == CF_PIECE_BISHOP || piece->type == CF_PIECE_PRINCE;
}

static void select_gui_piece(GuiCtx *ctx, CfCoord c, int p) {
    char sq[16], label[8], msg[220], fields[512];
    ctx->selected = true;
    ctx->selected_coord = c;
    ctx->legal_count = cf_engine_generate_moves(ctx->game, ctx->game->current_player, c, ctx->legal_moves, CF_MAX_MOVES);
    cf_coord_to_string(&ctx->game->board, c, sq, sizeof(sq));
    piece_label(&ctx->game->pieces[p], label, sizeof(label));
    if (!ctx->game->dice_rolled) {
        snprintf(msg, sizeof(msg), "[Turn %d] %s selected %s at %s. Roll dice before moving.",
                 ctx->game->turn_id, ctx->game->houses[ctx->game->pieces[p].team].house_name, label, sq);
    } else {
        snprintf(msg, sizeof(msg), "[Turn %d] %s selected %s at %s. %d legal move%s shown.",
                 ctx->game->turn_id, ctx->game->houses[ctx->game->pieces[p].team].house_name, label, sq,
                 ctx->legal_count, ctx->legal_count == 1 ? "" : "s");
    }
    snprintf(fields, sizeof(fields), "\"piece_id\":%d,\"piece_role\":\"%s\",\"square\":\"%s\",\"legal_count\":%d,\"human_readable_summary\":\"%s\"",
             ctx->game->pieces[p].id, cf_piece_role_name(ctx->game->pieces[p].role), sq, ctx->legal_count, msg);
    cf_log_event(ctx->game, "piece_selected", fields);
    set_status_only(ctx, msg);
}

static void legal_destinations_text(GuiCtx *ctx, char *out, int out_size) {
    int i, n = 0;
    if (!out || out_size <= 0) return;
    out[0] = '\0';
    for (i = 0; i < ctx->legal_count && i < 12 && n < out_size - 8; i++) {
        char sq[16];
        cf_coord_to_string(&ctx->game->board, ctx->legal_moves[i].to, sq, sizeof(sq));
        n += snprintf(out + n, (size_t)(out_size - n), "%s%s", i ? ", " : "", sq);
    }
    if (ctx->legal_count > 12 && n < out_size - 5) snprintf(out + n, (size_t)(out_size - n), ", ...");
}

static gboolean on_board_click(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    GuiCtx *ctx = data;
    CfCoord c;
    int p, i;
    if (!ctx->game || event->button != 1 || !screen_to_coord(ctx, widget, event->x, event->y, &c)) {
        set_status(ctx, "Invalid square.");
        return TRUE;
    }
    p = cf_engine_piece_at(ctx->game, c);
    if (!ctx->selected) {
        if (p < 0) {
            set_status(ctx, "No piece on selected square.");
            return TRUE;
        }
        if (ctx->game->pieces[p].team != ctx->game->players[ctx->game->current_player].team) {
            set_status(ctx, "That piece does not belong to the current player team.");
            return TRUE;
        }
        select_gui_piece(ctx, c, p);
        refresh_labels(ctx);
        return TRUE;
    }
    if (p >= 0 && ctx->game->pieces[p].team == ctx->game->players[ctx->game->current_player].team) {
        if (coord_equal(ctx->selected_coord, c)) {
            ctx->selected = false;
            ctx->legal_count = 0;
            set_status(ctx, "Selection cleared.");
            refresh_labels(ctx);
            return TRUE;
        }
        select_gui_piece(ctx, c, p);
        refresh_labels(ctx);
        return TRUE;
    }
    for (i = 0; i < ctx->legal_count; i++) {
        if (move_targets(&ctx->legal_moves[i], c)) {
            char err[128];
            int piece_index = cf_engine_piece_at(ctx->game, ctx->selected_coord);
            if (piece_index >= 0 && move_needs_dice(&ctx->game->pieces[piece_index]) && !ctx->game->dice_rolled) {
                set_status(ctx, "Roll dice before sliding movement.");
                return TRUE;
            }
            ctx->last_from = ctx->selected_coord;
            ctx->last_to = c;
            ctx->has_last_move = true;
            if (cf_engine_apply_move(ctx->game, &ctx->legal_moves[i], err, sizeof(err))) {
                char a[16], b[16], label[8], msg[180];
                cf_coord_to_string(&ctx->game->board, ctx->last_from, a, sizeof(a));
                cf_coord_to_string(&ctx->game->board, ctx->last_to, b, sizeof(b));
                piece_label(&ctx->game->pieces[piece_index], label, sizeof(label));
                snprintf(msg, sizeof(msg), "[Turn %d] %s moved %s from %s to %s.",
                         ctx->game->turn_id - 1, ctx->game->houses[ctx->game->pieces[piece_index].team].house_name, label, a, b);
                set_status_only(ctx, msg);
            } else set_status(ctx, err);
            ctx->selected = false;
            ctx->legal_count = 0;
            refresh_labels(ctx);
            return TRUE;
        }
    }
    {
        char options[220], msg[300];
        legal_destinations_text(ctx, options, sizeof(options));
        snprintf(msg, sizeof(msg), "Illegal move.%s%s", options[0] ? " Legal destinations: " : "", options);
        set_status(ctx, msg);
    }
    refresh_labels(ctx);
    return TRUE;
}

static void on_start(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    CfConfig config;
    char *teams;
    char *players;
    int active;
    (void)button;
    memset(&config, 0, sizeof(config));
    strncpy(config.mode, "board-gui", sizeof(config.mode) - 1);
    active = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->team_combo));
    config.team_count = active == 1 ? 4 : active == 2 ? 6 : active == 3 ? 8 : 2;
    config.time_travel = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->time_check));
    teams = buffer_text(ctx->team_buffer);
    players = buffer_text(ctx->player_buffer);
    copy_lines_to_names(teams, config.team_names, config.team_count, "Team");
    copy_lines_to_names(players, config.player_names, config.team_count * 3, "Player");
    g_free(teams);
    g_free(players);
    if (ctx->game) cf_engine_free(ctx->game);
    ctx->game = cf_engine_new(&config);
    ctx->selected = false;
    ctx->legal_count = 0;
    ctx->has_last_move = false;
    if (!ctx->game) {
        set_status(ctx, "Failed to start session.");
        return;
    }
    load_existing_log(ctx);
    cf_log_set_sink(ctx->game, gui_log_event_sink, ctx);
    if (config.team_count == 6) set_status(ctx, "6-House layout is experimental/WIP; using placeholder cross setup.");
    gtk_widget_set_visible(ctx->branch_button, config.time_travel);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->notebook), 1);
    gui_log(ctx, ctx->game->log_path);
    refresh_labels(ctx);
}

static void on_roll(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    char fields[768], line[160];
    CfPlayer *p;
    (void)button;
    if (!ctx->game) return;
    if (ctx->game->dice_rolled) {
        set_status(ctx, "Dice already rolled this turn.");
        return;
    }
    p = &ctx->game->players[ctx->game->current_player];
    cf_roll_custom_dice(&ctx->game->die_a, &ctx->game->die_b, &ctx->game->dice_sum);
    ctx->game->dice_rolled = true;
    snprintf(fields, sizeof(fields), "\"player_id\":%d,\"dice_a\":%d,\"dice_b\":%d,\"dice_sum\":%d,\"human_readable_summary\":\"%s %s rolled %d + %d = %d.\"",
             p->player_id, ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum,
             ctx->game->houses[p->team].house_name, p->role, ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    cf_log_event(ctx->game, "dice_roll", fields);
    snprintf(line, sizeof(line), "[Turn %d] %s %s rolled %d + %d = %d.",
             ctx->game->turn_id, ctx->game->houses[p->team].house_name, p->role, ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    set_status_only(ctx, line);
    refresh_labels(ctx);
}

static void on_branch(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    char path[256];
    GtkWidget *dialog, *entry, *content;
    int response, turn_id;
    (void)button;
    if (!ctx->game) return;
    dialog = gtk_dialog_new_with_buttons("Create Branch", GTK_WINDOW(ctx->window), GTK_DIALOG_MODAL,
                                         "Create Branch", GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Turn ID");
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
    gtk_widget_show_all(dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    turn_id = atoi(gtk_entry_get_text(GTK_ENTRY(entry)));
    gtk_widget_destroy(dialog);
    if (response != GTK_RESPONSE_OK) return;
    if (cf_engine_branch(ctx->game, turn_id, path, sizeof(path))) {
        char msg[360];
        snprintf(msg, sizeof(msg), "Branch created from session %s at turn %d: %s", ctx->game->session_id, turn_id, path);
        set_status(ctx, msg);
    }
    else set_status(ctx, "Time travel is disabled or branch failed.");
}

static gboolean on_board_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer data) {
    GuiCtx *ctx = data;
    CfCoord c;
    int p;
    char sq[16], text[384];
    (void)keyboard_mode;
    if (!ctx->game || !screen_to_coord(ctx, widget, x, y, &c)) return FALSE;
    p = cf_engine_piece_at(ctx->game, c);
    if (p < 0) return FALSE;
    cf_coord_to_string(&ctx->game->board, c, sq, sizeof(sq));
    snprintf(text, sizeof(text), "%s\n%s\nController: %s\nSquare: %s\nState: %s%s%s",
             ctx->game->pieces[p].identity,
             ctx->game->houses[ctx->game->pieces[p].team].motto,
             ctx->game->players[ctx->game->pieces[p].team * 3].name,
             sq,
             ctx->game->pieces[p].alive ? "alive" : "dead",
             ctx->game->pieces[p].widow ? ", widow" : "",
             ctx->game->pieces[p].frozen ? ", frozen" : "");
    gtk_tooltip_set_text(tooltip, text);
    return TRUE;
}

static void set_help(GuiCtx *ctx) {
    const char *help =
        "CrownFall: Dice Court\n\n"
        "The Eight Houses:\n"
        "House Albatross - Beyond All Horizons\n"
        "House Dragon - Reality Yields\n"
        "House Crocodile - Patience Consumes\n"
        "House Cobra - One Strike Suffices\n"
        "House Owl - Knowledge Devours\n"
        "House Eagle - None Escape Our Sight\n"
        "House Wolf - Together We Hunt\n"
        "House Lion - By Strength We Reign\n\n"
        "Boards: 2 teams use 9x9. 4 teams use a 15x15 cross with four 3x3 holes. 8 teams use two cross layers.\n\n"
        "Setup: South, East, North, and West courts each receive a rotated House legion. 8-team mode repeats this on L2.\n\n"
        "Pieces: K king, QL/QR queens, PL/PR crown princes, L love interest, RL/RR rooks, NL/NR knights, BL/BR bishops, p1-p6 pawns.\n\n"
        "Movement: kings and love interests move one square. Queens, rooks, bishops, and prince rook-moves are dice-limited. Knights jump. Princes also knight-jump. Pawns move forward and capture diagonally.\n\n"
        "Dice: roll Die A and Die B before sliding movement. The sum limits sliding distance.\n\n"
        "Royal rules: Prince cannot capture Prince. Queen can kill Prince. Prince can kill Queen unless Mercy Pact blocks it.\n\n"
        "Mercy Pact: sparing a legally capturable enemy Prince twice forms a pact. Bloodfall, Widow Queen, Love Interest, Sacred Intercession, Ascension, check/checkmate, and full replay remain advanced engine TODOs with logged extension points.\n\n"
        "Time travel: branch creates a new branch session from a selected turn without mutating old logs.";
    gtk_text_buffer_set_text(ctx->help_buffer, help, -1);
}

int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    GtkBuilder *builder;
    GtkWidget *start;
    GtkWidget *roll;
    GtkWidget *defaults;
    GtkWidget *randomize;
    GtkWidget *random_empty;
    GuiCtx *ctx;
    int i;
    gtk_init(argc, argv);
    builder = gtk_builder_new_from_file("ui/crownfall.glade");
    ctx = g_new0(GuiCtx, 1);
    ctx->game = game;
    ctx->window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    ctx->notebook = GTK_WIDGET(gtk_builder_get_object(builder, "main_tabs"));
    ctx->mode_combo = GTK_WIDGET(gtk_builder_get_object(builder, "mode_combo"));
    ctx->team_combo = GTK_WIDGET(gtk_builder_get_object(builder, "team_count_combo"));
    ctx->time_check = GTK_WIDGET(gtk_builder_get_object(builder, "time_travel_check"));
    ctx->turn_label = GTK_WIDGET(gtk_builder_get_object(builder, "turn_label"));
    ctx->dice_label = GTK_WIDGET(gtk_builder_get_object(builder, "dice_label"));
    ctx->status_label = GTK_WIDGET(gtk_builder_get_object(builder, "status_label"));
    ctx->branch_button = GTK_WIDGET(gtk_builder_get_object(builder, "branch_button"));
    ctx->filter_combo = GTK_WIDGET(gtk_builder_get_object(builder, "log_filter_combo"));
    ctx->layer_box[0] = GTK_WIDGET(gtk_builder_get_object(builder, "layer1_box"));
    ctx->layer_box[1] = GTK_WIDGET(gtk_builder_get_object(builder, "layer2_box"));
    ctx->layer_area[0] = GTK_WIDGET(gtk_builder_get_object(builder, "layer1_area"));
    ctx->layer_area[1] = GTK_WIDGET(gtk_builder_get_object(builder, "layer2_area"));
    ctx->team_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(builder, "team_names_view")));
    ctx->player_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(builder, "player_names_view")));
    ctx->log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(builder, "log_view")));
    ctx->help_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(builder, "help_view")));
    start = GTK_WIDGET(gtk_builder_get_object(builder, "start_button"));
    roll = GTK_WIDGET(gtk_builder_get_object(builder, "roll_button"));
    defaults = GTK_WIDGET(gtk_builder_get_object(builder, "default_names_button"));
    randomize = GTK_WIDGET(gtk_builder_get_object(builder, "random_names_button"));
    random_empty = GTK_WIDGET(gtk_builder_get_object(builder, "random_empty_names_button"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->team_combo), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->mode_combo), 0);
    if (ctx->filter_combo) gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->filter_combo), 0);
    fill_default_names(ctx);
    set_help(ctx);
    g_signal_connect(ctx->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(start, "clicked", G_CALLBACK(on_start), ctx);
    g_signal_connect(roll, "clicked", G_CALLBACK(on_roll), ctx);
    g_signal_connect(defaults, "clicked", G_CALLBACK(on_default_names), ctx);
    g_signal_connect(randomize, "clicked", G_CALLBACK(on_random_names), ctx);
    if (random_empty) g_signal_connect(random_empty, "clicked", G_CALLBACK(on_random_names), ctx);
    g_signal_connect(ctx->branch_button, "clicked", G_CALLBACK(on_branch), ctx);
    for (i = 0; i < GUI_MAX_LAYER_AREAS; i++) {
        g_object_set_data(G_OBJECT(ctx->layer_area[i]), "layer-id", GINT_TO_POINTER(i));
        gtk_widget_set_size_request(ctx->layer_area[i], 520, 520);
        gtk_widget_add_events(ctx->layer_area[i], GDK_BUTTON_PRESS_MASK);
        gtk_widget_set_has_tooltip(ctx->layer_area[i], TRUE);
        g_signal_connect(ctx->layer_area[i], "draw", G_CALLBACK(on_board_draw), ctx);
        g_signal_connect(ctx->layer_area[i], "button-press-event", G_CALLBACK(on_board_click), ctx);
        g_signal_connect(ctx->layer_area[i], "query-tooltip", G_CALLBACK(on_board_tooltip), ctx);
    }
    gtk_widget_set_visible(ctx->branch_button, FALSE);
    gtk_widget_set_visible(ctx->layer_box[1], FALSE);
    if (ctx->game) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->notebook), 1);
        refresh_labels(ctx);
    }
    gtk_widget_show_all(ctx->window);
    if (!ctx->game) gtk_widget_set_visible(ctx->layer_box[1], FALSE);
    gtk_main();
    if (ctx->game && ctx->game != game) cf_engine_free(ctx->game);
    g_free(ctx);
    g_object_unref(builder);
    return 0;
}
#else
#include <stdio.h>
int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    (void)game;
    (void)argc;
    (void)argv;
    puts("GTK3 support was not compiled in. Install GTK3 development packages and rebuild, or run ./bin/crownfall --cli.");
    return 1;
}
#endif
