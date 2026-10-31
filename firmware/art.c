/* Tidepool on the FM-1: the pixel art. A 5x7 pixel font for titles, the splash (a tide pool at dusk), and each
 * bank's look, after the web app's themes: its palette, an animated banner behind its name (Shallows' light shafts
 * and bubbles, Deep Space's stars, Forest Floor's pines and fireflies, Neon Rain's skyline in the rain, Dungeon
 * Synth's stones and embers; Aurora's northern lights over snow, Dunes' caravan at dusk, Zen Garden's mountain and
 * falling petals, Clockwork's turning gears, Night Cafe's steaming cup of coffee, Bonfire's fire among ruins, Old
 * Blood's blood moon over spires, Cyberpunk's burning city in the rain; the Blank Canvas's watercolours on paper)
 * and the shape of its pools. Everything is drawn procedurally into fm1-base's canvas
 * (gfx.c: cv_rect, cv_pset), in the main loop. Included by app.c. */

/* ---------------------------------------------------------------- the font --- */
/* 5x7, a row a byte, bit 4 the leftmost; capitals, digits and a little punctuation */
static const uint8_t PX_FONT[][7] = {
    /* ' ' */ {0, 0, 0, 0, 0, 0, 0},
    /* A */ {14, 17, 17, 31, 17, 17, 17}, /* B */ {30, 17, 17, 30, 17, 17, 30}, /* C */ {14, 17, 16, 16, 16, 17, 14},
    /* D */ {30, 17, 17, 17, 17, 17, 30}, /* E */ {31, 16, 16, 30, 16, 16, 31}, /* F */ {31, 16, 16, 30, 16, 16, 16},
    /* G */ {14, 17, 16, 23, 17, 17, 15}, /* H */ {17, 17, 17, 31, 17, 17, 17}, /* I */ {14, 4, 4, 4, 4, 4, 14},
    /* J */ {7, 2, 2, 2, 2, 18, 12},      /* K */ {17, 18, 20, 24, 20, 18, 17}, /* L */ {16, 16, 16, 16, 16, 16, 31},
    /* M */ {17, 27, 21, 21, 17, 17, 17}, /* N */ {17, 17, 25, 21, 19, 17, 17}, /* O */ {14, 17, 17, 17, 17, 17, 14},
    /* P */ {30, 17, 17, 30, 16, 16, 16}, /* Q */ {14, 17, 17, 17, 21, 18, 13}, /* R */ {30, 17, 17, 30, 20, 18, 17},
    /* S */ {15, 16, 16, 14, 1, 1, 30},   /* T */ {31, 4, 4, 4, 4, 4, 4},       /* U */ {17, 17, 17, 17, 17, 17, 14},
    /* V */ {17, 17, 17, 17, 17, 10, 4},  /* W */ {17, 17, 17, 21, 21, 21, 10}, /* X */ {17, 17, 10, 4, 10, 17, 17},
    /* Y */ {17, 17, 10, 4, 4, 4, 4},     /* Z */ {31, 1, 2, 4, 8, 16, 31},
    /* 0 */ {14, 17, 19, 21, 25, 17, 14}, /* 1 */ {4, 12, 4, 4, 4, 4, 14},      /* 2 */ {14, 17, 1, 2, 4, 8, 31},
    /* 3 */ {31, 2, 4, 2, 1, 17, 14},     /* 4 */ {2, 6, 10, 18, 31, 2, 2},     /* 5 */ {31, 16, 30, 1, 1, 17, 14},
    /* 6 */ {6, 8, 16, 30, 17, 17, 14},   /* 7 */ {31, 1, 2, 4, 8, 8, 8},       /* 8 */ {14, 17, 17, 14, 17, 17, 14},
    /* 9 */ {14, 17, 17, 15, 1, 2, 12},
    /* - */ {0, 0, 0, 31, 0, 0, 0},       /* . */ {0, 0, 0, 0, 0, 12, 12},      /* : */ {0, 12, 12, 0, 12, 12, 0},
    /* / */ {1, 2, 2, 4, 8, 8, 16},       /* + */ {0, 4, 4, 31, 4, 4, 0},
};

static int px_glyph(char c)
{
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 32);
    if (c >= 'A' && c <= 'Z')
        return 1 + (c - 'A');
    if (c >= '0' && c <= '9')
        return 27 + (c - '0');
    switch (c) {
    case '-': return 37;
    case '.': return 38;
    case ':': return 39;
    case '/': return 40;
    case '+': return 41;
    default: return 0;
    }
}

static int32_t px_text_w(const char *s, int32_t scale)   /* 6 columns a character, less the last gap */
{
    int32_t n = 0;
    while (s[n])
        n++;
    return n ? (n * 6 - 1) * scale : 0;
}

/* text at scale (each font pixel a scale x scale square); a shadow one screen pixel down-right first (0: none) */
static int32_t px_text(int32_t x, int32_t y, int32_t scale, const char *s, uint16_t c, uint16_t shadow, int has_shadow)
{
    int pass;
    for (pass = has_shadow ? 0 : 1; pass < 2; pass++) {
        int32_t cx = x + (pass ? 0 : 1), cy = y + (pass ? 0 : 1);
        const char *p;
        for (p = s; *p; p++, cx += 6 * scale) {
            const uint8_t *g = PX_FONT[px_glyph(*p)];
            int r, b;
            for (r = 0; r < 7; r++)
                for (b = 0; b < 5; b++)
                    if (g[r] >> (4 - b) & 1u)
                        cv_rect(cx + b * scale, cy + r * scale, scale, scale, pass ? c : shadow);
        }
    }
    return x + px_text_w(s, scale);
}

/* ---------------------------------------------------------------- helpers --- */
static uint32_t px_hash(uint32_t x)           /* a stable pseudo-random number for a position or an index */
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

static uint16_t px_mix(uint16_t a, uint16_t b, int32_t t)   /* RGB565 a .. b, t 0..256 */
{
    int32_t ra = a >> 11, ga = (a >> 5) & 63, ba = a & 31, rb = b >> 11, gb = (b >> 5) & 63, bb = b & 31;
    return (uint16_t)(((ra + ((rb - ra) * t >> 8)) << 11) | ((ga + ((gb - ga) * t >> 8)) << 5) |
                      (ba + ((bb - ba) * t >> 8)));
}

static int32_t px_tri(uint32_t t, uint32_t period)   /* a triangle wave 0..256..0 over period */
{
    uint32_t p = t % period;
    return (int32_t)(p < period / 2 ? p * 512u / period : (period - p) * 512u / period);
}

/* ------------------------------------------------------------- the themes --- */
enum { SHAPE_ROUND, SHAPE_ORB, SHAPE_PEBBLE, SHAPE_NEON, SHAPE_ARCH, SHAPE_PAPER, SHAPE_ICE, SHAPE_DUNE, SHAPE_ZEN,
       SHAPE_GEAR, SHAPE_BOOTH, SHAPE_SHIELD, SHAPE_LANCET, SHAPE_CHAMFER };
typedef struct {
    uint16_t bg, cell, cell_sel, title, title_shadow, accent, chord, fam[4];
    uint8_t shape;
    uint8_t light;                            /* paper, not night: dark ink (theme_ink) */
} theme_t;

