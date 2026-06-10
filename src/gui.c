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
    GtkWidget *team_combo;
    GtkWidget *time_check;
    GtkTextBuffer *team_buffer;
    GtkTextBuffer *player_buffer;
    GtkWidget *turn_label;
    GtkWidget *dice_label;
    GtkWidget *status_label;
    GtkWidget *branch_button;
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

static const char *RANDOM_NAMES[] = {
    "Aurelian", "Veyra", "Kaelen", "Myrr", "Osric", "Selene", "Thane",
    "Ilyra", "Corvin", "Maera", "Draven", "Eryndor", "Nyx", "Vael",
    "Soren", "Elara", "Arctus", "Neria", "Oryn", "Lyssa"
};

static const char *DEFAULT_PLAYER_NAMES[24] = {
    "White Court", "White Left House", "White Right House",
    "Black Court", "Black Left House", "Black Right House",
    "North Court", "North Left House", "North Right House",
    "West Court", "West Left House", "West Right House",
    "Aurelian Court", "Aurelian Left House", "Aurelian Right House",
    "Veyra Court", "Veyra Left House", "Veyra Right House",
    "Kaelen Court", "Kaelen Left House", "Kaelen Right House",
    "Myrr Court", "Myrr Left House", "Myrr Right House"
};

static const char *DEFAULT_TEAM_NAMES[8] = {
    "White", "Black", "North", "West", "Aurelian", "Veyra", "Kaelen", "Myrr"
};

static void gui_log(GuiCtx *ctx, const char *text) {
    GtkTextIter end;
    if (!ctx || !ctx->log_buffer || !text) return;
    gtk_text_buffer_get_end_iter(ctx->log_buffer, &end);
    gtk_text_buffer_insert(ctx->log_buffer, &end, text, -1);
    gtk_text_buffer_insert(ctx->log_buffer, &end, "\n", -1);
}

static void set_status(GuiCtx *ctx, const char *text) {
    if (ctx && ctx->status_label) gtk_label_set_text(GTK_LABEL(ctx->status_label), text ? text : "");
    if (text && text[0]) gui_log(ctx, text);
}

static void set_text_buffer_lines(GtkTextBuffer *buffer, const char **values, int count) {
    int i;
    GString *s = g_string_new("");
    for (i = 0; i < count; i++) g_string_append_printf(s, "%s\n", values[i]);
    gtk_text_buffer_set_text(buffer, s->str, -1);
    g_string_free(s, TRUE);
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
    while (line && i < max_names) {
        while (*line == ' ' || *line == '\t') line++;
        if (*line) strncpy(names[i], line, CF_MAX_NAME - 1);
        if (!names[i][0]) snprintf(names[i], CF_MAX_NAME, "%s %d", prefix, i + 1);
        i++;
        line = strtok(NULL, "\n\r");
    }
    while (i < max_names) {
        snprintf(names[i], CF_MAX_NAME, "%s %d", prefix, i + 1);
        i++;
    }
}

static void fill_default_names(GuiCtx *ctx) {
    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->team_combo));
    int teams = active == 1 ? 4 : active == 2 ? 6 : active == 3 ? 8 : 2;
    set_text_buffer_lines(ctx->team_buffer, DEFAULT_TEAM_NAMES, teams);
    set_text_buffer_lines(ctx->player_buffer, DEFAULT_PLAYER_NAMES, teams * 3);
    set_status(ctx, "Default names loaded.");
}

