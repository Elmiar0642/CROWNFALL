#include "rules.h"

#include <stdio.h>
#include <string.h>

bool cf_rules_can_capture(const CfGame *game, const CfPiece *attacker, const CfPiece *target) {
    int i;
    if (!game || !attacker || !target || !target->alive) return false;
    if (attacker->team == target->team) return false;
    if (attacker->type == CF_PIECE_PRINCE && target->type == CF_PIECE_PRINCE) return false;
    if (attacker->type == CF_PIECE_PRINCE && target->type == CF_PIECE_QUEEN) {
        for (i = 0; i < game->mercy_count; i++) {
            if (game->mercies[i].pact &&
                game->mercies[i].queen_id == target->id &&
                game->mercies[i].prince_id == attacker->id) return false;
        }
    }
    return true;
}

bool cf_rules_team_in_check(const CfGame *game, int team) {
    (void)game;
    (void)team;
    /* TODO(advanced-rules): full check detection across all active enemies. */
    return false;
}

bool cf_rules_repetition_legal(CfGame *game) {
    char key[512];
    int i, p, n = 0;
    if (!game) return true;
    n += snprintf(key + n, sizeof(key) - (size_t)n, "p%d|", game->current_player);
    for (p = 0; p < game->piece_count && n < (int)sizeof(key) - 16; p++) {
        const CfPiece *pc = &game->pieces[p];
        if (pc->alive) {
            n += snprintf(key + n, sizeof(key) - (size_t)n, "%d:%d:%d:%d;",
                          pc->id, pc->pos.layer, pc->pos.x, pc->pos.y);
        }
    }
    for (i = 0; i < game->repetition_len; i++) {
        if (strcmp(game->repetition[i], key) == 0) {
            game->repetition_count[i]++;
            return game->repetition_count[i] <= 3;
        }
    }
    if (game->repetition_len < CF_MAX_HISTORY) {
        strncpy(game->repetition[game->repetition_len], key, sizeof(game->repetition[0]) - 1);
        game->repetition_count[game->repetition_len] = 1;
        game->repetition_len++;
    }
    return true;
}
