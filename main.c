#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define ART_W 32
#define ART_H 32
#define CELL 6
#define ART_X 12
#define ART_Y 24
#define PAL_X 220
#define PAL_Y 34
#define PAL_CELL 20
#define SAVE_DIR "sdmc:/3ds/PixelForge3DS"
#define SAVE_FILE SAVE_DIR "/drawing.ppm"

/* Colors stored as 0xRRGGBB. PPM files can be opened in GIMP/Krita/Aseprite. */
static const uint32_t palette[16] = {
    0x000000, 0xFFFFFF, 0x7F7F7F, 0xC3C3C3,
    0xB13B41, 0xF07563, 0xF5B041, 0xF7E26B,
    0x439657, 0x79D283, 0x3E8CBB, 0x74C9E8,
    0x594FAD, 0xAB7ACD, 0xA36B46, 0xE3AE8A
};

static uint32_t pixels[ART_H][ART_W];
static uint8_t* framebuffer;
static int penColor = 4;
static int cursorX = 16;
static int cursorY = 16;
static bool dragging = false;
static bool gridEnabled = true;
static bool changed = false;
static char statusText[96] = "Ready to draw!";

static void putPixel(int x, int y, uint32_t rgb) {
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
    /* Nintendo 3DS RGB8 framebuffer is stored rotated 90 degrees, BGR bytes. */
    const int idx = (x * SCREEN_H + (SCREEN_H - 1 - y)) * 3;
    framebuffer[idx + 0] = (uint8_t)(rgb & 0xFF);
    framebuffer[idx + 1] = (uint8_t)((rgb >> 8) & 0xFF);
    framebuffer[idx + 2] = (uint8_t)((rgb >> 16) & 0xFF);
}

static void fillRect(int x, int y, int w, int h, uint32_t rgb) {
    for (int dy = 0; dy < h; ++dy)
        for (int dx = 0; dx < w; ++dx)
            putPixel(x + dx, y + dy, rgb);
}

static void rectOutline(int x, int y, int w, int h, uint32_t rgb) {
    fillRect(x, y, w, 1, rgb);
    fillRect(x, y + h - 1, w, 1, rgb);
    fillRect(x, y, 1, h, rgb);
    fillRect(x + w - 1, y, 1, h, rgb);
}

static void setStatus(const char* msg) {
    snprintf(statusText, sizeof(statusText), "%s", msg);
}

static void paint(int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= ART_W || y >= ART_H) return;
    if (pixels[y][x] != color) {
        pixels[y][x] = color;
        changed = true;
    }
}

static void renderBottom(void) {
    framebuffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    fillRect(0, 0, SCREEN_W, SCREEN_H, 0x171A25);
    fillRect(0, 0, SCREEN_W, 15, 0x37435C);
    fillRect(ART_X - 2, ART_Y - 2, ART_W * CELL + 4, ART_H * CELL + 4, 0xFFFFFF);
    for (int y = 0; y < ART_H; ++y) {
        for (int x = 0; x < ART_W; ++x) {
            int xx = ART_X + x * CELL;
            int yy = ART_Y + y * CELL;
            fillRect(xx, yy, CELL, CELL, pixels[y][x]);
            if (gridEnabled) {
                putPixel(xx + CELL - 1, yy, 0xA1A1A1);
                putPixel(xx, yy + CELL - 1, 0xA1A1A1);
            }
        }
    }
    rectOutline(ART_X + cursorX * CELL, ART_Y + cursorY * CELL, CELL, CELL, 0xFF236B);
    fillRect(PAL_X - 3, PAL_Y - 11, 86, 181, 0x2D3448);
    for (int i = 0; i < 16; ++i) {
        int x = PAL_X + (i % 4) * PAL_CELL;
        int y = PAL_Y + (i / 4) * PAL_CELL;
        fillRect(x, y, PAL_CELL - 3, PAL_CELL - 3, palette[i]);
        if (i == penColor) rectOutline(x - 2, y - 2, PAL_CELL + 1, PAL_CELL + 1, 0xFFE94F);
    }
    fillRect(PAL_X, 131, 76, 35, palette[penColor]);
    rectOutline(PAL_X - 1, 130, 78, 37, 0xFFFFFF);
    /* Color swatch for eraser: white square below current color. */
    fillRect(PAL_X, 179, 76, 24, 0xFFFFFF);
    rectOutline(PAL_X, 179, 76, 24, 0x777777);
    fillRect(0, 226, SCREEN_W, 14, changed ? 0x6B4F31 : 0x285446);
}

