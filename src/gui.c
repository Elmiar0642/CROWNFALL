#include "gui.h"

#ifdef CROWNFALL_HAVE_GTK
#include "dice.h"
#include "log.h"
#include <gtk/gtk.h>
<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    GtkTextBuffer *board_buffer;
    GtkTextBuffer *log_buffer;
    GtkWidget *from_entry;
    GtkWidget *to_entry;
    GtkWidget *branch_entry;
=======

typedef struct {
    CfGame *game;
    GtkWidget *turn_label;
    GtkWidget *dice_label;
    GtkTextBuffer *log_buffer;
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)
} GuiCtx;

static void append_log(GuiCtx *ctx, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(ctx->log_buffer, &end);
    gtk_text_buffer_insert(ctx->log_buffer, &end, text, -1);
    gtk_text_buffer_insert(ctx->log_buffer, &end, "\n", -1);
}

<<<<<<< HEAD
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

static void board_to_buffer(const CfGame *game, char *out, int out_size) {
    int l, y, x, n = 0;
    if (!game || !out || out_size <= 0) return;
    out[0] = '\0';
    for (l = 0; l < game->board.layers && n < out_size - 1; l++) {
        if (game->board.layers > 1) n += snprintf(out + n, (size_t)(out_size - n), "Layer L%d\n", l + 1);
        for (y = game->board.size - 1; y >= 0 && n < out_size - 1; y--) {
            n += snprintf(out + n, (size_t)(out_size - n), "%2d ", y + 1);
            for (x = 0; x < game->board.size && n < out_size - 1; x++) {
                CfCoord c = {l, x, y};
                int p = cf_engine_piece_at(game, c);
                if (!cf_board_is_playable(&game->board, c)) n += snprintf(out + n, (size_t)(out_size - n), " ##");
                else if (p >= 0) n += snprintf(out + n, (size_t)(out_size - n), " %c%d", game->pieces[p].label[0], game->pieces[p].team + 1);
                else n += snprintf(out + n, (size_t)(out_size - n), " ..");
            }
            n += snprintf(out + n, (size_t)(out_size - n), "\n");
        }
        n += snprintf(out + n, (size_t)(out_size - n), "   ");
        for (x = 0; x < game->board.size && n < out_size - 1; x++) n += snprintf(out + n, (size_t)(out_size - n), " %c ", 'a' + x);
        n += snprintf(out + n, (size_t)(out_size - n), "\n\n");
    }
}

static void refresh_view(GuiCtx *ctx) {
    char buf[16384];
    CfPlayer *p;
    if (!ctx->game) return;
    p = &ctx->game->players[ctx->game->current_player];
    snprintf(buf, sizeof(buf), "Turn %d: %s / Team %d / %s", ctx->game->turn_id, p->name, p->team + 1, p->role);
    gtk_label_set_text(GTK_LABEL(ctx->turn_label), buf);
    snprintf(buf, sizeof(buf), "Dice: %d + %d = %d%s", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum,
             ctx->game->dice_rolled ? "" : " (not rolled)");
    gtk_label_set_text(GTK_LABEL(ctx->dice_label), buf);
    board_to_buffer(ctx->game, buf, sizeof(buf));
    gtk_text_buffer_set_text(ctx->board_buffer, buf, -1);
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
    if (!ctx->game) {
        append_log(ctx, "Failed to start session.");
        return;
    }
    gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->notebook), 1);
    append_log(ctx, "Session started.");
    append_log(ctx, ctx->game->log_path);
    refresh_view(ctx);
=======
static void refresh_labels(GuiCtx *ctx) {
    char buf[256];
    CfPlayer *p = &ctx->game->players[ctx->game->current_player];
    snprintf(buf, sizeof(buf), "Turn %d: %s / Team %d / %s", ctx->game->turn_id, p->name, p->team + 1, p->role);
    gtk_label_set_text(GTK_LABEL(ctx->turn_label), buf);
    snprintf(buf, sizeof(buf), "Dice: %d + %d = %d%s", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum,
             ctx->game->dice_rolled ? "" : " (roll needed)");
    gtk_label_set_text(GTK_LABEL(ctx->dice_label), buf);
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)
}