/* the web app's palettes (public/index.html, :root[data-bank=...]), its dark ones; Shallows' light theme is under
 * water here, so its colours keep their hue and lose their paper. The Blank Canvas keeps its paper (its first) */
static const theme_t THEMES[TP_NBANK + 1] = {
    {RGB(9, 34, 44), RGB(16, 50, 62), RGB(24, 66, 80), RGB(230, 246, 246), RGB(6, 24, 32), RGB(143, 216, 228),
     RGB(30, 110, 150), {RGB(64, 128, 200), RGB(36, 156, 150), RGB(110, 116, 216), RGB(200, 140, 44)}, SHAPE_ROUND, 0},
    {RGB(5, 7, 15), RGB(12, 16, 32), RGB(20, 27, 61), RGB(232, 235, 255), RGB(3, 4, 10), RGB(147, 165, 255),
     RGB(42, 52, 110), {RGB(147, 165, 255), RGB(111, 216, 228), RGB(217, 166, 255), RGB(255, 190, 115)}, SHAPE_ORB, 0},
    {RGB(23, 38, 25), RGB(34, 53, 42), RGB(44, 70, 54), RGB(238, 240, 216), RGB(12, 22, 14), RGB(147, 208, 111),
     RGB(64, 96, 52), {RGB(200, 166, 114), RGB(147, 208, 111), RGB(235, 203, 91), RGB(228, 140, 99)}, SHAPE_PEBBLE, 0},
    {RGB(10, 6, 20), RGB(22, 12, 41), RGB(36, 18, 63), RGB(246, 237, 255), RGB(80, 10, 70), RGB(255, 82, 219),
     RGB(70, 30, 110), {RGB(59, 240, 255), RGB(140, 149, 255), RGB(255, 82, 219), RGB(255, 182, 64)}, SHAPE_NEON, 0},
    {RGB(21, 16, 12), RGB(34, 26, 19), RGB(46, 36, 25), RGB(235, 221, 188), RGB(10, 7, 5), RGB(221, 178, 80),
     RGB(90, 64, 34), {RGB(208, 96, 74), RGB(147, 174, 156), RGB(221, 178, 80), RGB(148, 166, 192)}, SHAPE_ARCH, 0},
    /* the FM-1's own five: Aurora (polar night: ice blue, frost, aurora green, violet) */
    {RGB(6, 14, 28), RGB(14, 28, 46), RGB(22, 42, 66), RGB(232, 250, 255), RGB(4, 8, 18), RGB(120, 250, 196),
     RGB(30, 96, 110), {RGB(110, 196, 255), RGB(176, 206, 228), RGB(110, 240, 176), RGB(196, 140, 255)}, SHAPE_ICE, 0},
    /* Dunes (dusk on sand: burnt orange, sand, gold, terracotta) */
    {RGB(34, 20, 16), RGB(54, 34, 24), RGB(74, 46, 30), RGB(255, 236, 204), RGB(20, 10, 6), RGB(255, 172, 84),
     RGB(140, 70, 36), {RGB(232, 136, 72), RGB(214, 190, 140), RGB(250, 200, 92), RGB(200, 96, 74)}, SHAPE_DUNE, 0},
    /* Zen Garden (moss and rice paper: sage, water, blossom, wood) */
    {RGB(20, 26, 22), RGB(32, 42, 34), RGB(44, 56, 46), RGB(246, 240, 226), RGB(10, 14, 10), RGB(240, 140, 160),
     RGB(104, 72, 84), {RGB(168, 196, 146), RGB(128, 184, 204), RGB(240, 150, 172), RGB(206, 164, 112)}, SHAPE_ZEN, 0},
    /* Clockwork (walnut and brass: copper, steel, brass, verdigris) */
    {RGB(22, 18, 14), RGB(40, 32, 24), RGB(56, 44, 32), RGB(252, 228, 170), RGB(10, 6, 2), RGB(226, 176, 84),
     RGB(112, 82, 40), {RGB(210, 124, 72), RGB(156, 166, 164), RGB(236, 196, 96), RGB(108, 176, 164)}, SHAPE_GEAR, 0},
    /* Night Cafe (wine and lamplight: warm red, tan, gold, smoke blue) */
    {RGB(26, 14, 18), RGB(44, 26, 30), RGB(62, 36, 40), RGB(255, 232, 204), RGB(14, 6, 8), RGB(255, 182, 92),
     RGB(118, 50, 52), {RGB(226, 120, 92), RGB(184, 160, 136), RGB(244, 200, 110), RGB(140, 156, 214)}, SHAPE_BOOTH, 0},
    /* Bonfire (ash and embers: old gold, ash, a soul's pale blue, ember red) */
    {RGB(18, 16, 15), RGB(32, 28, 26), RGB(46, 40, 36), RGB(236, 226, 206), RGB(8, 6, 5), RGB(240, 150, 60),
     RGB(110, 64, 34), {RGB(214, 184, 120), RGB(150, 146, 140), RGB(150, 182, 224), RGB(214, 90, 56)}, SHAPE_SHIELD, 0},
    /* Old Blood (a gothic night: bone, fog, moonlit violet, blood) */
    {RGB(16, 10, 14), RGB(30, 20, 26), RGB(46, 28, 36), RGB(238, 226, 214), RGB(8, 4, 6), RGB(214, 52, 60),
     RGB(100, 24, 32), {RGB(210, 196, 170), RGB(124, 144, 156), RGB(170, 150, 214), RGB(196, 52, 60)}, SHAPE_LANCET, 0},
    /* Cyberpunk (a city at night in the rain: smog orange, rain teal, a billboard's pink, warning red) */
    {RGB(6, 8, 16), RGB(14, 20, 34), RGB(24, 34, 56), RGB(255, 232, 210), RGB(4, 4, 10), RGB(255, 140, 60),
     RGB(110, 50, 24), {RGB(255, 150, 70), RGB(90, 200, 220), RGB(236, 116, 196), RGB(230, 72, 64)}, SHAPE_CHAMFER, 0},
    {RGB(243, 238, 228), RGB(229, 221, 206), RGB(216, 206, 189), RGB(43, 39, 35), RGB(255, 252, 244), RGB(181, 71, 62),
     RGB(181, 71, 62), {RGB(181, 71, 62), RGB(47, 127, 122), RGB(181, 122, 18), RGB(107, 78, 155)}, SHAPE_PAPER, 1},
};

/* text on the theme's ground: hi the main ink, else the quieter one */
static uint16_t theme_ink(const theme_t *T, int hi)
{
    if (T->light)
        return hi ? RGB(43, 39, 35) : RGB(106, 98, 86);
    return hi ? C_WHITE : C_GRAY;
}

/* --------------------------------------------------------------- banners --- */
/* each bank's scene behind its name: 240 x 22 at the top of the header canvas, 2x2 pixels, t in ms */
#define BN_W 120
#define BN_H 11
static void bn_px(int32_t x, int32_t y, uint16_t c) { cv_rect(x * 2, y * 2, 2, 2, c); }

