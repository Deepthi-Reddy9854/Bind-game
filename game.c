#include "game.h"

#include "raylib.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GAME_WIDTH 1120
#define GAME_HEIGHT 760
#define ANSWER_BUTTON_WIDTH 190
#define ANSWER_BUTTON_HEIGHT 54

static const char *SYMBOL_SETS[] = {
    "1234",
    "ABCDEF",
    "ABCDEFGH01234567"
};

static Sound correct_sound;
static Sound incorrect_sound;
static Sound click_sound;
static bool audio_ready;

static const char *difficulty_name(GameDifficulty difficulty)
{
    switch (difficulty) {
    case DIFFICULTY_MEDIUM:
        return "Medium";
    case DIFFICULTY_HARD:
        return "Hard";
    case DIFFICULTY_EASY:
    default:
        return "Easy";
    }
}

static float symbol_duration(const Game *game)
{
    float base;
    float duration;

    switch (game->difficulty) {
    case DIFFICULTY_MEDIUM:
        base = 0.62f;
        break;
    case DIFFICULTY_HARD:
        base = 0.48f;
        break;
    case DIFFICULTY_EASY:
    default:
        base = 0.78f;
        break;
    }
    duration = base - (float)((game->level - 1) / 4) * 0.035f;
    return duration < 0.25f ? 0.25f : duration;
}

static float answer_time_limit(const Game *game)
{
    float base;
    float limit;

    switch (game->difficulty) {
    case DIFFICULTY_MEDIUM:
        base = 14.0f;
        break;
    case DIFFICULTY_HARD:
        base = 11.0f;
        break;
    case DIFFICULTY_EASY:
    default:
        base = 18.0f;
        break;
    }
    limit = base - (float)((game->level - 1) / 3) * 0.5f;
    return limit < 6.0f ? 6.0f : limit;
}

static void begin_sequence(Game *game)
{
    game->phase = GAME_PHASE_SHOW_SEQUENCE;
    game->reveal_index = 0;
    game->reveal_visible = true;
    game->answer_length = 0;
    game->answer[0] = '\0';
    game->phase_started = GetTime();
}

static void append_random_symbol(Game *game)
{
    const char *symbols = SYMBOL_SETS[game->difficulty];
    int symbol_count = (int)strlen(symbols);

    if (game->sequence_length < GAME_MAX_SEQUENCE) {
        game->sequence[game->sequence_length++] =
            symbols[GetRandomValue(0, symbol_count - 1)];
        game->sequence[game->sequence_length] = '\0';
    }
}

static Wave create_tone_wave(float frequency, float duration)
{
    const unsigned int sample_rate = 44100;
    const unsigned int sample_count = (unsigned int)(sample_rate * duration);
    short *samples = MemAlloc(sample_count * sizeof(*samples));
    Wave wave = {0};
    unsigned int index;

    if (samples == NULL) {
        return wave;
    }
    for (index = 0; index < sample_count; index++) {
        float progress = (float)index / (float)sample_count;
        float envelope = 1.0f - progress;
        float sample = sinf(2.0f * PI * frequency * (float)index /
                            (float)sample_rate);
        samples[index] = (short)(sample * envelope * 11000.0f);
    }
    wave.frameCount = sample_count;
    wave.sampleRate = sample_rate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;
    return wave;
}

static Sound load_tone(float frequency, float duration)
{
    Wave wave = create_tone_wave(frequency, duration);
    Sound sound = {0};

    if (IsWaveValid(wave)) {
        sound = LoadSoundFromWave(wave);
        UnloadWave(wave);
    }
    return sound;
}

bool game_audio_init(void)
{
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    if (!IsAudioDeviceReady()) {
        return false;
    }

    correct_sound = load_tone(880.0f, 0.16f);
    incorrect_sound = load_tone(220.0f, 0.22f);
    click_sound = load_tone(540.0f, 0.06f);
    audio_ready = IsSoundValid(correct_sound) &&
                  IsSoundValid(incorrect_sound) &&
                  IsSoundValid(click_sound);
    return audio_ready;
}

