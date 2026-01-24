// ======================================================
// Big Bang 3D - MONOLITHIC ALL-IN-ONE SOURCE
// ======================================================

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ------------------------------------------------------
// CONFIG
// ------------------------------------------------------

#define BB3D_MAGIC 0xBB3D
#define MAX_PARTICLES 256

#define CIA_URL   "https://github.com/YOURNAME/BigBang3D/releases/latest/download/BigBang3D.cia"
#define _3DSX_URL "https://github.com/YOURNAME/BigBang3D/releases/latest/download/BigBang3D.3dsx"

#define CIA_PATH   "sdmc:/BigBang3D.cia"
#define _3DSX_PATH "sdmc:/3ds/BigBang3D/BigBang3D.3dsx"

// Pretendo / Miiverse (example title ID)
#define MIIVERSE_TITLE_ID 0x0004003000008F02ULL

// ------------------------------------------------------
// STRUCTS
// ------------------------------------------------------

typedef struct {
    float x, y, vx, vy;
    int life;
    u32 color;
} Particle;

typedef struct {
    int score;
    int combo;
    int maxCombo;
} Score;

typedef struct {
    bool crtFilter;
    bool onlineEnabled;
    bool streetPassEnabled;
} Settings;

// ------------------------------------------------------
// GLOBALS
// ------------------------------------------------------

static Particle particles[MAX_PARTICLES];
static Score score;
static Settings settings = { true, true, true };

static C2D_Target* top;
static bool showSettings = false;

// ------------------------------------------------------
// UTILS
// ------------------------------------------------------

bool IsOnline(void) {
    u32 status = 0;
    ACU_GetWifiStatus(&status);
    return status != 0;
}

u32 Neon(void) {
    return C2D_Color32(rand()%255, rand()%255, rand()%255, 255);
}

// ------------------------------------------------------
// BIG BANG GAMEPLAY
// ------------------------------------------------------

void SpawnBigBang(float x, float y) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        float a = ((float)i / MAX_PARTICLES) * 2.0f * M_PI;
        particles[i] = (Particle){
            x, y,
            cosf(a)*3.5f,
            sinf(a)*3.5f,
            60,
            Neon()
        };
    }
    score.combo++;
    score.score += 100 * score.combo;
    if (score.combo > score.maxCombo)
        score.maxCombo = score.combo;
}

void UpdateParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].life--;
        }
    }
}

void DrawParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            C2D_DrawRectSolid(
                particles[i].x,
                particles[i].y,
                0, 3, 3,
                particles[i].color
            );
        }
    }
}

// ------------------------------------------------------
// CRT FILTER (SCANLINES + VIGNETTE)
// ------------------------------------------------------

void DrawCRTFilter(void) {
    if (!settings.crtFilter) return;

    // Scanlines
    for (int y = 0; y < 240; y += 2) {
        C2D_DrawRectSolid(
            0, y,
            0,
            400, 1,
            C2D_Color32(0, 0, 0, 40)
        );
    }

    // Vignette edges
    C2D_DrawRectSolid(0, 0, 0, 400, 20, C2D_Color32(0,0,0,60));
    C2D_DrawRectSolid(0, 220, 0, 400, 20, C2D_Color32(0,0,0,60));
}

// ------------------------------------------------------
// STREETPASS (SEND ONLY SHOWN)
// ------------------------------------------------------

void StreetPass_Init(void) {
    udsInit(0x3000, NULL);
}

void StreetPass_Send(void) {
    if (!settings.streetPassEnabled) return;

    struct {
        u32 magic;
        u32 score;
        u16 combo;
    } data = { BB3D_MAGIC, score.score, score.maxCombo };

    udsSendTo(UDS_BROADCAST_MAC, &data, sizeof(data));
}

// ------------------------------------------------------
// MIIVERSE / PRETENDO
// ------------------------------------------------------

void PostScoreToMiiverse(void) {
    if (!settings.onlineEnabled || !IsOnline()) return;

    char text[256];
    snprintf(text, sizeof(text),
        "BIG BANG 3D 💥\nScore: %d\nCombo: x%d",
        score.score, score.maxCombo);

    httpcContext ctx;
    httpcOpenContext(&ctx, HTTPC_METHOD_POST,
        "https://miiverse.pretendo.cc/api/post", 1);

    httpcAddRequestHeaderField(&ctx, "Content-Type", "text/plain");
    httpcAddPostDataRaw(&ctx, text, strlen(text));
    httpcBeginRequest(&ctx);
    httpcCloseContext(&ctx);
}

void OpenMiiverse(void) {
    aptSetChainloader(MIIVERSE_TITLE_ID, NULL);
    aptExit();
}

// ------------------------------------------------------
// GITHUB CIA / 3DSX DOWNLOADER
// ------------------------------------------------------

bool DownloadFile(const char* url, const char* path) {
    if (!IsOnline()) return false;

    httpcContext ctx;
    if (R_FAILED(httpcOpenContext(&ctx, HTTPC_METHOD_GET, url, 1)))
        return false;

    httpcBeginRequest(&ctx);

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    u8 buf[4096];
    u32 size;

    while (true) {
        httpcDownloadData(&ctx, buf, sizeof(buf), &size);
        if (!size) break;
        fwrite(buf, 1, size, f);
    }

    fclose(f);
    httpcCloseContext(&ctx);
    return true;
}

// ------------------------------------------------------
// SETTINGS MENU
// ------------------------------------------------------

void DrawSettings(void) {
    C2D_DrawRectSolid(40, 40, 0, 320, 160, C2D_Color32(0,0,0,200));

    // (Text omitted for brevity – use C2D_Text in real project)
}

void HandleSettings(u32 down) {
    if (down & KEY_A) settings.crtFilter = !settings.crtFilter;
    if (down & KEY_X) settings.onlineEnabled = !settings.onlineEnabled;
    if (down & KEY_Y) settings.streetPassEnabled = !settings.streetPassEnabled;
}

// ------------------------------------------------------
// MAIN
// ------------------------------------------------------

int main(void) {
    gfxInitDefault();
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    memset(&score, 0, sizeof(score));

    StreetPass_Init();

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_START) break;

        if (down & KEY_SELECT) showSettings = !showSettings;

        if (showSettings) {
            HandleSettings(down);
        } else {
            if (down & KEY_A)
                SpawnBigBang(200, 120);

            if (!(held & KEY_A))
                score.combo = 0;

            if (down & KEY_X)
                DownloadFile(CIA_URL, CIA_PATH);

            if (down & KEY_Y)
                DownloadFile(_3DSX_URL, _3DSX_PATH);

            if (down & KEY_B) {
                PostScoreToMiiverse();
                OpenMiiverse();
            }
        }

        UpdateParticles();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(5,5,20,255));
        C2D_SceneBegin(top);

        DrawParticles();
        DrawCRTFilter();

        if (showSettings)
            DrawSettings();

        C3D_FrameEnd(0);
    }

    StreetPass_Send();
    C2D_Fini();
    gfxExit();
    return 0;
}