static void banner_shallows(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)                /* water: lighter at the surface */
        for (x = 0; x < BN_W; x++) {
            uint16_t c = px_mix(RGB(18, 78, 100), RGB(10, 44, 58), y * 256 / BN_H);
            int32_t shaft = (int32_t)((x * 10 + y * 6 + t / 40u) % 400u);   /* light shafts, drifting */
            if (shaft < 40)
                c = px_mix(c, RGB(120, 200, 210), 50);
            bn_px(x, y, c);
        }
    for (x = 0; x < BN_W; x++)                /* ripples on the surface */
        if ((px_hash((uint32_t)x / 3u + (t / 250u) * 7u) & 3u) == 0u)
            bn_px(x, 0, RGB(150, 220, 230));
    for (i = 0; i < 9; i++) {                 /* bubbles rising, each at its own pace */
        uint32_t h = px_hash((uint32_t)i + 11u), sp = 300u + h % 400u;
        int32_t bx = (int32_t)(h % BN_W), by = BN_H - 1 - (int32_t)((t / sp + h) % (BN_H + 4u));
        if (by >= 0 && by < BN_H) {
            bn_px(bx, by, RGB(170, 230, 236));
            if ((h >> 8 & 1u) && by + 1 < BN_H)
                bn_px(bx + 1, by + 1, RGB(90, 170, 190));
        }
    }
}

static void banner_deepspace(uint32_t t)
{
    int32_t i, x, y;
    cv_rect(0, 0, 240, 22, RGB(5, 7, 15));
    for (i = 0; i < 46; i++) {                /* stars, each twinkling at its own rate */
        uint32_t h = px_hash((uint32_t)i * 977u + 3u);
        int32_t b = px_tri(t + h % 4000u, 1600u + h % 2400u);
        x = (int32_t)(h % BN_W);
        y = (int32_t)(h >> 8) % BN_H;
        bn_px(x, y, px_mix(RGB(30, 34, 70), (h >> 12 & 1u) ? RGB(232, 235, 255) : RGB(217, 166, 255), b));
    }
    {   /* now and then a shooting star, across and down */
        uint32_t cyc = t % 7000u;
        if (cyc < 900u) {
            int32_t hx = BN_W - 10 - (int32_t)(cyc * 110u / 900u), hy = 1 + (int32_t)(cyc * 6u / 900u), k;
            for (k = 0; k < 6; k++)
                bn_px(hx + k, hy - k / 3, px_mix(RGB(5, 7, 15), RGB(255, 255, 255), 256 - k * 40));
        }
    }
}

static void banner_forest(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)                /* dusk over the trees */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(34, 58, 40), RGB(20, 34, 22), y * 256 / BN_H));
    for (i = 0; i < 16; i++) {                /* pines: stacked triangles, dark against the sky */
        uint32_t h = px_hash((uint32_t)i + 101u);
        int32_t cx = i * 8 + (int32_t)(h % 4u), ht = 5 + (int32_t)(h % 5u);
        for (y = 0; y < ht; y++) {
            int32_t row = BN_H - ht + y, half = y / 2 + 1;
            for (x = cx - half; x <= cx + half; x++)
                bn_px(x, row, RGB(12, 22, 14));
        }
    }
    for (i = 0; i < 7; i++) {                 /* fireflies wandering, blinking */
        uint32_t h = px_hash((uint32_t)i * 31u + 7u);
        int32_t fx = (int32_t)(h % BN_W) + (px_tri(t + h, 9000u + h % 5000u) - 128) / 20;
        int32_t fy = 1 + (int32_t)((h >> 9) % 7u) + (px_tri(t + h * 3u, 5000u + h % 3000u) - 128) / 64;
        int32_t glow = px_tri(t + h, 1400u + h % 1200u);
        if (glow > 150)
            bn_px(fx, fy, px_mix(RGB(80, 90, 30), RGB(255, 236, 120), (glow - 150) * 256 / 106));
    }
}

static void banner_neon(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(26, 12, 52), RGB(10, 6, 20), y * 256 / BN_H));
    for (i = 0, x = 0; x < BN_W; i++) {      /* the skyline, its windows lit in neon */
        uint32_t h = px_hash((uint32_t)i + 501u);
        int32_t w = 6 + (int32_t)(h % 7u), ht = 3 + (int32_t)((h >> 4) % 6u), bx, by;
        for (by = BN_H - ht; by < BN_H; by++)
            for (bx = x; bx < x + w && bx < BN_W; bx++) {
                uint32_t wh = px_hash((uint32_t)(bx * 31 + by * 7));
                uint16_t c = RGB(24, 12, 46);
                if (by > BN_H - ht && (bx - x) % 2 && by % 2 && (wh & 3u) == 0u && ((wh >> 4) + t / 1500u) % 7u)
                    c = (wh >> 8 & 1u) ? RGB(59, 240, 255) : (wh >> 9 & 1u) ? RGB(255, 82, 219) : RGB(255, 182, 64);
                bn_px(bx, by, c);
            }
        x += w + 1;
    }
    for (i = 0; i < 22; i++) {                /* rain, slanting down to the left */
        uint32_t h = px_hash((uint32_t)i * 13u + 5u);
        int32_t ry = (int32_t)((t / 45u + h) % (BN_H + 6u)) - 3, rx = (int32_t)(h % (BN_W + 10u)) - ry / 2, k;
        for (k = 0; k < 3; k++)
            if (ry - k >= 0 && ry - k < BN_H)
                bn_px(rx + k / 2, ry - k, k ? RGB(70, 76, 150) : RGB(150, 160, 255));
    }
}

static void banner_dungeon(uint32_t t)
{
    int32_t x, y, i;
    int32_t flick = (int32_t)(px_hash(t / 90u) % 40u);   /* the torchlight wavers */
    for (y = 0; y < BN_H; y++)                /* stones: courses three high, each course offset */
        for (x = 0; x < BN_W; x++) {
            int32_t course = y / 3, off = course % 2 ? 5 : 0;
            int mortar = y % 3 == 2 || (x + off) % 10 == 0;
            uint32_t h = px_hash((uint32_t)((x + off) / 10 + course * 37));
            uint16_t stone = px_mix(RGB(40, 31, 23), RGB(58, 44, 30), (int32_t)(h % 160u));
            uint16_t c = mortar ? RGB(14, 10, 7) : stone;
            int32_t light = 90 - (x < 60 ? x : BN_W - x) - flick / 2;   /* a warm glow toward the middle */
            if (light < 0 && !mortar)
                c = px_mix(c, RGB(120, 70, 30), (-light) * 2 > 120 ? 120 : (-light) * 2);
            bn_px(x, y, c);
        }
    for (i = 0; i < 8; i++) {                 /* embers drifting up */
        uint32_t h = px_hash((uint32_t)i * 17u + 9u), sp = 120u + h % 120u;
        int32_t ey = BN_H - 1 - (int32_t)((t / sp + h) % (BN_H + 3u));
        int32_t ex = (int32_t)(h % BN_W) + (px_tri(t + h, 2000u) - 128) / 64;
        if (ey >= 0)
            bn_px(ex, ey, ey > 6 ? RGB(255, 176, 80) : ey > 3 ? RGB(230, 110, 50) : RGB(150, 60, 30));
    }
}