void game_audio_close(void)
{
    if (!IsAudioDeviceReady()) {
        return;
    }
    if (IsSoundValid(correct_sound)) {
        UnloadSound(correct_sound);
    }
    if (IsSoundValid(incorrect_sound)) {
        UnloadSound(incorrect_sound);
    }
    if (IsSoundValid(click_sound)) {
        UnloadSound(click_sound);
    }
    CloseAudioDevice();
    audio_ready = false;
}

void game_play_ui_click(void)
{
    if (audio_ready) {
        PlaySound(click_sound);
    }
}

static void play_answer_sound(bool correct)
{
    if (!audio_ready) {
        return;
    }
    PlaySound(correct ? correct_sound : incorrect_sound);
}

void game_start(Game *game, GameDifficulty difficulty)
{
    memset(game, 0, sizeof(*game));
    game->difficulty = difficulty;
    game->level = 1;
    game->lives = 3;
    game->game_started = GetTime();
    game->sequence_length = 0;
    append_random_symbol(game);
    append_random_symbol(game);
    append_random_symbol(game);
    begin_sequence(game);
}

static void begin_answer_phase(Game *game)
{
    game->phase = GAME_PHASE_INPUT;
    game->phase_started = GetTime();
    game->response_deadline = game->phase_started + answer_time_limit(game);
    game->answer_length = 0;
    game->answer[0] = '\0';
}

static void submit_answer(Game *game)
{
    int index;

    if (game->answer_length != game->sequence_length) {
        snprintf(game->feedback, sizeof(game->feedback),
                 "Enter all %d symbols first.", game->sequence_length);
        game->feedback_until = GetTime() + 1.0;
        return;
    }

    for (index = 0; index < game->sequence_length; index++) {
        if (game->answer[index] != game->sequence[index]) {
            break;
        }
    }
    game->last_answer_correct = index == game->sequence_length;
    game->phase = GAME_PHASE_FEEDBACK;
    game->feedback_until = GetTime() + 0.9;

    if (game->last_answer_correct) {
        game->score += game->level * 100;
        game->correct_answers++;
        snprintf(game->feedback, sizeof(game->feedback), "Excellent memory!");
    } else {
        game->incorrect_answers++;
        game->lives--;
        snprintf(game->feedback, sizeof(game->feedback), "Not quite. Try again!");
        if (game->lives <= 0) {
            game->phase = GAME_PHASE_OVER;
            snprintf(game->feedback, sizeof(game->feedback), "Game over");
        }
    }
    play_answer_sound(game->last_answer_correct);
}

