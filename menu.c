#include "menu.h"

#include "raylib.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define BUTTON_WIDTH 330.0f
#define BUTTON_HEIGHT 56.0f
#define BUTTON_GAP 12.0f
#define BUTTON_COUNT 5

static const Color COLOR_BG_TOP = {18, 21, 55, 255};
static const Color COLOR_BG_BOTTOM = {48, 26, 82, 255};
static const Color COLOR_CYAN = {93, 224, 205, 255};
static const Color COLOR_PANEL = {255, 255, 255, 19};
static const Color COLOR_TEXT_MUTED = {184, 192, 224, 255};

static const char *difficulty_label(GameDifficulty difficulty)
{
    switch (difficulty) {
    case DIFFICULTY_MEDIUM:
        return "MEDIUM";
    case DIFFICULTY_HARD:
        return "HARD";
    case DIFFICULTY_EASY:
    default:
        return "EASY";
    }
}

static void center_text(const char *text, int y, int font_size, Color color)
{
    DrawText(text, (GetScreenWidth() - MeasureText(text, font_size)) / 2,
             y, font_size, color);
}

static void draw_background(void)
{
    double time = GetTime();
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    int index;

    DrawRectangleGradientV(0, 0, width, height, COLOR_BG_TOP, COLOR_BG_BOTTOM);
    DrawCircleGradient((Vector2){width * 0.12f + (float)sin(time * 0.35) * 24,
                                 height * 0.25f},
                       245.0f, (Color){73, 117, 230, 53},
                       (Color){73, 117, 230, 0});
    DrawCircleGradient((Vector2){width * 0.88f,
                                 height * 0.72f + (float)cos(time * 0.31) * 20},
                       270.0f, (Color){185, 87, 206, 47},
                       (Color){185, 87, 206, 0});

    for (index = 0; index < 22; index++) {
        float x = (float)((index * 197 + 61) % width);
        float y = (float)((index * 137 + 37) % height);
        float pulse = 1.0f + (float)sin(time * 1.5 + index) * 0.35f;
        DrawCircle((int)x, (int)y, pulse, (Color){218, 227, 255, 105});
    }
}

static Rectangle home_button(int index)
{
    float first_y = GetScreenHeight() < 680 ? 240.0f : 278.0f;
    return (Rectangle){(GetScreenWidth() - BUTTON_WIDTH) / 2.0f,
                       first_y + index * (BUTTON_HEIGHT + BUTTON_GAP),
                       BUTTON_WIDTH, BUTTON_HEIGHT};
}

static Rectangle bottom_button(float width, float y)
{
    return (Rectangle){(GetScreenWidth() - width) / 2.0f,
                       y, width, BUTTON_HEIGHT};
}

static bool button_clicked(Rectangle rectangle)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(GetMousePosition(), rectangle);
}

static void draw_button(Rectangle rectangle,
                        const char *label,
                        bool primary,
                        bool enabled)
{
    Vector2 mouse = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mouse, rectangle);
    Color fill = primary ? (Color){91, 104, 221, 255}
                         : (Color){255, 255, 255, 20};
    Color text_color = enabled ? RAYWHITE : (Color){150, 154, 180, 255};
    int text_width = MeasureText(label, 20);
    float hover_scale = hovered ? 1.018f : 1.0f;
    Rectangle visual = {
        rectangle.x - (rectangle.width * hover_scale - rectangle.width) / 2,
        rectangle.y - (rectangle.height * hover_scale - rectangle.height) / 2,
        rectangle.width * hover_scale,
        rectangle.height * hover_scale
    };

    if (hovered) {
        fill = primary ? (Color){117, 130, 255, 255}
                       : (Color){255, 255, 255, 37};
    }
    DrawRectangleRounded(visual, 0.28f, 12, fill);
    DrawRectangleRoundedLinesEx(visual, 0.28f, 12, hovered ? 2.0f : 1.0f,
                                hovered ? Fade(COLOR_CYAN, 0.9f)
                                        : (Color){255, 255, 255, 30});
    DrawText(label,
             (int)(rectangle.x + (rectangle.width - text_width) / 2),
             (int)rectangle.y + 17, 20, text_color);
}

static void draw_header(const char *title, const char *subtitle)
{
    center_text(title, 82, 48, RAYWHITE);
    center_text(subtitle, 139, 20, COLOR_TEXT_MUTED);
}

