/* Tidepool on the FM-1: the preset banks: the web app's five, in its order, then eight written for the FM-1. */
#include "tp_engine.h"

extern const tp_bank_t TP_SHALLOWS, TP_DEEPSPACE, TP_FORESTFLOOR, TP_NEONRAIN, TP_DUNGEON;
extern const tp_bank_t TP_AURORA, TP_DUNES, TP_ZEN, TP_CLOCKWORK, TP_CAFE, TP_BONFIRE, TP_OLDBLOOD, TP_CYBERPUNK;

const tp_bank_t *const TP_BANKS[TP_NBANK] = { &TP_SHALLOWS, &TP_DEEPSPACE, &TP_FORESTFLOOR, &TP_NEONRAIN, &TP_DUNGEON,
                                              &TP_AURORA, &TP_DUNES, &TP_ZEN, &TP_CLOCKWORK, &TP_CAFE,
                                              &TP_BONFIRE, &TP_OLDBLOOD, &TP_CYBERPUNK };