static void banner_aurora(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)                /* the polar sky, a little lighter toward the snow */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(4, 8, 24), RGB(14, 34, 52), y * 256 / BN_H));
    for (i = 0; i < 30; i++) {                /* stars */
        uint32_t h = px_hash((uint32_t)i * 613u + 5u);
        if (px_tri(t + h % 3000u, 2000u + h % 2000u) > 60)
            bn_px((int32_t)(h % BN_W), (int32_t)((h >> 8) % 6u), RGB(200, 214, 240));
    }
    for (x = 0; x < BN_W; x++) {              /* the curtains: a ribbon that ripples along and fades at its hem */
        int32_t mid = 3 + (px_tri(t / 3u + (uint32_t)x * 37u, 5200u) - 128) / 70
                      + (px_tri(t / 2u + (uint32_t)x * 91u, 3100u) - 128) / 140;
        int32_t glow = px_tri(t / 4u + (uint32_t)x * 23u, 7000u);
        uint16_t col = px_mix(RGB(80, 255, 170), RGB(170, 110, 255), px_tri((uint32_t)x * 60u + t / 20u, 9000u));
        for (y = 0; y < 8; y++) {
            int32_t d = y - mid, a;
            if (d < -1 || d > 4)
                continue;
            a = (d < 0 ? 150 : 255 - d * 42) * (140 + glow / 2) / 256;
            if ((px_hash((uint32_t)x / 2u + t / 400u * 131u) & 3u) == 0u)   /* rays, flickering along it */
                a += 50;
            a = a > 250 ? 250 : a;
            bn_px(x, y, px_mix(px_mix(RGB(4, 8, 24), RGB(14, 34, 52), y * 256 / BN_H), col, a));
        }
    }
    for (x = 0; x < BN_W; x++) {              /* snowy hills */
        int32_t top = 8 + (px_tri((uint32_t)x * 9u, 1100u) > 140) - (px_tri((uint32_t)x * 5u + 300u, 1500u) > 200);
        for (y = top; y < BN_H; y++)
            bn_px(x, y, y == top ? RGB(236, 246, 255) : RGB(176, 200, 222));
    }
}

static void banner_dunes(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)                /* dusk: violet above, embers at the horizon */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(54, 30, 78), RGB(236, 132, 72), y * 256 / (BN_H - 2)));
    for (i = 0; i < 10; i++) {
        uint32_t h = px_hash((uint32_t)i * 71u + 9u);
        bn_px((int32_t)(h % BN_W), (int32_t)((h >> 8) % 3u), RGB(240, 220, 240));
    }
    for (y = 0; y < 5; y++)                   /* a crescent moon */
        for (x = 0; x < 5; x++) {
            int32_t dx = x - 2, dy = y - 2, ex = x - 3, ey = y - 1;
            if (dx * dx + dy * dy <= 5 && ex * ex + ey * ey > 3)
                bn_px(74 + x, 1 + y, RGB(255, 240, 200));
        }
    for (x = 0; x < BN_W; x++) {              /* two rows of dunes, the far one paler */
        int32_t far = 6 + (px_tri((uint32_t)x * 11u + 400u, 2400u) - 128) / 70;
        int32_t near = 8 + (px_tri((uint32_t)x * 17u + t / 90u, 3000u) - 128) / 90;
        for (y = far; y < BN_H; y++)
            bn_px(x, y, y < near ? RGB(176, 104, 66) : px_mix(RGB(222, 148, 82), RGB(150, 84, 50), (y - near) * 120));
    }
    for (i = 0; i < 3; i++) {                 /* a caravan along the far ridge, slowly */
        static const char *const CAMEL[3] = {"..#.#..", ".#####.", ".#.#.#."};
        int32_t cx = (int32_t)((t / 500u + (uint32_t)i * 9u) % (BN_W + 20u)) - 10, r, c;
        int32_t cy = 6 + (px_tri((uint32_t)cx * 11u + 400u, 2400u) - 128) / 70 - 3;
        for (r = 0; r < 3; r++)
            for (c = 0; c < 7; c++)
                if (CAMEL[r][c] == '#' && cx + c >= 0 && cx + c < BN_W)
                    bn_px(cx + c, cy + r, RGB(60, 30, 30));
    }
}

static void banner_zen(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)                /* a soft evening: lilac to peach */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(58, 44, 84), RGB(170, 96, 112), y * 256 / BN_H));
    for (y = 2; y < 9; y++) {                 /* the mountain, snow on its peak */
        int32_t half = (y - 2) * 2;
        for (x = 76 - half; x <= 76 + half; x++)
            bn_px(x, y, y < 4 ? RGB(250, 248, 255) : RGB(80, 66, 110));
    }
    for (x = 0; x < BN_W; x++)                /* raked gravel: lines a little apart */
        for (y = 9; y < BN_H; y++)
            bn_px(x, y, ((x + y * 3 + (int32_t)(px_tri((uint32_t)x * 40u, 3000u) / 64)) % 4) ? RGB(214, 206, 190)
                                                                                              : RGB(176, 166, 150));
    for (x = 60; x < BN_W; x++) {             /* a branch along the top from the right, blossom on it */
        int32_t by = (BN_W - x) / 40;
        bn_px(x, by, RGB(70, 44, 40));
        if ((px_hash((uint32_t)x * 13u) & 3u) == 0u)
            bn_px(x, by + 1, RGB(250, 176, 196));
        if ((px_hash((uint32_t)x * 29u) & 3u) == 0u)
            bn_px(x, by - 1, RGB(255, 206, 220));
    }
    for (i = 0; i < 9; i++) {                 /* petals drifting down to the left */
        uint32_t h = px_hash((uint32_t)i * 97u + 1u), sp = 140u + h % 120u;
        int32_t py = (int32_t)((t / sp + h) % (BN_H + 2u)) - 1;
        int32_t px = 118 - (int32_t)(h % 40u) - py * 2 + (px_tri(t + h, 1800u) - 128) / 48;
        if (py >= 0 && py < BN_H && px >= 0 && px < BN_W)
            bn_px(px, py, RGB(255, 186, 206));
    }
}

/* a gear: a ring with teeth round it, turned by a (0..31 of a turn) */
static const int8_t GEAR_COS[32] = {64, 63, 59, 53, 45, 36, 24, 12, 0, -12, -24, -36, -45, -53, -59, -63,
                                    -64, -63, -59, -53, -45, -36, -24, -12, 0, 12, 24, 36, 45, 53, 59, 63};
static void bn_gear(int32_t cx, int32_t cy, int32_t r, int32_t teeth, uint32_t a, uint16_t c, uint16_t dark)
{
    int32_t x, y, k;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++) {
            int32_t d = x * x + y * y;
            if (d <= r * r && d >= (r - 2) * (r - 2))
                bn_px(cx + x, cy + y, c);
            else if (d <= 1)
                bn_px(cx + x, cy + y, c);     /* the hub */
            else if (d < (r - 2) * (r - 2) && (x == 0 || y == 0))
                bn_px(cx + x, cy + y, dark);  /* spokes */
        }
    for (k = 0; k < teeth; k++) {             /* each tooth a half-pixel square at the rim, 2x2 screen pixels */
        uint32_t i = (a + (uint32_t)(k * 32 / teeth)) & 31u;
        int32_t tx = cx * 2 + 1 + GEAR_COS[i] * (r * 2 + 1) / 64, ty = cy * 2 + 1 + GEAR_COS[(i + 24u) & 31u] * (r * 2 + 1) / 64;
        cv_rect(tx - 1, ty - 1, 2, 2, c);
    }
}

