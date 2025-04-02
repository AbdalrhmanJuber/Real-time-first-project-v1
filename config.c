// config.c
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

int load_config(const char *filename, GameConfig *cfg)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open config file");
        return -1;
    }

    char key[50];
    int read_values;
    
    while (fscanf(fp, "%s", key) != EOF) {
        if (strcmp(key, "energy_range") == 0) {
            read_values = fscanf(fp, "%d %d", &cfg->energy_min, &cfg->energy_max);
            if (read_values != 2 || cfg->energy_min < 0 || cfg->energy_max < cfg->energy_min) {
                printf("❌ Invalid energy_range values! Must be positive and max > min.\n");
                fclose(fp);
                return -1;
            }
        } 
        else if (strcmp(key, "decay_range") == 0) {
            read_values = fscanf(fp, "%d %d", &cfg->decay_min, &cfg->decay_max);
            if (read_values != 2 || cfg->decay_min < 0 || cfg->decay_max < cfg->decay_min) {
                printf("❌ Invalid decay_range values! Must be positive and max > min.\n");
                fclose(fp);
                return -1;
            }
        } 
        else if (strcmp(key, "fall_recover_range") == 0) {
            read_values = fscanf(fp, "%d %d", &cfg->fall_recover_min, &cfg->fall_recover_max);
            if (read_values != 2 || cfg->fall_recover_min < 0 || cfg->fall_recover_max < cfg->fall_recover_min) {
                printf("❌ Invalid fall_recover_range values! Must be positive and max > min.\n");
                fclose(fp);
                return -1;
            }
        } 
        else if (strcmp(key, "win_threshold") == 0) {
            read_values = fscanf(fp, "%d", &cfg->win_threshold);
            if (read_values != 1 || cfg->win_threshold <= 0) {
                printf("❌ Invalid win_threshold! Must be a positive number.\n");
                fclose(fp);
                return -1;
            }
        } 
        else if (strcmp(key, "max_game_time") == 0) {
            read_values = fscanf(fp, "%d", &cfg->max_game_time);
            if (read_values != 1 || cfg->max_game_time <= 0) {
                printf("❌ Invalid max_game_time! Must be a positive number.\n");
                fclose(fp);
                return -1;
            }
        } 
        else if (strcmp(key, "max_score") == 0) {
            read_values = fscanf(fp, "%d", &cfg->max_score);
            if (read_values != 1 || cfg->max_score < 0) {
                printf("❌ Invalid max_score! Must be a non-negative number.\n");
                fclose(fp);
                return -1;
            }
        } 
        else if (strcmp(key, "consecutive_wins") == 0) {
            read_values = fscanf(fp, "%d", &cfg->consecutive_wins);
            if (read_values != 1 || cfg->consecutive_wins < 1) {
                printf("❌ Invalid consecutive_wins! Must be at least 1.\n");
                fclose(fp);
                return -1;
            }
        } 
        else {
            printf("⚠️ Unknown config key: %s\n", key);
            fclose(fp);
            return -1;
        }
    }
    
    fclose(fp);
    return 0;
}