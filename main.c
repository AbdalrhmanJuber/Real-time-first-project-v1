#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

#include "constant.h"
#include "config.h"
#include "pipe.h"

int effort_pipes[MAX_PLAYERS][2]; // child->parent
int loc_pipes[MAX_PLAYERS][2];    // parent->child

pid_t players[MAX_PLAYERS];       // 8 child PIDs
GameConfig cfg;                   // config

int team_scores[2] = {0,0};
int consecutive_wins[2] = {0,0};

// spawn each child, passing 6 arguments
void spawn_players() {
    for (int i=0; i<MAX_PLAYERS; i++){
        // create the location pipe too
        if (pipe(loc_pipes[i]) == -1) {
            perror("pipe loc failed");
            exit(1);
        }

        int team_id   = (i < TEAM_SIZE) ? TEAM1 : TEAM2;
        int player_id = (team_id == TEAM1) ? i : i - TEAM_SIZE;

        int init_energy = rand() % (cfg.energy_max - cfg.energy_min + 1) + cfg.energy_min;
        int decay       = rand() % (cfg.decay_max - cfg.decay_min + 1) + cfg.decay_min;

        char buf_id[16], buf_team[16], buf_decay[16], buf_energy[16];
        char buf_write_effort[16], buf_read_loc[16];

        sprintf(buf_id, "%d", player_id);
        sprintf(buf_team, "%d", team_id);
        sprintf(buf_decay, "%d", decay);
        sprintf(buf_energy, "%d", init_energy);
        // child->parent => effort_pipes[i][1]
        sprintf(buf_write_effort, "%d", effort_pipes[i][1]);
        // parent->child => loc_pipes[i][0] (child reads)
        sprintf(buf_read_loc, "%d", loc_pipes[i][0]);

        pid_t pid=fork();
        if (pid==0) {
            // child
            // close parent's ends
            close(effort_pipes[i][0]); // child not reading from effort pipe
            close(loc_pipes[i][1]);    // child not writing to loc pipe

            execl("./player","./player",
                  buf_id, buf_team, buf_decay, buf_energy,
                  buf_write_effort, buf_read_loc,
                  (char*)NULL);
            perror("execl failed");
            exit(1);
        } else {
            players[i] = pid;
            // parent
            close(effort_pipes[i][1]); // parent won't write child's effort pipe
            close(loc_pipes[i][0]);    // parent won't read from child's loc pipe
        }
    }
}