static void banner_clockwork(uint32_t t)
{
    int32_t y, sw, k;
    for (y = 0; y < BN_H; y++)                /* walnut panelling */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(46, 32, 20), RGB(26, 18, 12), y * 256 / BN_H));
    for (k = 0; k < 120; k += 24)
        cv_rect(k * 2, 0, 2, 22, RGB(20, 14, 10));
    bn_gear(66, 5, 4, 8, t / 160u, RGB(206, 160, 76), RGB(120, 88, 40));           /* turning, each against */
    bn_gear(77, 7, 3, 8, 31u - (t / 120u) % 32u, RGB(184, 120, 70), RGB(110, 70, 40));   /* its neighbour */
    sw = (px_tri(t, 2400u) - 128) / 40;     /* the pendulum, swinging */
    for (k = 0; k < 8; k++)
        bn_px(87 + sw * k / 8, k, RGB(190, 150, 80));
    bn_px(87 + sw, 8, RGB(236, 196, 96));
    bn_px(86 + sw, 8, RGB(236, 196, 96));
    bn_px(87 + sw, 9, RGB(236, 196, 96));
    bn_px(86 + sw, 9, RGB(236, 196, 96));
}

static void banner_cafe(uint32_t t)
{
    static const uint8_t CUP[5][2] = {{71, 79}, {71, 79}, {71, 79}, {72, 78}, {73, 77}};   /* each row's ends */
    const uint16_t wall = RGB(30, 16, 20), cream = RGB(238, 226, 206), shade = RGB(176, 158, 140);
    int32_t x, y, i;
    cv_rect(0, 0, 240, 22, wall);
    for (x = 0; x < BN_W; x++)                /* the table's edge */
        bn_px(x, 10, RGB(74, 42, 30));
    for (x = 68; x <= 82; x++)                /* the saucer */
        bn_px(x, 9, x == 68 || x == 82 ? shade : cream);
    for (y = 0; y < 5; y++)                   /* the cup: cream, shaded on its right, a red band */
        for (x = CUP[y][0]; x <= CUP[y][1]; x++)
            bn_px(x, 4 + y, y == 0 ? RGB(250, 242, 228) : y == 2 ? RGB(178, 62, 52)
                            : x >= CUP[y][1] - 1 ? shade : cream);
    for (x = 73; x <= 77; x++)                /* the coffee inside the rim */
        bn_px(x, 4, RGB(92, 52, 30));
    bn_px(80, 5, cream);                      /* the handle, a loop on the right */
    bn_px(81, 5, cream);
    bn_px(81, 6, shade);
    bn_px(80, 7, shade);
    bn_px(81, 7, shade);
    for (i = 0; i < 3; i++)                   /* steam: three wisps rising, swaying, thinning as they go */
        for (y = 0; y < 4; y++) {
            uint32_t ph = t + (uint32_t)y * 380u + (uint32_t)i * 1500u;
            int32_t sway = (px_tri(ph, 3200u) - 128) / 90;
            int32_t a = 60 + px_tri(ph, 1900u + (uint32_t)i * 300u) * (y + 2) / 5;
            bn_px(73 + i * 2 + sway, y, px_mix(wall, RGB(240, 232, 226), a > 230 ? 230 : a));
        }
    for (x = 0; x < BN_W; x++)                /* a string of lights along the top, sagging between hooks */
        if (x < 64 || x > 88)
            bn_px(x, (x % 20 > 4 && x % 20 < 16) ? 1 : 0, RGB(60, 40, 36));
    for (x = 3; x < BN_W; x += 5) {           /* the bulbs, each glowing and dimming on its own */
        uint32_t h = px_hash((uint32_t)x * 7u);
        int32_t g = px_tri(t + h, 2400u + h % 1600u);
        if (x < 64 || x > 88)
            bn_px(x, (x % 20 > 4 && x % 20 < 16) ? 2 : 1, px_mix(RGB(150, 80, 40), RGB(255, 214, 140), 90 + g * 2 / 3));
    }
}

/* ruins against an ashen dusk: broken towers and walls, and in the gap a bonfire with a sword thrust into it, its
 * flames flickering, embers drifting up */
static void banner_bonfire(uint32_t t)
{
    int32_t x, y, i;
    for (y = 0; y < BN_H; y++)                /* ash grey above, a dull ember glow low down */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(58, 50, 54), RGB(132, 76, 48), y * 256 / BN_H));
    for (x = 0; x < BN_W; x++) {              /* the ruins: towers with broken tops, low walls between */
        uint32_t h = px_hash((uint32_t)(x / 6) * 13u + 5u);
        int32_t top = (h & 3u) == 0u ? 2 + (int32_t)((h >> 4) % 3u) - (int32_t)(px_hash((uint32_t)x) % 2u)
                                     : 6 + (int32_t)((h >> 6) % 2u);
        if (x >= 62 && x <= 90)
            top = 9;                          /* the clearing round the fire */
        for (y = top; y < BN_H; y++)
            bn_px(x, y, y >= 9 ? RGB(12, 10, 10) : RGB(18, 14, 14));
    }
    for (x = 70; x <= 82; x++)                /* the ash and the logs */
        bn_px(x, 9, (x + 1) % 3 ? RGB(70, 60, 56) : RGB(96, 56, 36));
    for (y = 3; y < 10; y++) {                /* flames, each column its own height, flickering */
        for (x = 71; x <= 81; x++) {
            int32_t d = x - 76, w = d < 0 ? -d : d;
            int32_t hgt = 5 - w + (px_tri(t + (uint32_t)x * 157u, 300u + (uint32_t)(x % 4) * 70u) - 128) / 70;
            if (9 - y <= hgt)
                bn_px(x, y, 9 - y >= hgt - 1 ? RGB(214, 70, 30) : 9 - y >= hgt - 3 ? RGB(250, 150, 50)
                                                                                     : RGB(255, 226, 140));
        }
    }
    for (y = 2; y < 9; y++)                   /* the sword, point down in the fire, its guard across */
        bn_px(76, y, y < 3 ? RGB(110, 90, 70) : RGB(176, 176, 184));
    bn_px(75, 3, RGB(150, 150, 160));
    bn_px(77, 3, RGB(150, 150, 160));
    for (i = 0; i < 7; i++) {                 /* embers rising and drifting */
        uint32_t h = px_hash((uint32_t)i * 41u + 7u), per = 1600u + h % 1400u;
        uint32_t p = (t + h) % per;
        int32_t ey = 8 - (int32_t)(p * 9u / per), ex = 73 + (int32_t)(h % 7u) + (px_tri(t + h, 1300u) - 128) / 64;
        if (ey >= 0)
            bn_px(ex, ey, ey > 4 ? RGB(255, 170, 70) : RGB(190, 90, 40));
    }
}