static void on_roll(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
<<<<<<< HEAD
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
    snprintf(line, sizeof(line), "Rolled %d + %d = %d", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    append_log(ctx, line);
    refresh_view(ctx);
}

static void on_legal(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    CfCoord from;
    CfMove moves[CF_MAX_MOVES];
    const char *from_text;
    int i, n;
    (void)button;
    if (!ctx->game) return;
    from_text = gtk_entry_get_text(GTK_ENTRY(ctx->from_entry));
    if (!cf_parse_coord(&ctx->game->board, from_text, &from)) {
        append_log(ctx, "Invalid or unplayable source square.");
        return;
    }
    n = cf_engine_generate_moves(ctx->game, ctx->game->current_player, from, moves, CF_MAX_MOVES);
    if (!n) append_log(ctx, "No legal moves.");
    for (i = 0; i < n; i++) append_log(ctx, moves[i].notation);
}

static void on_move(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    CfCoord from, to;
    CfMove moves[CF_MAX_MOVES];
    char err[128];
    int i, n;
    const char *from_text;
    const char *to_text;
    (void)button;
    if (!ctx->game) return;
    from_text = gtk_entry_get_text(GTK_ENTRY(ctx->from_entry));
    to_text = gtk_entry_get_text(GTK_ENTRY(ctx->to_entry));
    if (!cf_parse_coord(&ctx->game->board, from_text, &from) || !cf_parse_coord(&ctx->game->board, to_text, &to)) {
        append_log(ctx, "Invalid or unplayable move coordinate.");
        return;
    }
    n = cf_engine_generate_moves(ctx->game, ctx->game->current_player, from, moves, CF_MAX_MOVES);
    for (i = 0; i < n; i++) {
        if (moves[i].to.layer == to.layer && moves[i].to.x == to.x && moves[i].to.y == to.y) {
            if (cf_engine_apply_move(ctx->game, &moves[i], err, sizeof(err))) append_log(ctx, "Move accepted.");
            else append_log(ctx, err);
            refresh_view(ctx);
            return;
        }
    }
    append_log(ctx, "Illegal move.");
}

static void on_branch(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    char path[256];
    int turn_id;
    (void)button;
    if (!ctx->game) return;
    turn_id = atoi(gtk_entry_get_text(GTK_ENTRY(ctx->branch_entry)));
    if (cf_engine_branch(ctx->game, turn_id, path, sizeof(path))) append_log(ctx, path);
    else append_log(ctx, "Time travel is disabled or branch failed.");
=======
    char fields[160], line[80];
    (void)button;
    cf_roll_custom_dice(&ctx->game->die_a, &ctx->game->die_b, &ctx->game->dice_sum);
    ctx->game->dice_rolled = true;
    snprintf(fields, sizeof(fields), "\"die_a\":%d,\"die_b\":%d,\"sum\":%d", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    cf_log_event(ctx->game, "dice_roll", fields);
    snprintf(line, sizeof(line), "Rolled %d + %d = %d", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    append_log(ctx, line);
    refresh_labels(ctx);
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)
}

int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    GtkBuilder *builder;
<<<<<<< HEAD
    GtkWidget *roll;
    GtkWidget *legal;
    GtkWidget *move;
    GtkWidget *branch;
    GtkWidget *start;
    GtkWidget *team_view;
    GtkWidget *player_view;
    GtkWidget *board_view;
    GtkWidget *log_view;
    GuiCtx *ctx;
    gtk_init(argc, argv);
    builder = gtk_builder_new_from_file("ui/crownfall.glade");
    ctx = g_new0(GuiCtx, 1);
    ctx->game = game;
    ctx->window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    ctx->notebook = GTK_WIDGET(gtk_builder_get_object(builder, "main_tabs"));
    ctx->team_combo = GTK_WIDGET(gtk_builder_get_object(builder, "team_count_combo"));
    ctx->time_check = GTK_WIDGET(gtk_builder_get_object(builder, "time_travel_check"));
    team_view = GTK_WIDGET(gtk_builder_get_object(builder, "team_names_view"));
    player_view = GTK_WIDGET(gtk_builder_get_object(builder, "player_names_view"));
    board_view = GTK_WIDGET(gtk_builder_get_object(builder, "board_view"));
    log_view = GTK_WIDGET(gtk_builder_get_object(builder, "log_view"));
    ctx->turn_label = GTK_WIDGET(gtk_builder_get_object(builder, "turn_label"));
    ctx->dice_label = GTK_WIDGET(gtk_builder_get_object(builder, "dice_label"));
    ctx->from_entry = GTK_WIDGET(gtk_builder_get_object(builder, "from_entry"));
    ctx->to_entry = GTK_WIDGET(gtk_builder_get_object(builder, "to_entry"));
    ctx->branch_entry = GTK_WIDGET(gtk_builder_get_object(builder, "branch_entry"));
    ctx->team_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(team_view));
    ctx->player_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(player_view));
    ctx->board_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(board_view));
    ctx->log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    start = GTK_WIDGET(gtk_builder_get_object(builder, "start_button"));
    roll = GTK_WIDGET(gtk_builder_get_object(builder, "roll_button"));
    legal = GTK_WIDGET(gtk_builder_get_object(builder, "legal_button"));
    move = GTK_WIDGET(gtk_builder_get_object(builder, "move_button"));
    branch = GTK_WIDGET(gtk_builder_get_object(builder, "branch_button"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->team_combo), 0);
    g_signal_connect(ctx->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(start, "clicked", G_CALLBACK(on_start), ctx);
    g_signal_connect(roll, "clicked", G_CALLBACK(on_roll), ctx);
    g_signal_connect(legal, "clicked", G_CALLBACK(on_legal), ctx);
    g_signal_connect(move, "clicked", G_CALLBACK(on_move), ctx);
    g_signal_connect(branch, "clicked", G_CALLBACK(on_branch), ctx);
    if (ctx->game) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->notebook), 1);
        refresh_view(ctx);
        append_log(ctx, ctx->game->log_path);
    }
    gtk_widget_show_all(ctx->window);
    gtk_main();
    if (ctx->game && ctx->game != game) cf_engine_free(ctx->game);
    g_free(ctx);