static void fill_random_names(GuiCtx *ctx) {
    int i;
    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->team_combo));
    int teams = active == 1 ? 4 : active == 2 ? 6 : active == 3 ? 8 : 2;
    GString *team = g_string_new("");
    GString *players = g_string_new("");
    srand(1);
    for (i = 0; i < teams; i++) {
        g_string_append_printf(team, "%s Court\n", RANDOM_NAMES[rand() % 20]);
    }
    for (i = 0; i < teams * 3; i++) {
        g_string_append_printf(players, "%s %s\n", RANDOM_NAMES[rand() % 20],
                               i % 3 == 0 ? "Crown" : i % 3 == 1 ? "Left" : "Right");
    }
    gtk_text_buffer_set_text(ctx->team_buffer, team->str, -1);
    gtk_text_buffer_set_text(ctx->player_buffer, players->str, -1);
    g_string_free(team, TRUE);
    g_string_free(players, TRUE);
    set_status(ctx, "Deterministic random names loaded.");
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
    snprintf(buf, sizeof(buf), "Turn %d: %s / Team %d / %s", ctx->game->turn_id, p->name, p->team + 1, p->role);
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
    const char *prefix = piece->label;
    if (piece->type == CF_PIECE_QUEEN) prefix = "Q";
    else if (piece->type == CF_PIECE_PRINCE) prefix = "P";
    else if (piece->type == CF_PIECE_PAWN) prefix = "p";
    snprintf(out, (size_t)out_size, "%s%d", prefix, piece->team + 1);
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
    draw_centered_text(cr, w / 2.0, 14.0, layer == 0 ? "Layer L1" : "Layer L2", 12.0);
    for (y = 0; y < ctx->game->board.size; y++) {
        for (x = 0; x < ctx->game->board.size; x++) {
            CfCoord c = {layer, x, y};
            double sx = pad + x * cell;
            double sy = pad + (ctx->game->board.size - 1 - y) * cell;
            int p = cf_engine_piece_at(ctx->game, c);
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
            if (p >= 0) {
                char label[8];
                piece_label(&ctx->game->pieces[p], label, sizeof(label));
                cairo_set_source_rgb(cr, 0.05, 0.05, 0.05);
                cairo_arc(cr, sx + cell / 2, sy + cell / 2, cell * 0.38, 0, 6.28318);
                cairo_fill(cr);
                cairo_set_source_rgb(cr, 0.94, 0.93, 0.86);
                draw_centered_text(cr, sx + cell / 2, sy + cell / 2, label, cell * 0.32);
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
        ctx->selected = true;
        ctx->selected_coord = c;
        ctx->legal_count = cf_engine_generate_moves(ctx->game, ctx->game->current_player, c, ctx->legal_moves, CF_MAX_MOVES);
        set_status(ctx, "Piece selected.");
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
            if (cf_engine_apply_move(ctx->game, &ctx->legal_moves[i], err, sizeof(err))) set_status(ctx, "Move completed.");
            else set_status(ctx, err);
            ctx->selected = false;
            ctx->legal_count = 0;
            refresh_labels(ctx);
            return TRUE;
        }
    }
    ctx->selected = false;
    ctx->legal_count = 0;
    set_status(ctx, "Illegal move.");
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
    gtk_widget_set_visible(ctx->branch_button, config.time_travel);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->notebook), 1);
    gui_log(ctx, "Turn started");
    gui_log(ctx, ctx->game->log_path);
    refresh_labels(ctx);
}

