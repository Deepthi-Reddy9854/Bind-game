#ifndef BRAIN_MATCH_SCORE_H
#define BRAIN_MATCH_SCORE_H

#include <stdbool.h>
#include <stddef.h>

#define SCORE_FILE "highscores.txt"
#define SCORE_NAME_MAX 30
#define SCORE_DIFFICULTY_MAX 15
#define SCORE_TABLE_SIZE 10

typedef struct {
    char name[SCORE_NAME_MAX + 1];
    char difficulty[SCORE_DIFFICULTY_MAX + 1];
    int score;
    int level;
    long seconds;
} ScoreEntry;

void scores_storage_begin(void);
bool scores_storage_ready(void);
bool scores_storage_failed(void);

bool scores_load(ScoreEntry entries[SCORE_TABLE_SIZE],
                 size_t *count,
                 char *error,
                 size_t error_size);
bool scores_insert(ScoreEntry entries[SCORE_TABLE_SIZE],
                   size_t *count,
                   const ScoreEntry *entry);
bool scores_save(const ScoreEntry entries[SCORE_TABLE_SIZE],
                 size_t count,
                 char *error,
                 size_t error_size);

#endif