/* a gothic city at night: spires and gables black against the sky, a great moon in the gap that slowly turns blood
 * red and back (the sky reddening with it), lit windows (one now and then flickering, each going dark a while), fog
 * rolling in front of the roofs, a crow crossing now and then */
static void banner_oldblood(uint32_t t)
{
    static const uint16_t ROOF = RGB(10, 6, 10);
    int32_t x, y, i, top[BN_W];
    int32_t tri = px_tri(t, 40000u);          /* the blood moon: pale a while, red a while, slowly between */
    int32_t blood = tri < 70 ? 0 : tri > 190 ? 256 : (tri - 70) * 256 / 120;
    uint16_t low = px_mix(RGB(136, 36, 42), RGB(170, 24, 30), blood);
    for (y = 0; y < BN_H; y++)
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(36, 16, 32), low, y * 256 / BN_H));
    for (i = 0; i < 8; i++) {
        uint32_t h = px_hash((uint32_t)i * 29u + 3u);
        bn_px((int32_t)(h % BN_W), (int32_t)((h >> 8) % 3u), RGB(150, 130, 140));
    }
    for (y = 0; y < 8; y++)                   /* the moon, clear of the roofs */
        for (x = 71; x <= 81; x++) {
            int32_t dx = (x - 76) * 2, dy = y * 2 - 7;
            if (dx * dx + dy * dy <= 70) {
                uint16_t c = px_mix(RGB(244, 236, 222), RGB(206, 34, 40), blood);
                if (((x * 7 + y * 3) % 11) == 0 || (x == 74 && y == 2) || (x == 78 && y == 5))
                    c = px_mix(c, RGB(90, 20, 30), 80);   /* its seas */
                bn_px(x, y, c);
            }
        }
    for (x = 0; x < BN_W; x++) {              /* the roofline: spires, gables, chimneys */
        uint32_t h = px_hash((uint32_t)(x / 5) * 17u + 11u);
        int32_t c5 = x % 5;
        if ((h & 7u) == 0u)                   /* a spire: a narrow point, high */
            top[x] = 1 + (c5 < 2 ? 2 - c5 : c5 - 2) * 2;
        else if ((h & 3u) == 1u)              /* a gable */
            top[x] = 5 + (c5 < 2 ? 2 - c5 : c5 - 2);
        else
            top[x] = 6 + (int32_t)((h >> 5) % 2u) - (c5 == 3 && (h & 16u));
        if (x >= 71 && x <= 81)
            top[x] = top[x] < 8 ? 8 : top[x]; /* lower roofs under the moon */
        else if (x >= 62 && x <= 90)          /* and either side of it, tall houses with their windows in view */
            top[x] = x == 64 || x == 87 ? 3 : 5 - (x == 65 || x == 86);
        for (y = top[x]; y < BN_H; y++)
            bn_px(x, y, ROOF);
    }
    for (x = 2; x < BN_W; x += 5) {           /* windows: about half the houses lit, each dark now and then */
        uint32_t h = px_hash((uint32_t)x * 31u + 5u);
        int32_t wy = top[x] + 2;
        if ((h & 3u) == 3u || wy > 9 || ((t / 7000u + h) % 5u) == 0u)
            continue;
        if ((h >> 4) % 4u == 0u && (t / 90u + h) % 17u < 2u)   /* a guttering lamp */
            bn_px(x, wy, RGB(120, 70, 30));
        else
            bn_px(x, wy, RGB(236, 168, 74));
        if ((h >> 6) & 1u && wy + 2 <= 9)     /* a second window below */
            bn_px(x, wy + 2, RGB(214, 144, 60));
    }
    for (y = 6; y < BN_H; y++)                /* wisps of fog rolling across in front of the roofs */
        for (x = 0; x < BN_W; x++) {
            int32_t w = px_tri((uint32_t)(x * 90 + y * 900) + t / 6u, 6000u) - 170;
            int32_t v = px_tri((uint32_t)(x * 55 + y * 400) + t / 9u + 2500u, 9000u) - 190;
            int32_t a = ((w > 0 ? w : 0) + (v > 0 ? v : 0)) * 3 / 2 + (y - 8) * 12;
            if (a > 0) {
                uint16_t under = y >= top[x] ? ROOF : px_mix(RGB(36, 16, 32), low, y * 256 / BN_H);
                bn_px(x, y, px_mix(under, RGB(132, 120, 132), a > 110 ? 110 : a));
            }
        }
    {                                         /* a crow, now and then, crossing the moon */
        uint32_t p = t % 20000u;
        if (p < 6000u) {
            int32_t cx = (int32_t)(p * (BN_W + 6u) / 6000u) - 3, cy = 3 + (int32_t)(p / 1500u) % 2;
            int up = (t / 180u) & 1u;
            bn_px(cx, cy, RGB(6, 2, 6));
            bn_px(cx - 1, cy - up, RGB(6, 2, 6));
            bn_px(cx + 1, cy - up, RGB(6, 2, 6));
        }
    }
}

static void banner_cyberpunk(uint32_t t)
{
    static const uint16_t TOWER = RGB(8, 8, 16);
    int32_t x, y, i, top[BN_W];
    for (y = 0; y < BN_H; y++)                /* smog lit from below, orange at the horizon */
        cv_rect(0, y * 2, 240, 2, px_mix(RGB(16, 14, 30), RGB(168, 78, 32), y * 256 / BN_H));
    for (x = 0; x < BN_W; x++) {              /* towers, blocks of them, a few very tall */
        uint32_t h = px_hash((uint32_t)(x / 4) * 37u + 9u);
        top[x] = 4 + (int32_t)(h % 5u);
        if ((h & 15u) < 2u)
            top[x] = 1 + (int32_t)((h >> 4) % 2u);
        if (x % 4 == 3 && (h & 32u))          /* a gap between blocks */
            top[x] = top[x] + 2 < BN_H ? top[x] + 2 : top[x];
        if (x >= 72 && x <= 79)               /* the tallest, carrying the billboard, in view between the name and the
                                               * tempo */
            top[x] = x == 72 || x == 79 ? 2 : 1;
        else if ((x >= 61 && x <= 65) || (x >= 82 && x <= 86))   /* low roofs round the stacks */
            top[x] = 8;
        for (y = top[x]; y < BN_H; y++)
            bn_px(x, y, TOWER);
    }
    for (x = 0; x < BN_W; x++)                /* windows: rows of tiny lights, each flickering out now and then */
        for (y = top[x] + 1; y < BN_H; y += 2) {
            uint32_t h = px_hash((uint32_t)(x * 41 + y * 13));
            if ((h & 7u) < 2u && ((t / 2300u + h) % 11u))
                bn_px(x, y, (h >> 5 & 3u) ? RGB(214, 150, 80) : RGB(200, 220, 240));
        }
    for (x = 73; x <= 78; x++)                /* a billboard, high on the tower, its colours turning */
        for (y = 2; y <= 5; y++) {
            int32_t ph = px_tri(t + (uint32_t)(y * 700), 7000u);
            bn_px(x, y, px_mix(RGB(236, 70, 170), RGB(60, 210, 230), ph));
        }
    for (i = 0; i < 2; i++) {                 /* the stacks, and the fire that bursts from them */
        int32_t sx = i ? 84 : 63, burst = px_tri(t + (uint32_t)i * 2600u, 5200u);
        for (y = 6; y < BN_H; y++)
            bn_px(sx, y, RGB(20, 16, 22));
        if (burst > 100) {                    /* a column of flame, flickering, lighting the smog round it */
            int32_t hgt = 1 + (burst - 100) * 4 / 156, k;
            bn_px(sx - 1, 5, RGB(150, 70, 30));
            bn_px(sx + 1, 5, RGB(150, 70, 30));
            for (k = 0; k < hgt; k++) {
                uint32_t f = px_hash((uint32_t)(k * 7 + i * 3) + t / 80u);
                int32_t fx = sx + (k ? (int32_t)(f % 3u) - 1 : 0);
                bn_px(fx, 5 - k, k == 0 ? RGB(255, 236, 150) : k == hgt - 1 ? RGB(220, 70, 20) : RGB(255, 150, 40));
            }
        }
    }
    {                                         /* a flying car, crossing slowly, its light blinking */
        uint32_t p = t % 16000u;
        if (p < 11000u) {
            int32_t cx = BN_W + 2 - (int32_t)(p * (BN_W + 6u) / 11000u), cy = 3 + (int32_t)(p / 3000u) % 2;
            bn_px(cx, cy, RGB(230, 236, 255));
            bn_px(cx + 1, cy, RGB(90, 96, 120));
            if ((t / 300u) & 1u)
                bn_px(cx + 2, cy, RGB(255, 40, 40));
        }
    }
    for (i = 0; i < 26; i++) {                /* rain, straight down */
        uint32_t h = px_hash((uint32_t)i * 19u + 7u);
        int32_t ry = (int32_t)((t / 40u + h) % (BN_H + 5u)) - 2, rx = (int32_t)(h % BN_W);
        if (ry >= 0 && ry < BN_H)
            bn_px(rx, ry, RGB(110, 140, 170));
        if (ry - 1 >= 0 && ry - 1 < BN_H)
            bn_px(rx, ry - 1, RGB(50, 70, 96));
    }
}