static void on_roll(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    char fields[256], line[128];
    CfPlayer *p;
    (void)button;
    if (!ctx->game) return;
    p = &ctx->game->players[ctx->game->current_player];
    cf_roll_custom_dice(&ctx->game->die_a, &ctx->game->die_b, &ctx->game->dice_sum);
    ctx->game->dice_rolled = true;
    snprintf(fields, sizeof(fields), "\"player\":\"%s\",\"player_id\":%d,\"die_a\":%d,\"die_b\":%d,\"sum\":%d",
             p->name, p->player_id, ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    cf_log_event(ctx->game, "dice_roll", fields);
    snprintf(line, sizeof(line), "Dice rolled: %d + %d = %d", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    set_status(ctx, line);
    refresh_labels(ctx);
}

static void on_branch(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    char path[256];
    (void)button;
    if (!ctx->game) return;
    if (cf_engine_branch(ctx->game, ctx->game->turn_id, path, sizeof(path))) set_status(ctx, path);
    else set_status(ctx, "Time travel is disabled or branch failed.");
}

static void set_help(GuiCtx *ctx) {
    const char *help =
        "CrownFall: Dice Court\n\n"
        "Boards: 2 teams use 9x9. 4 teams use a 15x15 cross with four 3x3 holes. 8 teams use two cross layers.\n\n"
        "Setup: South, East, North, and West courts each receive a rotated legion. 8-team mode repeats this on L2.\n\n"
        "Pieces: K king, Q queen, P prince, L love interest, R rook, N knight, B bishop, p pawn.\n\n"
        "Movement: kings and love interests move one square. Queens, rooks, bishops, and prince rook-moves are dice-limited. Knights jump. Princes also knight-jump. Pawns move forward and capture diagonally.\n\n"
        "Dice: roll Die A and Die B before sliding movement. The sum limits sliding distance.\n\n"
        "Royal rules: Prince cannot capture Prince. Queen can kill Prince. Prince can kill Queen unless Mercy Pact blocks it.\n\n"
        "Mercy Pact, Bloodfall, Widow Queen, Sacred Intercession, Ascension, check/checkmate, and full replay remain advanced engine TODOs with logged extension points.\n\n"
        "Time travel: branch creates a new branch session from a selected turn without mutating old logs.";
    gtk_text_buffer_set_text(ctx->help_buffer, help, -1);
}

int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    GtkBuilder *builder;
    GtkWidget *start;
    GtkWidget *roll;
    GtkWidget *defaults;
    GtkWidget *randomize;
    GuiCtx *ctx;
    int i;
    gtk_init(argc, argv);
    builder = gtk_builder_new_from_file("ui/crownfall.glade");
    ctx = g_new0(GuiCtx, 1);
    ctx->game = game;
    ctx->window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    ctx->notebook = GTK_WIDGET(gtk_builder_get_object(builder, "main_tabs"));
    ctx->team_combo = GTK_WIDGET(gtk_builder_get_object(builder, "team_count_combo"));
    ctx->time_check = GTK_WIDGET(gtk_builder_get_object(builder, "time_travel_check"));
    ctx->turn_label = GTK_WIDGET(gtk_builder_get_object(builder, "turn_label"));
    ctx->dice_label = GTK_WIDGET(gtk_builder_get_object(builder, "dice_label"));
    ctx->status_label = GTK_WIDGET(gtk_builder_get_object(builder, "status_label"));
    ctx->branch_button = GTK_WIDGET(gtk_builder_get_object(builder, "branch_button"));
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
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->team_combo), 0);
    fill_default_names(ctx);
    set_help(ctx);
    g_signal_connect(ctx->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(start, "clicked", G_CALLBACK(on_start), ctx);
    g_signal_connect(roll, "clicked", G_CALLBACK(on_roll), ctx);
    g_signal_connect(defaults, "clicked", G_CALLBACK(on_default_names), ctx);
    g_signal_connect(randomize, "clicked", G_CALLBACK(on_random_names), ctx);
    g_signal_connect(ctx->branch_button, "clicked", G_CALLBACK(on_branch), ctx);
    for (i = 0; i < GUI_MAX_LAYER_AREAS; i++) {
        g_object_set_data(G_OBJECT(ctx->layer_area[i]), "layer-id", GINT_TO_POINTER(i));
        gtk_widget_set_size_request(ctx->layer_area[i], 520, 520);
        gtk_widget_add_events(ctx->layer_area[i], GDK_BUTTON_PRESS_MASK);
        g_signal_connect(ctx->layer_area[i], "draw", G_CALLBACK(on_board_draw), ctx);
        g_signal_connect(ctx->layer_area[i], "button-press-event", G_CALLBACK(on_board_click), ctx);
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
