// constant.h
#ifndef COMM_H
#define COMM_H

#define MAX_PLAYERS 8
#define TEAM_SIZE 4

// Signals used in the project
#define SIG_READY  SIGUSR1
#define SIG_PULL   SIGUSR2
#define SIG_TERMINATE SIGTERM

// Team IDs
#define TEAM1 0
#define TEAM2 1

// Structure to hold a player’s data
typedef struct {
    int id;           // Player ID (0-3)
    int team;         // TEAM1 or TEAM2
    int energy;       // Current energy level
    int decay_rate;   // Energy loss per second
    int is_fallen;    // 0 or 1
    int location;     // 0 (middle) to 3 (end of rope)
} PlayerData;

// Structure to represent weighted effort sent to parent
typedef struct {
    int player_id;
    int team;
    int weighted_effort;
} EffortMessage;

#endif