static void draw_notice(const AppState *app)
{
    if (app->notice[0] != '\0' && GetTime() < app->notice_until) {
        center_text(app->notice, GetScreenHeight() - 36, 15,
                    (Color){255, 220, 143, 255});
    }
}

static void set_name_error(AppState *app, bool error)
{
    app->name_error = error;
}

static MenuAction submit_name(AppState *app)
{
    while (app->name_length > 0 &&
           app->name_input[app->name_length - 1] == ' ') {
        app->name_input[--app->name_length] = '\0';
    }
    if (app->name_length == 0) {
        set_name_error(app, true);
        return MENU_ACTION_NONE;
    }
    memcpy(app->player_name, app->name_input, app->name_length + 1);
    set_name_error(app, false);
    menu_set_screen(app, SCREEN_PLAYING);
    return MENU_ACTION_START_GAME;
}

void menu_init(AppState *app)
{
    char error[128];

    memset(app, 0, sizeof(*app));
    app->screen = SCREEN_HOME;
    app->difficulty = DIFFICULTY_EASY;
    strcpy(app->player_name, "Player");
    if (!scores_load(app->scores, &app->score_count, error, sizeof(error))) {
        menu_set_notice(app, error);
    }
}

void menu_set_screen(AppState *app, AppScreen screen)
{
    app->screen = screen;
    app->fade_alpha = 0.27f;
}

void menu_set_notice(AppState *app, const char *message)
{
    snprintf(app->notice, sizeof(app->notice), "%s", message);
    app->notice_until = GetTime() + 5.0;
}

static MenuAction update_home(AppState *app)
{
    int index;

    for (index = 0; index < BUTTON_COUNT; index++) {
        bool keyboard_click = IsKeyPressed((KeyboardKey)(KEY_ONE + index));
        if (button_clicked(home_button(index)) || keyboard_click) {
            game_play_ui_click();
            switch (index) {
            case 0:
                app->name_length = 0;
                app->name_input[0] = '\0';
                set_name_error(app, false);
                menu_set_screen(app, SCREEN_NAME_ENTRY);
                break;
            case 1:
                menu_set_screen(app, SCREEN_DIFFICULTY);
                break;
            case 2:
                menu_set_screen(app, SCREEN_INSTRUCTIONS);
                break;
            case 3:
                menu_set_screen(app, SCREEN_HIGH_SCORES);
                break;
            case 4:
                return MENU_ACTION_EXIT;
            default:
                break;
            }
            return MENU_ACTION_NONE;
        }
    }
    return MENU_ACTION_NONE;
}

static MenuAction update_difficulty(AppState *app)
{
    float card_width = 250.0f;
    float gap = 22.0f;
    float total_width = card_width * 3.0f + gap * 2.0f;
    float start_x = (GetScreenWidth() - total_width) / 2.0f;
    int index;

    for (index = 0; index < 3; index++) {
        Rectangle card = {start_x + index * (card_width + gap),
                          GetScreenHeight() / 2.0f - 76.0f,
                          card_width, 200.0f};
        if (button_clicked(card)) {
            app->difficulty = (GameDifficulty)index;
            game_play_ui_click();
            menu_set_notice(app, TextFormat("%s difficulty selected.",
                                            difficulty_label(app->difficulty)));
            menu_set_screen(app, SCREEN_HOME);
            return MENU_ACTION_NONE;
        }
    }
    if (button_clicked(bottom_button(190.0f, GetScreenHeight() - 88.0f)) ||
        IsKeyPressed(KEY_ESCAPE)) {
        game_play_ui_click();
        menu_set_screen(app, SCREEN_HOME);
    }
    return MENU_ACTION_NONE;
}

