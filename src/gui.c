#include "gui.h"

#ifdef CROWNFALL_HAVE_GTK
#include "dice.h"
#include "log.h"
#include <gtk/gtk.h>

typedef struct {
    CfGame *game;
    GtkWidget *turn_label;
    GtkWidget *dice_label;
    GtkTextBuffer *log_buffer;
} GuiCtx;

static void append_log(GuiCtx *ctx, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(ctx->log_buffer, &end);
    gtk_text_buffer_insert(ctx->log_buffer, &end, text, -1);
    gtk_text_buffer_insert(ctx->log_buffer, &end, "\n", -1);
}

static void refresh_labels(GuiCtx *ctx) {
    char buf[256];
    CfPlayer *p = &ctx->game->players[ctx->game->current_player];
    snprintf(buf, sizeof(buf), "Turn %d: %s / Team %d / %s", ctx->game->turn_id, p->name, p->team + 1, p->role);
    gtk_label_set_text(GTK_LABEL(ctx->turn_label), buf);
    snprintf(buf, sizeof(buf), "Dice: %d + %d = %d%s", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum,
             ctx->game->dice_rolled ? "" : " (roll needed)");
    gtk_label_set_text(GTK_LABEL(ctx->dice_label), buf);
}

static void on_roll(GtkButton *button, gpointer data) {
    GuiCtx *ctx = data;
    char fields[160], line[80];
    (void)button;
    cf_roll_custom_dice(&ctx->game->die_a, &ctx->game->die_b, &ctx->game->dice_sum);
    ctx->game->dice_rolled = true;
    snprintf(fields, sizeof(fields), "\"die_a\":%d,\"die_b\":%d,\"sum\":%d", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    cf_log_event(ctx->game, "dice_roll", fields);
    snprintf(line, sizeof(line), "Rolled %d + %d = %d", ctx->game->die_a, ctx->game->die_b, ctx->game->dice_sum);
    append_log(ctx, line);
    refresh_labels(ctx);
}

int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    GtkBuilder *builder;
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
    g_object_unref(builder);
    return 0;
}
#else
#include <stdio.h>
int cf_gui_run(CfGame *game, int *argc, char ***argv) {
    (void)game;
    (void)argc;
    (void)argv;
    puts("GTK3 support was not available at build time. Rebuild with GTK3 development packages installed.");
    return 1;
}
#endif
