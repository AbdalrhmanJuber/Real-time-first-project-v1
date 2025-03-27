// config.h
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int energy_min;
    int energy_max;

    int decay_min;
    int decay_max;

    int fall_recover_min;
    int fall_recover_max;

    int win_threshold;
    int max_game_time;
    int max_score;
    int consecutive_wins;
} GameConfig;

int load_config(const char *filename, GameConfig *cfg);

#endif