static void banner_canvas(uint32_t t)
{
    static const uint16_t WASH[5] = {RGB(181, 71, 62), RGB(47, 127, 122), RGB(181, 122, 18), RGB(107, 78, 155),
                                     RGB(47, 94, 158)};
    int32_t x, y, i, cx[5], cy[5], rr[5];
    for (i = 0; i < 5; i++) {                 /* watercolour washes, drifting slowly along the paper */
        uint32_t h = px_hash((uint32_t)i * 53u + 17u);
        cx[i] = (int32_t)(h % BN_W) + (px_tri(t + h, 26000u + h % 9000u) - 128) / 6;
        cy[i] = 2 + (int32_t)((h >> 8) % 7u) + (px_tri(t + h * 5u, 17000u + h % 6000u) - 128) / 64;
        rr[i] = 7 + (int32_t)((h >> 12) % 6u);
    }
    for (y = 0; y < BN_H; y++)
        for (x = 0; x < BN_W; x++) {
            uint32_t g = px_hash((uint32_t)(x * 131 + y * 977));
            uint16_t c = (g & 15u) == 0u ? RGB(232, 225, 210) : RGB(243, 238, 228);   /* the paper's grain */
            for (i = 0; i < 5; i++) {
                int32_t dx = x - cx[i], dy = (y - cy[i]) * 2, d2 = dx * dx + dy * dy, r2 = rr[i] * rr[i];
                if (d2 < r2) {                /* paler toward the edge, a darker rim where the paint dried */
                    int32_t a = 40 + (r2 - d2) * 30 / r2;
                    if (d2 > r2 * 3 / 4)
                        a += 26;
                    c = px_mix(c, WASH[i], a - (int32_t)(g >> 8 & 7u));
                }
            }
            bn_px(x, y, c);
        }
}

static void banner(int bank, uint32_t t)
{
    switch (bank) {
    case 0: banner_shallows(t); break;
    case 1: banner_deepspace(t); break;
    case 2: banner_forest(t); break;
    case 3: banner_neon(t); break;
    case 4: banner_dungeon(t); break;
    case 5: banner_aurora(t); break;
    case 6: banner_dunes(t); break;
    case 7: banner_zen(t); break;
    case 8: banner_clockwork(t); break;
    case 9: banner_cafe(t); break;
    case 10: banner_bonfire(t); break;
    case 11: banner_oldblood(t); break;
    case 12: banner_cyberpunk(t); break;
    default: banner_canvas(t); break;
    }
}

/* ------------------------------------------------------------ pool shapes --- */
/* a cell's corners cut back to the grid's colour, by the bank's shape; i makes the pebbles each their own */
static void pool_shape(int shape, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t grid, int i)
{
    static const uint8_t ROUND[3] = {3, 1, 1}, ORB[6] = {7, 4, 3, 2, 1, 1}, ARCH[6] = {9, 6, 3, 2, 1, 1};
    static const uint8_t ICE[5] = {5, 4, 3, 2, 1}, STONE[5] = {6, 4, 2, 1, 1}, BOOTH[4] = {5, 3, 2, 1};
    const uint8_t *cut;
    int32_t n, r;
    uint32_t hh = px_hash((uint32_t)i * 7u + 1u);
    switch (shape) {
    case SHAPE_ICE: cut = ICE; n = 5; break;  /* cut straight: a crystal's facets */
    case SHAPE_ZEN: cut = STONE; n = 5; break;
    case SHAPE_BOOTH: cut = BOOTH; n = 4; break;
    case SHAPE_SHIELD: {                       /* a heater shield: square shoulders, curving to the foot */
        static const uint8_t F[6] = {11, 8, 5, 3, 2, 1};
        for (r = 0; r < 6; r++) {
            cv_rect(x, y + h - 1 - r, F[r], 1, grid);
            cv_rect(x + w - F[r], y + h - 1 - r, F[r], 1, grid);
        }
        cv_rect(x, y, 1, 1, grid);
        cv_rect(x + w - 1, y, 1, 1, grid);
        return;
    }
    case SHAPE_LANCET: {                       /* a lancet window: a pointed arch, a square sill */
        static const uint8_t A[5] = {10, 6, 3, 2, 1};
        for (r = 0; r < 5; r++) {
            cv_rect(x, y + r, A[r], 1, grid);
            cv_rect(x + w - A[r], y + r, A[r], 1, grid);
        }
        return;
    }
    case SHAPE_CHAMFER: {                      /* two corners cut on the slant, as a display's frame */
        for (r = 0; r < 6; r++) {
            cv_rect(x, y + r, 6 - r, 1, grid);
            cv_rect(x + w - (6 - r), y + h - 1 - r, 6 - r, 1, grid);
        }
        return;
    }
    case SHAPE_DUNE: {                         /* a dune: a long slope up from the left, a short drop on the right */
        static const uint8_t L[7] = {16, 11, 7, 5, 3, 2, 1}, R[3] = {3, 2, 1};
        for (r = 0; r < 7; r++)
            cv_rect(x, y + r, L[r], 1, grid);
        for (r = 0; r < 3; r++)
            cv_rect(x + w - R[r], y + r, R[r], 1, grid);
        cv_rect(x, y + h - 1, 1, 1, grid);
        cv_rect(x + w - 1, y + h - 1, 1, 1, grid);
        return;
    }
    case SHAPE_GEAR: {                         /* teeth along the top and bottom, square corners cut */
        int32_t k;
        for (k = 4; k < w - 4; k += 8) {
            cv_rect(x + k, y, 4, 2, grid);
            cv_rect(x + k, y + h - 2, 4, 2, grid);
        }
        cut = ROUND;
        n = 2;
        break;
    }
    case SHAPE_ORB: cut = ORB; n = 6; break;
    case SHAPE_ARCH: cut = ARCH; n = 6; break;
    case SHAPE_NEON: cut = ROUND; n = 2; break;
    case SHAPE_PEBBLE: {                       /* each corner its own size: a pebble, not a box */
        static const uint8_t P[4][4] = {{4, 2, 1, 1}, {3, 1, 1, 0}, {5, 3, 2, 1}, {2, 1, 0, 0}};
        int k;
        for (k = 0; k < 4; k++) {
            const uint8_t *c = P[(hh >> (k * 2)) & 3u];
            for (r = 0; r < 4; r++) {
                int32_t yy = k < 2 ? y + r : y + h - 1 - r, xx = k % 2 ? x + w - c[r] : x;
                if (c[r])
                    cv_rect(xx, yy, c[r], 1, grid);
            }
        }
        return;
    }
    default: cut = ROUND; n = 3; break;
    }
    for (r = 0; r < n; r++) {
        cv_rect(x, y + r, cut[r], 1, grid);    /* the top corners */
        cv_rect(x + w - cut[r], y + r, cut[r], 1, grid);
        if (shape != SHAPE_ARCH && shape != SHAPE_BOOTH) {   /* an arch and a booth keep their square foot */
            cv_rect(x, y + h - 1 - r, cut[r], 1, grid);
            cv_rect(x + w - cut[r], y + h - 1 - r, cut[r], 1, grid);
        }
    }
}

