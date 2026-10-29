/* Tidepool on the FM-1: the app's settings and its half of the contract with fm1-base
 * (base/device/unity.c on the FM-1, base/host/plat_host.c in the host simulator). */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "TIDEPOOL"
#define APP_SPLASH_RGB RGB(79, 179, 179)
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz (eight engine blocks) */

/* the store: the app's settings, the panel calibration (the platform's), and the Blank Canvas's 16 loops, one object
 * each (saved as each is recorded or deleted), in the data area from 0x97000; and their flash offsets */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_CANVAS, OBJ_NOBJ = OBJ_CANVAS + 16 };
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0x97000u, 0x99000u, 0x9B000u, 0x9D000u, 0x9F000u, 0xA1000u, 0xA3000u, \
                       0xA5000u, 0xA7000u, 0xA9000u, 0xAB000u, 0xAD000u, 0xAF000u, 0xB1000u, 0xB3000u, 0xB5000u}

/* the keys' velocity (lib/touch.h): on in the Blank Canvas's recording mode only */
#define APP_TOUCH

/* the contract: the platform calls these */
void app_init(void);                     /* boot: the engine, then the saved settings (or defaults) */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);   /* audio interrupt: n stereo frames, 24-bit */
void app_panic(void);                    /* silence now (before an update rewrites the flash) */
void app_ui_init(void);                  /* the first screen, after the splash */
void app_frame(void);                    /* the UI: input and a redraw, about 60 times a second */
void app_input_only(void);               /* between frames: input only */
void app_say(const char *a, const char *b);   /* a short message on the screen and to a screen reader */

/* the first screen at power-on: the splash (art.c) */
#define APP_SPLASH
void app_splash(void);

/* the render over 90 % of its time: the engine sheds its quietest notes (app.c) */
#define APP_OVERLOAD
void app_overload(void);

/* the Mac's bench over USB-MIDI SysEx (app.c, tools/fm1_bench.py) */
#define APP_SYSEX
int app_sysex(const uint8_t *p, uint32_t n);

/* the host simulator's checks (base/host/plat_host.c "expect"): want0..want15, setting, row, bpm, dirty, voices,
 * and the canvas's (app.c app_expect) */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);

/* the host simulator's own commands (app.c): "oldsettings 5|3 BANK" writes a settings record as an older firmware
 * saved it (TDP5: five banks and the canvas; TDP3: five banks), to test what a newer one keeps of it */
#define APP_HOST_CMD
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
