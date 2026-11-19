#include "score.h"

#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message);

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

static volatile int storage_status;
static volatile int sync_status;

EMSCRIPTEN_KEEPALIVE void score_storage_ready_callback(int success)
{
    storage_status = success ? 1 : -1;
}

EMSCRIPTEN_KEEPALIVE void score_storage_sync_callback(int success)
{
    sync_status = success ? 1 : -1;
}

void scores_storage_begin(void)
{
    storage_status = 0;
    EM_ASM({
        try {
            if (!FS.analyzePath('/persist').exists) FS.mkdir('/persist');
            FS.mount(IDBFS, {}, '/persist');
            FS.syncfs(true, function(error) {
                Module._score_storage_ready_callback(error ? 0 : 1);
            });
        } catch (error) {
            console.error('Could not initialize saved scores:', error);
            Module._score_storage_ready_callback(0);
        }
    });
}

bool scores_storage_ready(void)
{
    return storage_status == 1;
}

bool scores_storage_failed(void)
{
    return storage_status < 0;
}

static const char *score_file_path(void)
{
    return "/persist/" SCORE_FILE;
}

static bool sync_browser_storage(char *error, size_t error_size)
{
    sync_status = 0;
    EM_ASM({
        FS.syncfs(false, function(error) {
            Module._score_storage_sync_callback(error ? 0 : 1);
        });
    });
    while (sync_status == 0) {
        emscripten_sleep(10);
    }
    if (sync_status < 0) {
        set_error(error, error_size, "Could not persist browser high scores.");
        return false;
    }
    return true;
}
#else
void scores_storage_begin(void)
{
}

bool scores_storage_ready(void)
{
    return true;
}

bool scores_storage_failed(void)
{
    return false;
}

static const char *score_file_path(void)
{
    return SCORE_FILE;
}
#endif

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) {
        snprintf(error, error_size, "%s", message);
    }
}

bool scores_load(ScoreEntry entries[SCORE_TABLE_SIZE],
                 size_t *count,
                 char *error,
                 size_t error_size)
{
    FILE *file;
    char line[256];

    *count = 0;
    file = fopen(score_file_path(), "r");
    if (file == NULL) {
        file = fopen(score_file_path(), "a");
        if (file == NULL) {
            set_error(error, error_size, "Could not create highscores.txt.");
            return false;
        }
        if (fclose(file) != 0) {
            set_error(error, error_size, "Could not close highscores.txt.");
            return false;
        }
        return true;
    }

    while (*count < SCORE_TABLE_SIZE && fgets(line, sizeof(line), file) != NULL) {
        ScoreEntry entry;
        int fields = sscanf(line, "%d|%30[^|]|%15[^|]|%d|%ld",
                            &entry.score, entry.name, entry.difficulty,
                            &entry.level, &entry.seconds);
        if (fields == 5 && entry.score >= 0 && entry.level >= 0 &&
            entry.seconds >= 0) {
            entries[(*count)++] = entry;
        }
    }

    if (ferror(file)) {
        fclose(file);
        set_error(error, error_size, "Could not read highscores.txt.");
        return false;
    }
    if (fclose(file) != 0) {
        set_error(error, error_size, "Could not close highscores.txt.");
        return false;
    }
    return true;
}

bool scores_insert(ScoreEntry entries[SCORE_TABLE_SIZE],
                   size_t *count,
                   const ScoreEntry *entry)
{
    size_t position = 0;
    size_t index;

    while (position < *count &&
           (entries[position].score > entry->score ||
            (entries[position].score == entry->score &&
             entries[position].level >= entry->level))) {
        position++;
    }
    if (position >= SCORE_TABLE_SIZE) {
        return false;
    }
    if (*count < SCORE_TABLE_SIZE) {
        (*count)++;
    }
    for (index = *count - 1; index > position; index--) {
        entries[index] = entries[index - 1];
    }
    entries[position] = *entry;
    return true;
}

bool scores_save(const ScoreEntry entries[SCORE_TABLE_SIZE],
                 size_t count,
                 char *error,
                 size_t error_size)
{
    FILE *file = fopen(score_file_path(), "w");
    size_t index;

    if (file == NULL) {
        set_error(error, error_size, "Could not write highscores.txt.");
        return false;
    }
    for (index = 0; index < count; index++) {
        if (fprintf(file, "%d|%s|%s|%d|%ld\n",
                    entries[index].score, entries[index].name,
                    entries[index].difficulty, entries[index].level,
                    entries[index].seconds) < 0) {
            fclose(file);
            set_error(error, error_size, "Could not write the score table.");
            return false;
        }
    }
    if (fclose(file) != 0) {
        set_error(error, error_size, "Could not finish saving highscores.txt.");
        return false;
    }
#if defined(__EMSCRIPTEN__)
    if (!sync_browser_storage(error, error_size)) {
        return false;
    }
#endif
    return true;
}