void game_update(Game *game)
{
    double now = GetTime();
    int queued_character;

    if (game->phase == GAME_PHASE_OVER) {
        return;
    }

    if (game->phase == GAME_PHASE_SHOW_SEQUENCE) {
        float visible_time = symbol_duration(game);
        double phase_elapsed = now - game->phase_started;
        double phase_limit = game->reveal_visible ? visible_time : 0.16;

        while ((queued_character = GetCharPressed()) > 0) {
            (void)queued_character;
        }
        if (phase_elapsed >= phase_limit) {
            if (game->reveal_visible) {
                game->reveal_visible = false;
                game->phase_started = now;
            } else if (game->reveal_index + 1 < game->sequence_length) {
                game->reveal_index++;
                game->reveal_visible = true;
                game->phase_started = now;
            } else {
                begin_answer_phase(game);
            }
        }
        return;
    }

    if (game->phase == GAME_PHASE_INPUT) {
        int character;
        Vector2 mouse = GetMousePosition();
        Rectangle submit_button = {
            (float)GetScreenWidth() / 2.0f - ANSWER_BUTTON_WIDTH - 12.0f,
            (float)GetScreenHeight() - 112.0f,
            ANSWER_BUTTON_WIDTH,
            ANSWER_BUTTON_HEIGHT
        };
        Rectangle clear_button = {
            (float)GetScreenWidth() / 2.0f + 12.0f,
            (float)GetScreenHeight() - 112.0f,
            ANSWER_BUTTON_WIDTH,
            ANSWER_BUTTON_HEIGHT
        };

        if (now >= game->response_deadline) {
            game->incorrect_answers++;
            game->lives--;
            game->last_answer_correct = false;
            game->phase = game->lives <= 0 ? GAME_PHASE_OVER : GAME_PHASE_FEEDBACK;
            game->feedback_until = now + 0.9;
            snprintf(game->feedback, sizeof(game->feedback),
                     game->lives <= 0 ? "Time is up. Game over!"
                                      : "Time is up. Try again!");
            play_answer_sound(false);
            return;
        }

        while ((character = GetCharPressed()) > 0) {
            char value = (char)toupper((unsigned char)character);
            const char *symbols = SYMBOL_SETS[game->difficulty];
            if (strchr(symbols, value) != NULL &&
                game->answer_length < GAME_MAX_SEQUENCE) {
                game->answer[game->answer_length++] = value;
                game->answer[game->answer_length] = '\0';
                game->feedback[0] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && game->answer_length > 0) {
            game->answer[--game->answer_length] = '\0';
            game->feedback[0] = '\0';
        }
        if (CheckCollisionPointRec(mouse, clear_button) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            game->answer_length = 0;
            game->answer[0] = '\0';
            game->feedback[0] = '\0';
            game_play_ui_click();
        }
        if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
             (CheckCollisionPointRec(mouse, submit_button) &&
              IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))) {
            game_play_ui_click();
            submit_answer(game);
            return;
        }
        return;
    }

    if (game->phase == GAME_PHASE_FEEDBACK && now >= game->feedback_until) {
        while ((queued_character = GetCharPressed()) > 0) {
            (void)queued_character;
        }
        if (game->last_answer_correct) {
            game->level++;
            append_random_symbol(game);
        }
        begin_sequence(game);
    }
}

bool game_is_over(const Game *game)
{
    return game->phase == GAME_PHASE_OVER;
}

long game_elapsed_seconds(const Game *game)
{
    double elapsed = GetTime() - game->game_started;
    return elapsed < 0.0 ? 0 : (long)elapsed;
}

static void draw_centered_text(const char *text, int y, int font_size, Color color)
{
    int width = MeasureText(text, font_size);
    DrawText(text, (GetScreenWidth() - width) / 2, y, font_size, color);
}

static void draw_game_background(void)
{
    double time = GetTime();

    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
                           (Color){19, 20, 54, 255},
                           (Color){48, 25, 82, 255});
    DrawCircleGradient((Vector2){GetScreenWidth() * 0.17f +
                                     (float)sin(time * 0.35) * 25.0f,
                                 GetScreenHeight() * 0.28f},
                       210.0f, (Color){58, 97, 190, 45},
                       (Color){58, 97, 190, 0});
    DrawCircleGradient((Vector2){GetScreenWidth() * 0.83f,
                                 GetScreenHeight() * 0.73f +
                                     (float)cos(time * 0.3) * 20.0f},
                       260.0f, (Color){171, 72, 196, 40},
                       (Color){171, 72, 196, 0});
}

static void draw_stat_card(Rectangle rectangle,
                           const char *label,
                           const char *value,
                           Color accent)
{
    int value_width = MeasureText(value, 24);

    DrawRectangleRounded(rectangle, 0.22f, 8, (Color){255, 255, 255, 18});
    DrawRectangleRounded((Rectangle){rectangle.x, rectangle.y + 13, 4,
                                     rectangle.height - 26},
                         0.5f, 4, accent);
    DrawText(label, (int)rectangle.x + 17, (int)rectangle.y + 13,
             15, (Color){192, 198, 226, 255});
    DrawText(value, (int)(rectangle.x + rectangle.width - value_width - 16),
             (int)rectangle.y + 32, 24, RAYWHITE);
}

static void draw_answer_button(Rectangle rectangle,
                               const char *text,
                               Vector2 mouse,
                               Color color)
{
    bool hovered = CheckCollisionPointRec(mouse, rectangle);
    DrawRectangleRounded(rectangle, 0.32f, 10,
                         hovered ? Fade(color, 1.0f) : Fade(color, 0.82f));
    DrawRectangleRoundedLinesEx(rectangle, 0.32f, 10, 1.5f,
                                hovered ? Fade(RAYWHITE, 0.8f)
                                        : Fade(RAYWHITE, 0.25f));
    DrawText(text, (int)(rectangle.x +
                         (rectangle.width - MeasureText(text, 18)) / 2),
             (int)rectangle.y + 17, 18, RAYWHITE);
}

