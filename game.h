#ifndef BRAIN_MATCH_GAME_H
#define BRAIN_MATCH_GAME_H

#include <stdbool.h>

#define GAME_MAX_SEQUENCE 24

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
} GameDifficulty;

typedef enum {
    GAME_PHASE_SHOW_SEQUENCE,
    GAME_PHASE_INPUT,
    GAME_PHASE_FEEDBACK,
    GAME_PHASE_OVER
} GamePhase;

typedef struct {
    GameDifficulty difficulty;
    GamePhase phase;
    char sequence[GAME_MAX_SEQUENCE + 1];
    char answer[GAME_MAX_SEQUENCE + 1];
    int sequence_length;
    int reveal_index;
    int answer_length;
    int score;
    int level;
    int correct_answers;
    int incorrect_answers;
    int lives;
    bool reveal_visible;
    bool last_answer_correct;
    double phase_started;
    double response_deadline;
    double feedback_until;
    double game_started;
    char feedback[48];
} Game;

void game_start(Game *game, GameDifficulty difficulty);
void game_update(Game *game);
void game_draw(const Game *game, const char *player_name);
bool game_is_over(const Game *game);
long game_elapsed_seconds(const Game *game);

bool game_audio_init(void);
void game_audio_close(void);
void game_play_ui_click(void);

#endif
