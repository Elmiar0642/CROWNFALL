#ifndef CROWNFALL_CLI_H
#define CROWNFALL_CLI_H

#include "engine.h"

int cf_cli_run(CfGame *game);
int cf_agent_stdin_run(CfGame *game);
void cf_prompt_config(CfConfig *config);

#endif