static MenuAction update_name_entry(AppState *app)
{
    int character;
    Rectangle submit = bottom_button(220.0f, GetScreenHeight() / 2.0f + 133.0f);

    while ((character = GetCharPressed()) > 0) {
        if (character >= 32 && character <= 126 &&
            app->name_length < SCORE_NAME_MAX) {
            unsigned char value = (unsigned char)character;
            if ((isalnum(value) || value == ' ') &&
                !(value == ' ' && app->name_length == 0)) {
                char candidate[SCORE_NAME_MAX + 1];
                memcpy(candidate, app->name_input, app->name_length);
                candidate[app->name_length] = (char)value;
                candidate[app->name_length + 1] = '\0';
                if (MeasureText(candidate, 28) < 540) {
                    app->name_input[app->name_length++] = (char)value;
                    app->name_input[app->name_length] = '\0';
                    set_name_error(app, false);
                }
            }
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && app->name_length > 0) {
        app->name_input[--app->name_length] = '\0';
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        menu_set_screen(app, SCREEN_HOME);
        return MENU_ACTION_NONE;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
        button_clicked(submit)) {
        game_play_ui_click();
        return submit_name(app);
    }
    return MENU_ACTION_NONE;
}

MenuAction menu_update(AppState *app)
{
    if (app->screen != SCREEN_NAME_ENTRY) {
        int character;
        while ((character = GetCharPressed()) > 0) {
            (void)character;
        }
    }

    switch (app->screen) {
    case SCREEN_HOME:
        return update_home(app);
    case SCREEN_DIFFICULTY:
        return update_difficulty(app);
    case SCREEN_NAME_ENTRY:
        return update_name_entry(app);
    case SCREEN_INSTRUCTIONS:
    case SCREEN_HIGH_SCORES:
        if (button_clicked(bottom_button(190.0f, GetScreenHeight() - 88.0f)) ||
            IsKeyPressed(KEY_ESCAPE)) {
            game_play_ui_click();
            menu_set_screen(app, SCREEN_HOME);
        }
        return MENU_ACTION_NONE;
    case SCREEN_GAME_OVER:
        if (button_clicked(bottom_button(230.0f, GetScreenHeight() / 2.0f + 104.0f))) {
            game_play_ui_click();
            menu_set_screen(app, SCREEN_PLAYING);
            return MENU_ACTION_START_GAME;
        }
        if (button_clicked(bottom_button(230.0f, GetScreenHeight() / 2.0f + 173.0f)) ||
            IsKeyPressed(KEY_ESCAPE)) {
            game_play_ui_click();
            menu_set_screen(app, SCREEN_HOME);
        }
        return MENU_ACTION_NONE;
    case SCREEN_PLAYING:
    default:
        return MENU_ACTION_NONE;
    }
}

static void draw_home(const AppState *app)
{
    int index;
    const char *labels[BUTTON_COUNT] = {
        "START GAME", "SELECT DIFFICULTY", "INSTRUCTIONS",
        "HIGH SCORES", "EXIT"
    };

    draw_header("BRAIN MATCH", "TEST YOUR MEMORY");
    DrawRectangleRounded((Rectangle){GetScreenWidth() / 2.0f - 5.0f, 216, 10, 4},
                         0.5f, 5, COLOR_CYAN);
    for (index = 0; index < BUTTON_COUNT; index++) {
        draw_button(home_button(index), labels[index], index == 0, true);
    }
    center_text(TextFormat("CURRENT LEVEL: %s", difficulty_label(app->difficulty)),
                646, 16, COLOR_TEXT_MUTED);
}

static void draw_difficulty(void)
{
    const char *names[] = {"EASY", "MEDIUM", "HARD"};
    const char *details[] = {
        "Digits 1 - 4", "Letters A - F", "Letters and digits"
    };
    const char *speeds[] = {
        "Relaxed pace", "Steady pace", "Fast pace"
    };
    float card_width = 250.0f;
    float gap = 22.0f;
    float total_width = card_width * 3.0f + gap * 2.0f;
    float start_x = (GetScreenWidth() - total_width) / 2.0f;
    int index;

    draw_header("CHOOSE YOUR CHALLENGE", "Select a difficulty to return home");
    for (index = 0; index < 3; index++) {
        Rectangle card = {start_x + index * (card_width + gap),
                          GetScreenHeight() / 2.0f - 76.0f,
                          card_width, 200.0f};
        bool hovered = CheckCollisionPointRec(GetMousePosition(), card);
        Color accent = index == 0 ? (Color){97, 218, 178, 255}
                                  : index == 1 ? (Color){125, 166, 255, 255}
                                               : (Color){208, 126, 255, 255};
        DrawRectangleRounded(card, 0.12f, 12,
                             hovered ? (Color){255, 255, 255, 35} : COLOR_PANEL);
        DrawRectangleRoundedLinesEx(card, 0.12f, 12, hovered ? 2.0f : 1.0f,
                                    Fade(accent, hovered ? 0.9f : 0.45f));
        DrawText(names[index],
                 (int)(card.x + (card.width - MeasureText(names[index], 29)) / 2),
                 (int)card.y + 39, 29, accent);
        DrawText(details[index],
                 (int)(card.x + (card.width - MeasureText(details[index], 18)) / 2),
                 (int)card.y + 94, 18, RAYWHITE);
        DrawText(speeds[index],
                 (int)(card.x + (card.width - MeasureText(speeds[index], 15)) / 2),
                 (int)card.y + 131, 15, COLOR_TEXT_MUTED);
        DrawText(index == 0 ? "4 symbols   /   3 lives"
                            : index == 1 ? "6 symbols   /   3 lives"
                                         : "8 symbols   /   3 lives",
                 (int)(card.x + (card.width - MeasureText(
                     index == 0 ? "4 symbols   /   3 lives"
                                : index == 1 ? "6 symbols   /   3 lives"
                                             : "8 symbols   /   3 lives",
                     14)) / 2),
                 (int)card.y + 164, 14, (Color){200, 206, 232, 255});
    }
    draw_button(bottom_button(190.0f, GetScreenHeight() - 88.0f),
                "BACK", false, true);
}

static void draw_instructions(void)
{
    const char *lines[] = {
        "Watch each symbol as it appears, then repeat the full sequence.",
        "Type the sequence in order and press Enter or click SUBMIT.",
        "Backspace edits your answer; CLEAR removes it.",
        "Correct rounds add a new symbol and increase your score.",
        "You have three lives. A wrong answer or timeout costs one life.",
        "Each difficulty uses a faster sequence and a shorter answer timer."
    };
    int index;

    draw_header("HOW TO PLAY", "A little focus goes a long way");
    for (index = 0; index < 6; index++) {
        DrawCircle(GetScreenWidth() / 2.0f - 310.0f,
                   260 + index * 47, 4, COLOR_CYAN);
        DrawText(lines[index], GetScreenWidth() / 2 - 290,
                 251 + index * 47, 18, RAYWHITE);
    }
    draw_button(bottom_button(190.0f, GetScreenHeight() - 88.0f),
                "BACK", false, true);
}

static void draw_high_scores(const AppState *app)
{
    int index;
    int row_y = 253;
    int row_gap = (GetScreenHeight() - 340) / 10;

    if (row_gap > 38) {
        row_gap = 38;
    }

    draw_header("HIGH SCORES", "Top 10 memory champions");
    DrawRectangleRounded((Rectangle){GetScreenWidth() / 2.0f - 390.0f,
                                     194, 780, 48},
                         0.16f, 8, (Color){255, 255, 255, 24});
    DrawText("RANK", GetScreenWidth() / 2 - 365, 209, 15, COLOR_TEXT_MUTED);
    DrawText("PLAYER", GetScreenWidth() / 2 - 285, 209, 15, COLOR_TEXT_MUTED);
    DrawText("LEVEL", GetScreenWidth() / 2 + 95, 209, 15, COLOR_TEXT_MUTED);
    DrawText("SCORE", GetScreenWidth() / 2 + 190, 209, 15, COLOR_TEXT_MUTED);
    DrawText("MODE", GetScreenWidth() / 2 + 292, 209, 15, COLOR_TEXT_MUTED);

    if (app->score_count == 0) {
        center_text("No scores yet. Complete a game to be first!",
                    330, 19, COLOR_TEXT_MUTED);
    }
    for (index = 0; index < (int)app->score_count; index++) {
        const ScoreEntry *entry = &app->scores[index];
        Color rank_color = index == 0 ? (Color){255, 210, 105, 255}
                                      : index == 1 ? (Color){206, 218, 236, 255}
                                                   : RAYWHITE;
        DrawRectangleRounded((Rectangle){GetScreenWidth() / 2.0f - 390.0f,
                                         (float)row_y - 5, 780, (float)row_gap - 2},
                             0.16f, 8,
                             index % 2 == 0 ? (Color){255, 255, 255, 12}
                                           : (Color){255, 255, 255, 5});
        DrawText(TextFormat("%02d", index + 1),
                 GetScreenWidth() / 2 - 365, row_y + 5, 17, rank_color);
        DrawText(entry->name, GetScreenWidth() / 2 - 285, row_y + 5,
                 17, RAYWHITE);
        DrawText(TextFormat("%d", entry->level),
                 GetScreenWidth() / 2 + 100, row_y + 5, 17, COLOR_TEXT_MUTED);
        DrawText(TextFormat("%d", entry->score),
                 GetScreenWidth() / 2 + 190, row_y + 5, 17, COLOR_CYAN);
        DrawText(entry->difficulty, GetScreenWidth() / 2 + 292,
                 row_y + 5, 16, COLOR_TEXT_MUTED);
        row_y += row_gap;
    }
    draw_button(bottom_button(190.0f, GetScreenHeight() - 88.0f),
                "BACK", false, true);
}

static void draw_name_entry(const AppState *app)
{
    Rectangle input = {GetScreenWidth() / 2.0f - 300.0f,
                       GetScreenHeight() / 2.0f - 17.0f, 600.0f, 70.0f};
    float caret_visible = fmod(GetTime(), 1.0) < 0.55 ? 1.0f : 0.0f;

    draw_header("WHO'S PLAYING?", "Enter your name to begin");
    DrawRectangleRounded(input, 0.16f, 10, (Color){255, 255, 255, 24});
    DrawRectangleRoundedLinesEx(input, 0.16f, 10, 1.5f,
                                Fade(COLOR_CYAN, 0.7f));
    DrawText(app->name_input, (int)input.x + 22, (int)input.y + 18, 28, RAYWHITE);
    if (caret_visible > 0.0f) {
        int text_width = MeasureText(app->name_input, 28);
        DrawRectangle((int)input.x + 24 + text_width, (int)input.y + 20,
                      2, 31, COLOR_CYAN);
    }
    if (app->name_error) {
        center_text("Please enter a name before continuing.",
                    (int)input.y + 88, 16, (Color){255, 145, 165, 255});
    }
    draw_button(bottom_button(220.0f, GetScreenHeight() / 2.0f + 133.0f),
                "START", true, true);
    center_text("Letters and numbers   |   Enter to continue   |   Esc to go back",
                GetScreenHeight() - 45, 14, COLOR_TEXT_MUTED);
}

static void draw_game_over(const AppState *app)
{
    const Game *game = &app->game;
    char summary[96];
    char accuracy[96];

    DrawCircleGradient((Vector2){GetScreenWidth() / 2.0f,
                                 GetScreenHeight() / 2.0f - 68.0f},
                       210.0f, (Color){224, 102, 150, 34},
                       (Color){224, 102, 150, 0});
    center_text("GAME OVER", GetScreenHeight() / 2 - 206, 48, RAYWHITE);
    center_text(TextFormat("Well played, %s!", app->player_name),
                GetScreenHeight() / 2 - 145, 20, COLOR_TEXT_MUTED);
    center_text(TextFormat("%d", game->score),
                GetScreenHeight() / 2 - 64, 64, COLOR_CYAN);
    center_text("FINAL SCORE", GetScreenHeight() / 2 + 8, 15, COLOR_TEXT_MUTED);
    snprintf(summary, sizeof(summary), "Level reached: %d      Correct: %d",
             game->level, game->correct_answers);
    snprintf(accuracy, sizeof(accuracy), "Incorrect: %d      Time played: %ld sec",
             game->incorrect_answers, game_elapsed_seconds(game));
    center_text(summary, GetScreenHeight() / 2 + 48, 18, RAYWHITE);
    center_text(accuracy, GetScreenHeight() / 2 + 78, 16, COLOR_TEXT_MUTED);
    draw_button(bottom_button(230.0f, GetScreenHeight() / 2.0f + 128.0f),
                "PLAY AGAIN", true, true);
    draw_button(bottom_button(230.0f, GetScreenHeight() / 2.0f + 197.0f),
                "RETURN HOME", false, true);
}

void menu_draw(const AppState *app)
{
    if (app->screen != SCREEN_PLAYING) {
        draw_background();
    }
    switch (app->screen) {
    case SCREEN_HOME:
        draw_home(app);
        break;
    case SCREEN_DIFFICULTY:
        draw_difficulty();
        break;
    case SCREEN_INSTRUCTIONS:
        draw_instructions();
        break;
    case SCREEN_HIGH_SCORES:
        draw_high_scores(app);
        break;
    case SCREEN_NAME_ENTRY:
        draw_name_entry(app);
        break;
    case SCREEN_PLAYING:
        game_draw(&app->game, app->player_name);
        break;
    case SCREEN_GAME_OVER:
        draw_game_over(app);
        break;
    }
    draw_notice(app);
}
