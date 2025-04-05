#ifndef COMM_H
#define COMM_H

#define MAX_PLAYERS  8
#define TEAM_SIZE    4

// Signal Assignments
#define SIG_ENERGY_REQ  SIGUSR1       // Request energy report
#define SIG_SET_LOC     SIGUSR2       // Assign location
#define SIG_READY       SIGRTMIN      // Ready state (RT signal 0)
#define SIG_PULL        (SIGRTMIN+1)  // Start pulling (RT signal 1)
#define SIG_STOP        (SIGRTMIN+2)  // Stop pulling (RT signal 2)
#define SIG_RESET_ENERGY (SIGRTMIN+3) // Reset energy (RT signal 3)
#define SIG_TERMINATE   SIGTERM       // Terminate process

#define TEAM1 0
#define TEAM2 1

// Data structures (unchanged)
typedef struct {
    int id;
    int team;
    int energy;
    int decay_rate;
    int is_fallen;
    int location;
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