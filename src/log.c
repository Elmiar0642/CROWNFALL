#include "log.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static void timestamp(char *buf, int size, const char *fmt) {
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    if (!tmv) {
        snprintf(buf, (size_t)size, "unknown-time");
        return;
    }
    strftime(buf, (size_t)size, fmt, tmv);
}

void cf_json_escape(const char *src, char *dst, int dst_size) {
    int i = 0;
    if (!dst || dst_size <= 0) return;
    if (!src) src = "";
    while (*src && i < dst_size - 1) {
        if ((*src == '"' || *src == '\\') && i < dst_size - 2) dst[i++] = '\\';
        if ((unsigned char)*src >= 32) dst[i++] = *src;
        src++;
    }
    dst[i] = '\0';
}

bool cf_log_open(CfGame *game) {
    char stamp[32];
    if (!game) return false;
    mkdir("logs", 0775);
    mkdir("logs/snapshots", 0775);
    timestamp(stamp, sizeof(stamp), "%Y%m%d_%H%M%S");
    snprintf(game->session_id, sizeof(game->session_id), "session_%s", stamp);
    snprintf(game->log_path, sizeof(game->log_path), "logs/%s.jsonl", game->session_id);
    game->log_file = fopen(game->log_path, "a");
    return game->log_file != NULL;
}

void cf_log_event(CfGame *game, const char *event, const char *json_fields) {
    char stamp[40];
    if (!game || !game->log_file || !event) return;
    timestamp(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%S%z");
    fprintf(game->log_file,
            "{\"event\":\"%s\",\"turn_id\":%d,\"time\":\"%s\",\"session_id\":\"%s\",\"mode\":\"%s\",\"team_count\":%d,\"current_player\":%d",
            event, game->turn_id, stamp, game->session_id, game->config.mode,
            game->config.team_count, game->current_player);
    if (json_fields && json_fields[0]) fprintf(game->log_file, ",%s", json_fields);
    fprintf(game->log_file, "}\n");
    fflush(game->log_file);
}

void cf_log_close(CfGame *game) {
    if (!game || !game->log_file) return;
    fclose(game->log_file);
    game->log_file = NULL;
}
