#ifndef CROWNFALL_RULES_H
#define CROWNFALL_RULES_H

#include "engine.h"

bool cf_rules_can_capture(const CfGame *game, const CfPiece *attacker, const CfPiece *target);
bool cf_rules_team_in_check(const CfGame *game, int team);
bool cf_rules_repetition_legal(CfGame *game);

#endif
