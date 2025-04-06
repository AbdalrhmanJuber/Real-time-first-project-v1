#ifndef GAME_STATE_H
#define GAME_STATE_H

typedef struct {
    float team1_energy[4];
    float team2_energy[4];
    int round_number;
    int team1_effort_sum;
    int team2_effort_sum;
    int round_winner;  // -1 if not determined yet, 0 for Team1, 1 for Team2
    char status_message[50];  // For showing round info, winners, etc.
    int game_state;    // 0: new round, 1: playing, 2: round ended
} GameState;

#endif