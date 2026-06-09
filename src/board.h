#ifndef CROWNFALL_BOARD_H
#define CROWNFALL_BOARD_H

#include <stdbool.h>

#define CF_MAX_LAYERS 2
#define CF_MAX_SIZE 15
#define CF_MAX_SQUARES (CF_MAX_LAYERS * CF_MAX_SIZE * CF_MAX_SIZE)

typedef enum {
    CF_LAYOUT_2T,
    CF_LAYOUT_4T,
    CF_LAYOUT_6T_WIP,
    CF_LAYOUT_8T
} CfLayout;

typedef struct {
    int layers;
    int size;
    CfLayout layout;
    bool playable[CF_MAX_LAYERS][CF_MAX_SIZE][CF_MAX_SIZE];
} CfBoard;

typedef struct {
    int layer;
    int x;
    int y;
} CfCoord;

void cf_board_init(CfBoard *board, int team_count);
bool cf_board_is_playable(const CfBoard *board, CfCoord c);
bool cf_parse_coord(const CfBoard *board, const char *text, CfCoord *out);
void cf_coord_to_string(const CfBoard *board, CfCoord c, char *out, int out_size);
int cf_board_playable_count(const CfBoard *board);

#endif