/* -------------------------------------------------------------- the splash --- */
/* a tide pool at dusk, 80 x 80 at 3x: the sky, a setting sun, the sea, rocks round a pool with a starfish,
 * bubbles rising; TIDEPOOL in pixel letters over it. t (ms) moves the sea, the sun's reflection and the bubbles */
static const char *const STARFISH[7] = {"...O...", "..OOO..", "OOOOOOO", ".OOOOO.", ".OO.OO.", "OO...OO", "O.....O"};

static uint16_t splash_px(int32_t x, int32_t y, uint32_t t)
{
    static const uint16_t SKY[6] = {RGB(27, 31, 74), RGB(46, 40, 98), RGB(80, 52, 120), RGB(130, 66, 120),
                                    RGB(208, 96, 90), RGB(242, 166, 90)};
    int32_t dx = x - 40, dy = y - 38;
    /* rocks: a bowl round the pool, the near rocks dark against the light */
    int32_t rock_top = x < 22 ? 54 + (x * 3) / 22 - ((int32_t)(px_hash((uint32_t)x) % 2u))
                     : x > 58 ? 57 - ((x - 58) * 3) / 22 - ((int32_t)(px_hash((uint32_t)x + 9u) % 2u)) : 70;
    if (y >= 60 && y <= 69 && x >= 22 && x <= 58) {   /* the pool */
        if (y == 60)
            return ((x + (int32_t)(t / 200u)) % 6) < 3 ? RGB(150, 230, 236) : RGB(80, 190, 200);
        {
            int32_t i;                         /* bubbles rising in it */
            for (i = 0; i < 5; i++) {
                uint32_t h = px_hash((uint32_t)i + 77u);
                int32_t bx = 25 + (int32_t)(h % 32u), by = 69 - (int32_t)((t / (180u + h % 120u) + h) % 10u);
                if (x == bx && y == by && by > 60)
                    return RGB(190, 240, 244);
            }
        }
        return px_mix(RGB(30, 138, 134), RGB(14, 70, 80), (y - 60) * 256 / 10);
    }
    if (y >= rock_top || y >= 70) {            /* the rocks, a highlight on their top edge */
        uint32_t h = px_hash((uint32_t)(x * 131 + y * 17));
        uint16_t c = (y == rock_top) ? RGB(90, 76, 96) : (h & 7u) == 0u ? RGB(58, 48, 66) : RGB(42, 34, 48);
        if (x >= 6 && x < 13 && y >= 60 && y < 67 && STARFISH[y - 60][x - 6] == 'O')
            return RGB(240, 122, 74);          /* the starfish */
        if (x >= 66 && x <= 70 && y >= 61 && y <= 64 && ((x + y) % 2 == 0 || y == 64))
            return RGB(214, 120, 160);         /* an anemone */
        return c;
    }
    if (y < 38) {                               /* the sky, banded with a dither between bands; the sun on the horizon */
        if (dx * dx + dy * dy <= 81)
            return dx * dx + dy * dy <= 36 ? RGB(255, 236, 170) : RGB(255, 206, 110);
        {
            int32_t band = y * 6 / 38, frac = (y * 6) % 38;
            if (frac > 30 && band < 5 && (x + y) % 2)
                band++;
            return SKY[band];
        }
    }
    {   /* the sea: darker toward the shore; the sun's reflection as thin streaks on every other row, each its own
         * length, shimmering, fading toward the shore; wave crests moving */
        uint16_t c = px_mix(RGB(40, 92, 130), RGB(16, 50, 74), (y - 38) * 256 / 20);
        int32_t d = y - 38;
        if (d % 2 == 0 && d < 16) {
            uint32_t h = px_hash((uint32_t)y * 97u + t / 260u);
            int32_t half = 2 + (int32_t)(h % 7u) - d / 5, mid = (int32_t)((h >> 8) % 3u) - 1;
            if (dx - mid >= -half && dx - mid <= half)
                return px_mix(RGB(255, 226, 156), RGB(226, 128, 104), d * 256 / 16);
        }
        if (((x * 3 + y * 7 + (int32_t)(t / 120u)) % 37) == 0)
            return RGB(110, 170, 200);
        return c;
    }
}

/* the whole splash, in two 240 x 120 strips */
static void splash_draw(uint32_t t)
{
    int32_t s, x, y;
    for (s = 0; s < 2; s++) {
        cv_begin(240, 120, C_BLACK);
        for (y = s * 40; y < s * 40 + 40; y++)
            for (x = 0; x < 80; x++)
                cv_rect(x * 3, (y - s * 40) * 3, 3, 3, splash_px(x, y, t));
        if (s == 0) {
            px_text((240 - px_text_w("TIDEPOOL", 4)) / 2, 22, 4, "TIDEPOOL", RGB(255, 244, 224), RGB(27, 31, 74), 1);
            px_text((240 - px_text_w("AMBIENT LOOPS", 2)) / 2, 62, 2, "AMBIENT LOOPS", RGB(255, 196, 140),
                    RGB(46, 40, 98), 1);
        } else
            px_text((240 - px_text_w("FM-1", 2)) / 2, 100, 2, "FM-1", RGB(200, 190, 210), RGB(20, 16, 26), 1);
        cv_blit(0, (uint32_t)(s * 120));
    }
}
