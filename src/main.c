#include "cli.h"
#include "engine.h"
#include "gui.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    CfConfig config;
    CfGame *game;
    if (argc > 1 && strcmp(argv[1], "--agent") == 0) {
        memset(&config, 0, sizeof(config));
        config.team_count = 2;
        strcpy(config.mode, "agent");
        game = cf_engine_new(&config);
        if (!game) return 1;
        cf_agent_stdin_run(game);
        cf_engine_free(game);
        return 0;
    }
    cf_prompt_config(&config);
    game = cf_engine_new(&config);
    if (!game) {
        fprintf(stderr, "Failed to create game.\n");
        return 1;
    }
    if (strcmp(config.mode, "board-gui") == 0 || strcmp(config.mode, "gui") == 0) {
        int rc = cf_gui_run(game, &argc, &argv);
        cf_engine_free(game);
        return rc;
    }
    cf_cli_run(game);
    cf_engine_free(game);
    return 0;
}
