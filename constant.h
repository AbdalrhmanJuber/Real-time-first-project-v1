#ifndef COMM_H
#define COMM_H

#define MAX_PLAYERS  8
#define TEAM_SIZE    4

#include <signal.h>

// Additional signals
#define SIG_ENERGY_REQ  SIGUSR1   // parent -> child: "report your raw energy"
#define SIG_SET_LOC     SIGUSR2   // parent -> child: "here is your assigned location"
#define SIG_READY       SIGRTMIN  // real-time signals can be used if you prefer
#define SIG_PULL        (SIGRTMIN+1)
#define SIG_TERMINATE   SIGTERM

#define TEAM1 0
#define TEAM2 1

// This is your existing data structure
typedef struct {
    int id;          // 0..3
    int team;        // TEAM1 or TEAM2
    int energy;
    int decay_rate;
    int is_fallen;
    int location;    // 0..3
    int fall_time_left;
} PlayerData;

// Weighted effort -> already used
typedef struct {
    int player_id;
    int team;
    int weighted_effort;
} EffortMessage;

// Child -> Parent: raw energy
typedef struct {
    int player_id;
    int team;
    int energy; // raw
} EnergyReply;

// Parent -> Child: assigned location
typedef struct {
    int player_id;
    int location;
} LocationMessage;

#endif
