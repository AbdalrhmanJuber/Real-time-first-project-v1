#include <stdio.h>
#include <string.h>
#include "config.h"

int load_config(const char *filename, GameConfig *cfg)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open config file");
        return -1;
    }

    char key[50];
    while (fscanf(fp, "%s", key) != EOF) {
        if (strcmp(key, "energy_range") == 0)
            fscanf(fp, "%d %d", &cfg->energy_min, &cfg->energy_max);
        else if (strcmp(key, "decay_range") == 0)
            fscanf(fp, "%d %d", &cfg->decay_min, &cfg->decay_max);
        else if (strcmp(key, "fall_recover_range") == 0)
            fscanf(fp, "%d %d", &cfg->fall_recover_min, &cfg->fall_recover_max);
        else if (strcmp(key, "win_threshold") == 0)
            fscanf(fp, "%d", &cfg->win_threshold);
        else if (strcmp(key, "max_game_time") == 0)
            fscanf(fp, "%d", &cfg->max_game_time);
        else if (strcmp(key, "max_score") == 0)
            fscanf(fp, "%d", &cfg->max_score);
        else if (strcmp(key, "consecutive_wins") == 0)
            fscanf(fp, "%d", &cfg->consecutive_wins);
    }
    fclose(fp);
    return 0;
}