=======
    GtkWidget *window;
    GtkWidget *roll;
    GtkWidget *view;
    GuiCtx ctx;
    gtk_init(argc, argv);
    builder = gtk_builder_new_from_file("ui/crownfall.glade");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    roll = GTK_WIDGET(gtk_builder_get_object(builder, "roll_button"));
    view = GTK_WIDGET(gtk_builder_get_object(builder, "log_view"));
    ctx.game = game;
    ctx.turn_label = GTK_WIDGET(gtk_builder_get_object(builder, "turn_label"));
    ctx.dice_label = GTK_WIDGET(gtk_builder_get_object(builder, "dice_label"));
    ctx.log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_builder_connect_signals(builder, NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(roll, "clicked", G_CALLBACK(on_roll), &ctx);
    append_log(&ctx, "CrownFall GUI skeleton ready. Use CLI for full notation play.");
    refresh_labels(&ctx);
    gtk_widget_show_all(window);
    gtk_main();
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)
    g_object_unref(builder);
    return 0;
}
#else
#include <stdio.h>
int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    (void)game;
    (void)argc;
    (void)argv;
<<<<<<< HEAD
    puts("GTK3 support was not compiled in. Install GTK3 development packages and rebuild, or run ./bin/crownfall --cli.");
=======
    puts("GTK3 support was not available at build time. Rebuild with GTK3 development packages installed.");
>>>>>>> b1b79ed (Initial CrownFall engine scaffold)
    return 1;
}
#endif