static void renderTop(void) {
    /* The top screen is the instructional/status console. */
    printf("\x1b[1;1HPIXELFORGE 3DS - Pixel Art Editor   ");
    printf("\x1b[3;1H32 x 32 pixels | 16 color palette  ");
    printf("\x1b[5;1HSTYLUS: draw on bottom canvas      ");
    printf("\x1b[6;1HSTYLUS: tap palette to pick color  ");
    printf("\x1b[8;1HD-PAD: move cursor               ");
    printf("\x1b[9;1HA: paint  B: erase                ");
    printf("\x1b[10;1HL/R: change selected color        ");
    printf("\x1b[11;1HX: toggle grid                    ");
    printf("\x1b[12;1HY: save   SELECT: load            ");
    printf("\x1b[13;1HSTART: exit                       ");
    printf("\x1b[16;1HSelected: %2d   Cursor: %2d,%2d   ", penColor + 1, cursorX, cursorY);
    printf("\x1b[18;1H%-38.38s", statusText);
    printf("\x1b[20;1HSD output: /3ds/PixelForge3DS/  ");
    printf("\x1b[21;1Hdrawing.ppm (P6 RGB)             ");
    printf("\x1b[23;1H%s", changed ? "Unsaved changes   " : "Drawing saved/clean");
}

static bool saveDrawing(void) {
    mkdir(SAVE_DIR, 0777); /* Directory may already exist. */
    FILE* fp = fopen(SAVE_FILE, "wb");
    if (!fp) {
        setStatus("SAVE FAILED: SD card unavailable?");
        return false;
    }
    if (fprintf(fp, "P6\n%d %d\n255\n", ART_W, ART_H) < 0) {
        fclose(fp);
        setStatus("SAVE FAILED: cannot write header");
        return false;
    }
    for (int y = 0; y < ART_H; ++y) {
        for (int x = 0; x < ART_W; ++x) {
            uint32_t c = pixels[y][x];
            uint8_t rgb[3] = { (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c };
            if (fwrite(rgb, 1, 3, fp) != 3) {
                fclose(fp);
                setStatus("SAVE FAILED: SD write error");
                return false;
            }
        }
    }
    if (fclose(fp) != 0) {
        setStatus("SAVE FAILED: SD close error");
        return false;
    }
    changed = false;
    setStatus("Saved drawing.ppm successfully!");
    return true;
}

static bool loadDrawing(void) {
    FILE* fp = fopen(SAVE_FILE, "rb");
    if (!fp) {
        setStatus("LOAD FAILED: drawing.ppm missing");
        return false;
    }
    int w = 0, h = 0, maxValue = 0;
    char magic[3] = {0};
    if (fscanf(fp, "%2s %d %d %d", magic, &w, &h, &maxValue) != 4 ||
        strcmp(magic, "P6") != 0 || w != ART_W || h != ART_H || maxValue != 255 ||
        fgetc(fp) != '\n') {
        fclose(fp);
        setStatus("LOAD FAILED: invalid 32x32 PPM");
        return false;
    }
    uint32_t temporary[ART_H][ART_W];
    for (int y = 0; y < ART_H; ++y) {
        for (int x = 0; x < ART_W; ++x) {
            uint8_t rgb[3];
            if (fread(rgb, 1, 3, fp) != 3) {
                fclose(fp);
                setStatus("LOAD FAILED: incomplete file");
                return false;
            }
            temporary[y][x] = ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
        }
    }
    fclose(fp);
    memcpy(pixels, temporary, sizeof(pixels));
    changed = false;
    setStatus("Drawing loaded successfully!");
    return true;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB8_OES);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    for (int y = 0; y < ART_H; ++y)
        for (int x = 0; x < ART_W; ++x)
            pixels[y][x] = 0xFFFFFF;
    renderTop();

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();
        if (down & KEY_START) break;
        if (down & KEY_L) penColor = (penColor + 15) % 16;
        if (down & KEY_R) penColor = (penColor + 1) % 16;
        if (down & KEY_X) gridEnabled = !gridEnabled;
        if (down & KEY_Y) saveDrawing();
        if (down & KEY_SELECT) loadDrawing();

        /* One cursor step per keypress; touch allows continuous drawing. */
        if (down & KEY_DUP && cursorY > 0) --cursorY;
        if (down & KEY_DDOWN && cursorY < ART_H - 1) ++cursorY;
        if (down & KEY_DLEFT && cursorX > 0) --cursorX;
        if (down & KEY_DRIGHT && cursorX < ART_W - 1) ++cursorX;
        if (held & KEY_A) paint(cursorX, cursorY, palette[penColor]);
        if (held & KEY_B) paint(cursorX, cursorY, 0xFFFFFF);

        if (held & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);
            int tx = touch.px;
            int ty = touch.py;
            if (tx >= ART_X && tx < ART_X + ART_W * CELL &&
                ty >= ART_Y && ty < ART_Y + ART_H * CELL) {
                cursorX = (tx - ART_X) / CELL;
                cursorY = (ty - ART_Y) / CELL;
                paint(cursorX, cursorY, palette[penColor]);
                dragging = true;
            } else if (!dragging && tx >= PAL_X && tx < PAL_X + PAL_CELL * 4 &&
                       ty >= PAL_Y && ty < PAL_Y + PAL_CELL * 4) {
                int column = (tx - PAL_X) / PAL_CELL;
                int row = (ty - PAL_Y) / PAL_CELL;
                penColor = row * 4 + column;
            } else if (!dragging && tx >= PAL_X && tx < PAL_X + 76 &&
                       ty >= 179 && ty < 203) {
                paint(cursorX, cursorY, 0xFFFFFF);
            }
        } else {
            dragging = false;
        }
        renderBottom();
        renderTop();
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