void game_draw(const Game *game, const char *player_name)
{
    char value[64];
    char time_text[32];
    Rectangle panel;
    Vector2 mouse = GetMousePosition();
    int index;
    int center_x = GetScreenWidth() / 2;
    int center_y = GetScreenHeight() / 2;

    draw_game_background();
    DrawText("BRAIN MATCH", 36, 28, 25, RAYWHITE);
    DrawText(TextFormat("%s  /  %s", player_name, difficulty_name(game->difficulty)),
             38, 63, 16, (Color){190, 197, 228, 255});

    snprintf(value, sizeof(value), "%d", game->score);
    draw_stat_card((Rectangle){GetScreenWidth() - 600.0f, 28, 126, 74},
                   "SCORE", value, (Color){109, 220, 191, 255});
    snprintf(value, sizeof(value), "%d", game->level);
    draw_stat_card((Rectangle){GetScreenWidth() - 458.0f, 28, 126, 74},
                   "LEVEL", value, (Color){133, 158, 255, 255});
    snprintf(value, sizeof(value), "%d", game->correct_answers);
    draw_stat_card((Rectangle){GetScreenWidth() - 316.0f, 28, 126, 74},
                   "CORRECT", value, (Color){110, 228, 171, 255});
    snprintf(value, sizeof(value), "%d", game->incorrect_answers);
    draw_stat_card((Rectangle){GetScreenWidth() - 174.0f, 28, 138, 74},
                   "INCORRECT", value, (Color){255, 134, 154, 255});

    {
        float panel_width = fminf(800.0f, GetScreenWidth() - 60.0f);
        panel = (Rectangle){center_x - panel_width / 2.0f,
                            center_y - 125.0f, panel_width, 310.0f};
    }
    DrawRectangleRounded(panel, 0.08f, 18, (Color){255, 255, 255, 17});
    DrawRectangleRoundedLinesEx(panel, 0.08f, 18, 1.0f,
                                (Color){255, 255, 255, 30});

    if (game->phase == GAME_PHASE_SHOW_SEQUENCE) {
        float pulse = 0.94f + (float)sin(GetTime() * 5.0) * 0.025f;
        draw_centered_text("WATCH THE SEQUENCE", center_y - 93, 18,
                           (Color){197, 202, 232, 255});
        if (game->reveal_visible) {
            snprintf(value, sizeof(value), "%c", game->sequence[game->reveal_index]);
            {
                int font_size = (int)(126.0f * pulse);
                draw_centered_text(value, center_y - 63, font_size,
                                   (Color){126, 237, 220, 255});
            }
        } else {
            DrawCircle(center_x, center_y - 25, 4,
                       (Color){124, 132, 178, 255});
        }
        for (index = 0; index < game->sequence_length; index++) {
            Color dot = index <= game->reveal_index
                            ? (Color){125, 232, 216, 255}
                            : (Color){255, 255, 255, 55};
            DrawCircle(center_x - (game->sequence_length - 1) * 13 + index * 26,
                       center_y + 102, 5, dot);
        }
        draw_centered_text("Keep the pattern in order", center_y + 130, 17,
                           (Color){190, 196, 225, 255});
    } else if (game->phase == GAME_PHASE_INPUT) {
        float time_left = (float)(game->response_deadline - GetTime());
        float time_limit = answer_time_limit(game);
        float bar_width = (float)panel.width - 100.0f;
        float fill = time_left / time_limit;
        if (time_left < 0.0f) {
            time_left = 0.0f;
        }
        if (fill < 0.0f) {
            fill = 0.0f;
        }
        snprintf(time_text, sizeof(time_text), "Time left: %.1f s", time_left);
        draw_centered_text("REPEAT THE SEQUENCE", center_y - 97, 19,
                           (Color){197, 202, 232, 255});
        draw_centered_text(time_text, center_y - 60, 16,
                           time_left < 4.0f ? (Color){255, 132, 149, 255}
                                            : (Color){175, 205, 255, 255});
        DrawRectangleRounded((Rectangle){center_x - bar_width / 2,
                                         center_y - 28, bar_width, 8},
                             0.5f, 4, (Color){255, 255, 255, 30});
        DrawRectangleRounded((Rectangle){center_x - bar_width / 2,
                                         center_y - 28, bar_width * fill, 8},
                             0.5f, 4, (Color){99, 214, 194, 255});

        {
            float slot_gap = fminf(50.0f,
                                   (GetScreenWidth() - 120.0f) /
                                       game->sequence_length);
            float slot_width = slot_gap - 6.0f;
            float total_width = slot_gap * (game->sequence_length - 1) +
                                slot_width;
            float first_x = center_x - total_width / 2.0f;
            int slot_font_size = slot_width < 34.0f ? 22 : 36;

            for (index = 0; index < game->sequence_length; index++) {
                Rectangle slot = {first_x + index * slot_gap,
                                  center_y + 9.0f, slot_width, 58.0f};

                DrawRectangleRounded(slot, 0.2f, 8,
                                     index < game->answer_length
                                         ? (Color){83, 135, 190, 100}
                                         : (Color){255, 255, 255, 20});
                if (index < game->answer_length) {
                    snprintf(value, sizeof(value), "%c",
                             game->answer[index]);
                    DrawText(value,
                             (int)(slot.x + (slot.width -
                                             MeasureText(value, slot_font_size)) /
                                         2.0f),
                             (int)slot.y + 12, slot_font_size,
                             RAYWHITE);
                } else {
                    DrawCircle((int)(slot.x + slot.width / 2),
                               (int)(slot.y + slot.height / 2), 3,
                               (Color){255, 255, 255, 90});
                }
            }
        }
        draw_answer_button((Rectangle){center_x - ANSWER_BUTTON_WIDTH - 12.0f,
                                       GetScreenHeight() - 112.0f,
                                       ANSWER_BUTTON_WIDTH,
                                       ANSWER_BUTTON_HEIGHT},
                           "SUBMIT", mouse, (Color){76, 172, 164, 255});
        draw_answer_button((Rectangle){center_x + 12.0f,
                                       GetScreenHeight() - 112.0f,
                                       ANSWER_BUTTON_WIDTH,
                                       ANSWER_BUTTON_HEIGHT},
                           "CLEAR", mouse, (Color){83, 96, 145, 255});
        if (game->feedback[0] != '\0' && GetTime() < game->feedback_until) {
            draw_centered_text(game->feedback, center_y + 85, 16,
                               (Color){255, 214, 137, 255});
        }
        draw_centered_text("Type the symbols, use Backspace to edit, or press Enter",
                           GetScreenHeight() - 45, 15,
                           (Color){179, 187, 217, 255});
    } else if (game->phase == GAME_PHASE_FEEDBACK) {
        Color feedback_color = game->last_answer_correct
                                   ? (Color){125, 235, 193, 255}
                                   : (Color){255, 144, 164, 255};
        draw_centered_text(game->feedback, center_y - 12, 32, feedback_color);
        draw_centered_text(game->last_answer_correct ? "Next level incoming..."
                                                     : "Take a breath and try again",
                           center_y + 42, 17,
                           (Color){198, 203, 230, 255});
    }

    for (index = 0; index < game->lives; index++) {
        DrawCircle(42 + index * 28, GetScreenHeight() - 42, 8,
                   (Color){255, 126, 154, 255});
    }
    DrawText("LIVES", 36 + game->lives * 28, GetScreenHeight() - 50, 14,
             (Color){196, 201, 226, 255});
    snprintf(value, sizeof(value), "%ld s", game_elapsed_seconds(game));
    DrawText(TextFormat("PLAY TIME  %s", value),
             GetScreenWidth() - 150, GetScreenHeight() - 48, 14,
             (Color){196, 201, 226, 255});
}
