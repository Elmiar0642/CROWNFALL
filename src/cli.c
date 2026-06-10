#include "cli.h"

#include "dice.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_newline(char *s) {
    size_t n;
    if (!s) return;
    n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

static void read_line(const char *prompt, char *out, int size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(out, size, stdin)) {
        out[0] = '\0';
        return;
    }
    trim_newline(out);
}

void cf_prompt_config(CfConfig *config) {
    char line[128];
    int i, players;
    memset(config, 0, sizeof(*config));
    read_line("Mode (text/board-gui) [text]: ", line, sizeof(line));
    strncpy(config->mode, line[0] ? line : "text", sizeof(config->mode) - 1);
    read_line("Team count (2/4/6/8) [2]: ", line, sizeof(line));
    config->team_count = atoi(line);
    if (!(config->team_count == 2 || config->team_count == 4 || config->team_count == 6 || config->team_count == 8)) config->team_count = 2;
    read_line("Time travel (enable/disable) [disable]: ", line, sizeof(line));
    config->time_travel = line[0] == 'e' || line[0] == 'E' || strcmp(line, "yes") == 0;
    for (i = 0; i < config->team_count; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Team %d name: ", i + 1);
        read_line(prompt, config->team_names[i], CF_MAX_NAME);
        if (!config->team_names[i][0]) snprintf(config->team_names[i], CF_MAX_NAME, "Team %d", i + 1);
    }
    players = config->team_count * 3;
    for (i = 0; i < players; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Player %d name: ", i + 1);
        read_line(prompt, config->player_names[i], CF_MAX_NAME);
    }
}

static void print_help(void) {
    puts("Commands: help, board, roll, moves <square>, move <from> <to>, state, log, branch <turn_id>, quit");
}

static bool find_requested_move(CfGame *g, CfCoord from, CfCoord to, CfMove *out) {
    CfMove moves[CF_MAX_MOVES];
    int i, n = cf_engine_generate_moves(g, g->current_player, from, moves, CF_MAX_MOVES);
    for (i = 0; i < n; i++) {
        if (moves[i].to.layer == to.layer && moves[i].to.x == to.x && moves[i].to.y == to.y) {
            *out = moves[i];
            return true;
        }
    }
    return false;
}

int cf_cli_run(CfGame *game) {
    char line[256];
    print_help();
    cf_engine_print_state(game, stdout);
    while (1) {
        printf("crownfall> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        trim_newline(line);
        if (strcmp(line, "quit") == 0) break;
        if (strcmp(line, "help") == 0) print_help();
        else if (strcmp(line, "board") == 0) cf_engine_print_board(game, stdout);
        else if (strcmp(line, "state") == 0) cf_engine_print_state(game, stdout);
        else if (strcmp(line, "log") == 0) printf("%s\n", game->log_path);
        else if (strcmp(line, "roll") == 0) {
            char fields[256];
            CfPlayer *p = &game->players[game->current_player];
            cf_roll_custom_dice(&game->die_a, &game->die_b, &game->dice_sum);
            game->dice_rolled = true;
            snprintf(fields, sizeof(fields), "\"player\":\"%s\",\"player_id\":%d,\"die_a\":%d,\"die_b\":%d,\"sum\":%d",
                     p->name, p->player_id, game->die_a, game->die_b, game->dice_sum);
            cf_log_event(game, "dice_roll", fields);
            printf("Rolled %d + %d = %d\n", game->die_a, game->die_b, game->dice_sum);
        } else if (strncmp(line, "moves ", 6) == 0) {
            CfCoord from;
            CfMove moves[CF_MAX_MOVES];
            int i, n;
            if (!cf_parse_coord(&game->board, line + 6, &from)) {
                puts("Invalid or unplayable square.");
                continue;
            }
            n = cf_engine_generate_moves(game, game->current_player, from, moves, CF_MAX_MOVES);
            for (i = 0; i < n; i++) printf("%s\n", moves[i].notation);
            if (!n) puts("No legal moves.");
        } else if (strncmp(line, "move ", 5) == 0) {
            char a[32], b[32], err[128];
            CfCoord from, to;
            CfMove move;
            if (sscanf(line + 5, "%31s %31s", a, b) != 2 ||
                !cf_parse_coord(&game->board, a, &from) ||
                !cf_parse_coord(&game->board, b, &to)) {
                puts("Usage: move <from> <to> with playable coordinates.");
                continue;
            }
            if (!find_requested_move(game, from, to, &move)) {
                puts("Illegal move.");
                continue;
            }
            if (!cf_engine_apply_move(game, &move, err, sizeof(err))) printf("Move failed: %s\n", err);
            else puts("Move accepted.");
        } else if (strncmp(line, "branch ", 7) == 0) {
            char path[256];
            if (cf_engine_branch(game, atoi(line + 7), path, sizeof(path))) printf("Branch planned at %s\n", path);
            else puts("Time travel is disabled or branch failed.");
        } else {
            puts("Unknown command. Try help.");
        }
    }
    return 0;
}

int cf_agent_stdin_run(CfGame *game) {
    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        trim_newline(line);
        if (strcmp(line, "QUIT") == 0) break;
        if (strcmp(line, "STATE") == 0) cf_engine_print_state(game, stdout);
        else if (strncmp(line, "LEGAL ", 6) == 0) printf("Use LEGAL as future JSON API; current player=%d\n", game->current_player);
        else if (strcmp(line, "ROLL") == 0) {
            cf_roll_custom_dice(&game->die_a, &game->die_b, &game->dice_sum);
            game->dice_rolled = true;
            printf("{\"die_a\":%d,\"die_b\":%d,\"sum\":%d}\n", game->die_a, game->die_b, game->dice_sum);
        } else if (strncmp(line, "MOVE ", 5) == 0) {
            printf("MOVE protocol accepted as textual stub; use CLI move for now.\n");
        } else if (strncmp(line, "APPLY ", 6) == 0) {
            printf("APPLY move_json stub\n");
        } else if (strncmp(line, "BRANCH ", 7) == 0) {
            char path[256];
            if (cf_engine_branch(game, atoi(line + 7), path, sizeof(path))) printf("%s\n", path);
            else printf("BRANCH disabled\n");
        }
        fflush(stdout);
    }
    return 0;
}
