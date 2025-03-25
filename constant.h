
#ifndef COMM_H
#define COMM_H

#define MAX_PLAYERS     8
#define TEAM_SIZE       4

#include <signal.h>
#define SIG_READY       SIGUSR1
#define SIG_PULL        SIGUSR2
#define SIG_TERMINATE   SIGTERM

#define TEAM1 0
#define TEAM2 1

typedef struct {
    int id;               // 0..3
    int team;             // TEAM1 or TEAM2
    int energy;           // current energy
    int decay_rate;       // used for per-second energy drop
    int is_fallen;        // 0 or 1
    int location;         // 0..3
    int fall_time_left;   // how many seconds left to recover
} PlayerData;

typedef struct {
    int player_id;
    int team;
    int weighted_effort;
} EffortMessage;

#endif
