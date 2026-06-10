#ifndef CROWNFALL_LOG_H
#define CROWNFALL_LOG_H

#include "engine.h"

#include <stdbool.h>

bool cf_log_open(CfGame *game);
void cf_log_set_sink(CfGame *game, CfLogSink sink, void *user);
void cf_log_event(CfGame *game, const char *event, const char *json_fields);
void cf_log_close(CfGame *game);
void cf_json_escape(const char *src, char *dst, int dst_size);

#endif
