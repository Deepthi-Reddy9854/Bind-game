#ifndef BRAIN_MATCH_MENU_H
#define BRAIN_MATCH_MENU_H

#include "game.h"
#include "score.h"

#include <stdbool.h>
#include <stddef.h>

#define PLAYER_NAME_SIZE (SCORE_NAME_MAX + 1)

typedef enum {
    SCREEN_HOME,
    SCREEN_DIFFICULTY,
    SCREEN_INSTRUCTIONS,
    SCREEN_HIGH_SCORES,
    SCREEN_NAME_ENTRY,
    SCREEN_PLAYING,
    SCREEN_GAME_OVER
} AppScreen;

typedef enum {
    MENU_ACTION_NONE,
    MENU_ACTION_START_GAME,
    MENU_ACTION_EXIT
} MenuAction;

typedef struct {
    AppScreen screen;
    GameDifficulty difficulty;
    Game game;
    char player_name[PLAYER_NAME_SIZE];
    char name_input[PLAYER_NAME_SIZE];
    size_t name_length;
    bool name_error;
    ScoreEntry scores[SCORE_TABLE_SIZE];
    size_t score_count;
    float fade_alpha;
    char notice[128];
    double notice_until;
} AppState;

void menu_init(AppState *app);
MenuAction menu_update(AppState *app);
void menu_draw(const AppState *app);
void menu_set_screen(AppState *app, AppScreen screen);
void menu_set_notice(AppState *app, const char *message);

#endif