// Request raw energy from each child, gather replies, assign location
void assign_locations() {
    // 1) Ask for energy
    for (int i=0; i<MAX_PLAYERS; i++){
        kill(players[i], SIG_ENERGY_REQ);
    }

    // 2) read 8 replies (EnergyReply)
    // We'll store them in arrays for Team1 & Team2
    typedef struct { int id; int energy; } SimplePlayer;
    SimplePlayer t1[TEAM_SIZE], t2[TEAM_SIZE];
    int c1=0, c2=0;

    int replies = 0;
    while (replies < MAX_PLAYERS) {
        EnergyReply er;
        int got = read_effort(effort_pipes[replies][0], &er, sizeof(er));
        if (got == sizeof(er)) {
            if (er.team == TEAM1) {
                t1[c1].id     = er.player_id; // 0..3
                t1[c1].energy = er.energy;
                c1++;
            } else {
                t2[c2].id     = er.player_id; // 0..3
                t2[c2].energy = er.energy;
                c2++;
            }
            replies++;
        }
    }

    // 3) sort each
    for (int i=0; i<TEAM_SIZE-1; i++){
        for (int j=i+1; j<TEAM_SIZE; j++){
            if (t1[i].energy > t1[j].energy){
                SimplePlayer tmp=t1[i]; t1[i]=t1[j]; t1[j]=tmp;
            }
            if (t2[i].energy > t2[j].energy){
                SimplePlayer tmp=t2[i]; t2[i]=t2[j]; t2[j]=tmp;
            }
        }
    }

    // 4) assign location
    // smallest energy => location=0, next =>1, etc.
    // then send location to each child via loc_pipes
    for (int i=0; i<TEAM_SIZE; i++){
        // For Team1
        LocationMessage lm1;
        lm1.player_id = t1[i].id;
        lm1.location  = i;  // 0..3
        // We'll compute the parent's "index" in players[] for that child. If T1 => same
        int pipe_idx1 = t1[i].id;   // if T1 is i => 0..3
        write_effort(loc_pipes[pipe_idx1][1], &lm1, sizeof(lm1));
        kill(players[pipe_idx1], SIG_SET_LOC);

        // For Team2
        LocationMessage lm2;
        lm2.player_id = t2[i].id;   // 0..3
        lm2.location  = i; 
        int pipe_idx2 = t2[i].id; // if T2 => child index is 4..7
        write_effort(loc_pipes[pipe_idx2][1], &lm2, sizeof(lm2));
        kill(players[pipe_idx2], SIG_SET_LOC);
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    if (load_config(argv[1], &cfg) != 0) {
        return 1;
    }

    // create the child->parent effort pipes
    if (init_pipes(effort_pipes, MAX_PLAYERS)<0){
        return 1;
    }

    // spawn children
    spawn_players();
    usleep(10000); // let them start
    
    // Time tracking
    struct timeval start_time, current_time;
    gettimeofday(&start_time,NULL);

    int total_rounds=0;
    int last_winner=-1;
    while (1) {
        total_rounds++;
        printf("\n===== START ROUND %d =====\n", total_rounds);

        // *** 1) parent-based location assignment before SIG_READY
        assign_locations();

        // *** 2) now that children have location, we can signal them "ready"
        for (int i=0; i<MAX_PLAYERS; i++){
            kill(players[i], SIG_READY);
        }
        sleep(1); // let them reorder (they already have location though)

        // *** 3) send SIG_PULL
        for (int i=0; i<MAX_PLAYERS; i++){
            kill(players[i], SIG_PULL);
        }

        // *** 4) same as your existing round logic
        int round_winner = -1;
        int round_duration = 10;
        for (int t=0; t<round_duration; t++){
            sleep(1);

            int sum_t1=0, sum_t2=0;
            EffortMessage em;
            // read from each child's pipe
            for (int i=0; i<MAX_PLAYERS; i++){
                int n= read_effort(effort_pipes[i][0], &em, sizeof(em));
                if (n==sizeof(em)){
                    if (em.team==TEAM1) sum_t1 += em.weighted_effort;
                    else sum_t2 += em.weighted_effort;
                }
            }
            printf("[Round %d, sec %d] T1=%d, T2=%d\n", total_rounds, t+1, sum_t1, sum_t2);

            if (sum_t1 >= cfg.win_threshold || sum_t2 >= cfg.win_threshold) {
                round_winner = (sum_t1 > sum_t2) ? TEAM1 : TEAM2;
                break;
            }

            gettimeofday(&current_time,NULL);
            long elapsed= current_time.tv_sec - start_time.tv_sec;
            if (elapsed >= cfg.max_game_time){
                printf("Time limit => end\n");
                round_winner = (sum_t1>sum_t2)?TEAM1:TEAM2;
                break;
            }
        }
        // tie-break
        if (round_winner<0){
            int sum_t1=0, sum_t2=0;
            EffortMessage em;
            for (int i=0; i<MAX_PLAYERS; i++){
                int n= read_effort(effort_pipes[i][0], &em, sizeof(em));
                if (n==sizeof(em)){
                    if (em.team==TEAM1) sum_t1 += em.weighted_effort;
                    else sum_t2 += em.weighted_effort;
                }
            }
            if (sum_t1>sum_t2) round_winner=TEAM1;
            else if (sum_t2>sum_t1) round_winner=TEAM2;
            else round_winner = (rand()%2);
        }

        printf("=== Winner of round %d is Team %d ===\n", total_rounds, round_winner);

        // scoring logic
        team_scores[round_winner]++;
        if (round_winner==last_winner) {
            consecutive_wins[round_winner]++;
        } else {
            consecutive_wins[TEAM1]=0;
            consecutive_wins[TEAM2]=0;
            consecutive_wins[round_winner]=1;
        }
        last_winner= round_winner;

        // end conditions
        if (team_scores[TEAM1]>= cfg.max_score || team_scores[TEAM2]>= cfg.max_score) {
            printf("Team %d reached max_score => end game.\n", round_winner);
            break;
        }
        if (consecutive_wins[round_winner]>= cfg.consecutive_wins){
            printf("Team %d got %d consecutive => end\n", round_winner, consecutive_wins[round_winner]);
            break;
        }
        gettimeofday(&current_time,NULL);
        long elapsed= current_time.tv_sec - start_time.tv_sec;
        if (elapsed>= cfg.max_game_time){
            printf("Time limit => end\n");
            break;
        }
    }

    // terminate
    for (int i=0; i<MAX_PLAYERS; i++){
        kill(players[i], SIG_TERMINATE);
    }
    for (int i=0; i<MAX_PLAYERS; i++){
        wait(NULL);
    }
    printf("\n==== Final Score ====\n");
    printf("Team1=%d, Team2=%d\n",team_scores[TEAM1], team_scores[TEAM2]);
    printf("Bye!\n");
    return 0;
}