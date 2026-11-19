#include "game.h"
#include "menu.h"
#include "score.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>

#define MESSAGE_BOX_OK 0x00000000U
#define MESSAGE_BOX_ICON_ERROR 0x00000010U

__declspec(dllimport) int __stdcall MessageBoxA(void *window,
                                                const char *text,
                                                const char *caption,
                                                unsigned int type);

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
    AppState app;
    MenuAction action;
    bool audio_available;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1120, 760, "Brain Match - Test Your Memory");
    if (!IsWindowReady()) {
        MessageBoxA(NULL, "Raylib could not create the game window.",
                    "Brain Match", MESSAGE_BOX_OK | MESSAGE_BOX_ICON_ERROR);
        return 1;
    }
    SetExitKey(KEY_NULL);
    SetWindowMinSize(850, 620);
    SetTargetFPS(60);

    menu_init(&app);
    audio_available = game_audio_init();
    if (!audio_available) {
        menu_set_notice(&app, "Audio is unavailable. The game will continue without sound.");
    }

    while (!WindowShouldClose()) {
        float frame_time = GetFrameTime();

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

        BeginDrawing();
        menu_draw(&app);
        if (app.fade_alpha > 0.0f) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                          Fade(BLACK, app.fade_alpha));
        }
        EndDrawing();
    }

    game_audio_close();
    CloseWindow();
    return 0;
}
