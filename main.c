#include "game.h"
#include "menu.h"
#include "score.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define MESSAGE_BOX_OK 0x00000000U
#define MESSAGE_BOX_ICON_ERROR 0x00000010U

__declspec(dllimport) int __stdcall MessageBoxA(void *window,
                                                const char *text,
                                                const char *caption,
                                                unsigned int type);
#endif

static void save_game_score(AppState *app)
{
    ScoreEntry entry = {0};
    char error[128];

    snprintf(entry.name, sizeof(entry.name), "%s", app->player_name);
    snprintf(entry.difficulty, sizeof(entry.difficulty), "%s",
             app->difficulty == DIFFICULTY_HARD ? "Hard"
             : app->difficulty == DIFFICULTY_MEDIUM ? "Medium"
                                                    : "Easy");
    entry.score = app->game.score;
    entry.level = app->game.correct_answers;
    entry.seconds = game_elapsed_seconds(&app->game);

    if (!scores_insert(app->scores, &app->score_count, &entry)) {
        menu_set_notice(app, "Your score did not make the top 10.");
        return;
    }
    if (!scores_save(app->scores, app->score_count, error, sizeof(error))) {
        menu_set_notice(app, error);
        return;
    }
    menu_set_notice(app, "Your score has been added to the leaderboard.");
}

int main(void)
{
    AppState app = {0};
    MenuAction action;
    bool app_ready = false;
    bool audio_attempted = false;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1120, 760, "Brain Match - Test Your Memory");
    if (!IsWindowReady()) {
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
        MessageBoxA(NULL, "Raylib could not create the game window.",
                    "Brain Match", MESSAGE_BOX_OK | MESSAGE_BOX_ICON_ERROR);
#else
        TraceLog(LOG_ERROR, "Raylib could not create the game window.");
#endif
        return 1;
    }
    SetExitKey(KEY_NULL);
    SetWindowMinSize(850, 620);
    SetTargetFPS(60);

    scores_storage_begin();

    while (!WindowShouldClose()) {
        float frame_time = GetFrameTime();

        if (!app_ready &&
            (scores_storage_ready() || scores_storage_failed())) {
            menu_init(&app);
            app_ready = true;
            if (scores_storage_failed()) {
                menu_set_notice(&app,
                                "Browser storage is unavailable; scores may not persist.");
            }
#if !defined(__EMSCRIPTEN__)
            audio_attempted = true;
            if (!game_audio_init()) {
                menu_set_notice(&app,
                                "Audio is unavailable. The game will continue without sound.");
            }
#endif
        }

#if defined(__EMSCRIPTEN__)
        if (app_ready && !audio_attempted &&
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
             IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ONE) ||
             IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_THREE) ||
             IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_FIVE))) {
            audio_attempted = true;
            if (!game_audio_init()) {
                menu_set_notice(&app,
                                "Audio is unavailable. The game will continue without sound.");
            }
        }
#endif

        if (app_ready) {
            if (app.screen == SCREEN_PLAYING) {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    menu_set_screen(&app, SCREEN_HOME);
                } else {
                    game_update(&app.game);
                    if (game_is_over(&app.game)) {
                        save_game_score(&app);
                        menu_set_screen(&app, SCREEN_GAME_OVER);
                    }
                }
            } else {
                action = menu_update(&app);
                if (action == MENU_ACTION_START_GAME) {
                    game_start(&app.game, app.difficulty);
                } else if (action == MENU_ACTION_EXIT) {
                    break;
                }
            }

            if (app.fade_alpha > 0.0f) {
                app.fade_alpha -= frame_time * 1.4f;
                if (app.fade_alpha < 0.0f) {
                    app.fade_alpha = 0.0f;
                }
            }
        }

        BeginDrawing();
        if (app_ready) {
            menu_draw(&app);
            if (app.fade_alpha > 0.0f) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                              Fade(BLACK, app.fade_alpha));
            }
        } else {
            ClearBackground((Color){19, 20, 54, 255});
            DrawText("BRAIN MATCH", GetScreenWidth() / 2 - 118,
                     GetScreenHeight() / 2 - 28, 38, RAYWHITE);
            DrawText("Preparing your saved scores...",
                     GetScreenWidth() / 2 - 125,
                     GetScreenHeight() / 2 + 28, 16,
                     (Color){190, 197, 228, 255});
        }
        EndDrawing();
    }

    if (audio_attempted) {
        game_audio_close();
    }
    CloseWindow();
    return 0;
}
