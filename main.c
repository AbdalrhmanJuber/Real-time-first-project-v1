#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "constant.h"
#include "config.h"
#include "pipe.h"

<<<<<<< HEAD
static int graphics_pipe[2]; // parent->graphics pipe
pid_t graphics_pid = -1;     // graphics child PID
=======

pid_t graphics_pid = -1;          // graphics child PID
>>>>>>> 2c6da580472c567665bd6f3ddb4522e302d6033c

int effort_pipes[MAX_PLAYERS][2]; // child->parent
int loc_pipes[MAX_PLAYERS][2];    // parent->child
int range_energy[2]; // Store range energy values

pid_t players[MAX_PLAYERS];       // 8 child PIDs
GameConfig cfg;                   // config

int team_scores[2] = {0,0};
int consecutive_wins[2] = {0,0};

<<<<<<< HEAD
// Modified to use pipe communication
void fork_graphics_process() {
    if (pipe(graphics_pipe) == -1) {
        perror("graphics_pipe creation failed");
        exit(1);
    }
    
    graphics_pid = fork();
    
    if (graphics_pid == 0) {
        // Child: graphics process
        close(graphics_pipe[1]); // Close write end, child only reads
        
        // Convert pipe read fd to string to pass as argument
        char read_fd_str[16];
        sprintf(read_fd_str, "%d", graphics_pipe[0]);
        
        execl("./graphics", "./graphics", read_fd_str, (char*)NULL);
        perror("execl graphics failed");
        exit(1);
    } else if (graphics_pid > 0) {
        printf("[PARENT] Spawned graphics process with PID %d\n", graphics_pid);
        close(graphics_pipe[0]); // Close read end, parent only writes
    } else {
        perror("fork failed for graphics process");
        exit(1);
    }
}

=======


void fork_graphics_process() {

    graphics_pid = fork();

    if (graphics_pid == 0) {

        execl("./graphics", "./graphics", NULL);

        perror("execl graphics failed");

        exit(1);

    } else if (graphics_pid > 0) {

        printf("[PARENT] Spawned graphics process with PID %d\n", graphics_pid);

    } else {

        perror("fork failed for graphics process");

        exit(1);

    }

}



>>>>>>> 2c6da580472c567665bd6f3ddb4522e302d6033c
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
        char buf_decay_min[16], buf_decay_max[16], buf_recover_min[16], buf_recover_max[16];
        char buf_max_energy[16], buf_min_energy[16];

        sprintf(buf_id, "%d", player_id);
        sprintf(buf_team, "%d", team_id);
        sprintf(buf_decay, "%d", decay);
        sprintf(buf_energy, "%d", init_energy);
        // child->parent => effort_pipes[i][1]
        sprintf(buf_write_effort, "%d", effort_pipes[i][1]);
        // parent->child => loc_pipes[i][0] (child reads)
        sprintf(buf_read_loc, "%d", loc_pipes[i][0]);
        // decay_min/max and recover_min/max are passed to the child
        sprintf(buf_decay_min, "%d", cfg.decay_min);
        sprintf(buf_decay_max, "%d", cfg.decay_max);
        sprintf(buf_recover_min, "%d", cfg.fall_recover_min);
        sprintf(buf_recover_max, "%d", cfg.fall_recover_max);
        sprintf(buf_max_energy, "%d", cfg.energy_max);
        sprintf(buf_min_energy, "%d", cfg.energy_min);


        pid_t pid=fork();
        if (pid==0) {
            // child
            // close parent's ends
            close(effort_pipes[i][0]); // child not reading from effort pipe
            close(loc_pipes[i][1]);    // child not writing to loc pipe

            execl("./player", "./player",
                buf_id,          // argv[1]
                buf_team,        // argv[2]
                buf_decay,       // argv[3]
                buf_energy,      // argv[4]
                buf_write_effort,// argv[5]
                buf_read_loc,    // argv[6]
                buf_decay_min,   // argv[7]
                buf_decay_max,   // argv[8]
                buf_recover_min, // argv[9]
                buf_recover_max, // argv[10]
                buf_max_energy,  // argv[11]
                buf_min_energy,  // argv[12]
                (char*)NULL);
            perror("execl failed");
            exit(1);
        } else {
            players[i] = pid;
            // parent
            close(effort_pipes[i][1]); // parent won't write child's effort pipe
            close(loc_pipes[i][0]);    // parent won't read from child's loc pipe
        }
        range_energy[0] = cfg.energy_min; // Save initial energy
        range_energy[1] = cfg.energy_max;
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
                printf("T1: %d %d\n", t1[c1].id, t1[c1].energy);
                fflush(stdout);
                c1++;
            } else {
                t2[c2].id     = er.player_id; // 0..3
                t2[c2].energy = er.energy;
                printf("T2: %d %d\n", t2[c2].id, t2[c2].energy);
                fflush(stdout);
                c2++;
            }
            replies++;
        }
    }

    // 3) sort each
    for (int i=0; i<TEAM_SIZE-1; i++){
        for (int j=i+1; j<TEAM_SIZE; j++){
            if (t1[i].energy < t1[j].energy){
                SimplePlayer tmp=t1[i]; t1[i]=t1[j]; t1[j]=tmp;
            }
            if (t2[i].energy < t2[j].energy){
                SimplePlayer tmp=t2[i]; t2[i]=t2[j]; t2[j]=tmp;
            }
        }
    }
    usleep(10000); // let them process the energy message
    // 4) assign location
    // Bigger energy => location=0, next =>1, etc.
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
        int pipe_idx2 = t2[i].id + TEAM_SIZE;// if T2 => child index is 4..7
        write_effort(loc_pipes[pipe_idx2][1], &lm2, sizeof(lm2));
        kill(players[pipe_idx2], SIG_SET_LOC);
    }
}

void reset_players_energy() {
    for (int i=0; i<MAX_PLAYERS; i++) {
        kill(players[i], SIG_RESET_ENERGY);
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

<<<<<<< HEAD
    fork_graphics_process(); // Now uses pipe communication
    
=======
     fork_graphics_process(); 
>>>>>>> 2c6da580472c567665bd6f3ddb4522e302d6033c
    // spawn children
    spawn_players();
    usleep(10000); // let them 
    
    // Time tracking
    struct timeval start_time, current_time;
    gettimeofday(&start_time,NULL);

    // *** 1) parent-based location assignment before SIG_READY
    printf("=== Assigning locations ===\n");
    fflush(stdout);
    usleep(10000); // let them start
    printf("=== Locations assigned ===\n");
    fflush(stdout);

    usleep(100000); // let them process the location message

    int total_rounds=0;
    int last_winner=-1;
    while (1) {
        total_rounds++;
        printf("\n===== START ROUND %d =====\n", total_rounds);
<<<<<<< HEAD
        
=======
        kill(graphics_pid, SIGUSR1); // ** //


>>>>>>> 2c6da580472c567665bd6f3ddb4522e302d6033c
        reset_players_energy();
        assign_locations();
        
        usleep(100000); // Let players process reset
<<<<<<< HEAD
        
=======
>>>>>>> 2c6da580472c567665bd6f3ddb4522e302d6033c
        // *** 2) now that children have location, we can signal them "ready"
        for (int i=0; i<MAX_PLAYERS; i++){
            kill(players[i], SIG_READY);
        }
        usleep(2000000); // let them process the ready message
        printf("=== Players are ready ===\n");
        
        // *** 3) send SIG_PULL
        for (int i=0; i<MAX_PLAYERS; i++){
            kill(players[i], SIG_PULL);
        }
        usleep(10000); // let them process the pull message

        printf("=== Players are pulling ===\n");     
        fflush(stdout);
<<<<<<< HEAD
        
=======

        kill(graphics_pid, SIGUSR2); // *** //
>>>>>>> 2c6da580472c567665bd6f3ddb4522e302d6033c
        // *** 4) read from each child's pipe and compute the sum of efforts
        // *** 5) check for winner
        int round_winner = -1;
        int sum_t1=0, sum_t2=0;

        // Gather energy information for display
        typedef struct { int id; int energy; } SimplePlayer;
        SimplePlayer t1[TEAM_SIZE], t2[TEAM_SIZE];
        int c1=0, c2=0;
        
        // Initialize message to graphics
        GraphicsMessage graphics_msg;
        memset(&graphics_msg, 0, sizeof(graphics_msg));
        graphics_msg.roundNumber = total_rounds;
        graphics_msg.roundWinner = 0; // No winner yet

        // Send initial round information to graphics
        write(graphics_pipe[1], &graphics_msg, sizeof(graphics_msg));

        //int round_duration = 10; // IF you want to test with a fixed duration
        for (int t=0; ; t++){
            sleep(1);
            
            // Reset sums for this time step
            sum_t1 = 0;
            sum_t2 = 0;
            
            // Request energy from all players first
            for (int i=0; i<MAX_PLAYERS; i++){
                kill(players[i], SIG_ENERGY_REQ);
            }
            
            // Short delay to allow players to respond
            usleep(10000);
            
            // Collect energy data for display
            c1 = 0;
            c2 = 0;
            for (int i=0; i<MAX_PLAYERS; i++){
                EnergyReply er;
                int got = read_effort(effort_pipes[i][0], &er, sizeof(er));
                if (got == sizeof(er)) {
                    if (er.team == TEAM1) {
                        t1[c1].id = er.player_id;
                        t1[c1].energy = er.energy;
                        c1++;
                    } else {
                        t2[c2].id = er.player_id;
                        t2[c2].energy = er.energy;
                        c2++;
                    }
                }
            }
            
            // Read effort messages
            EffortMessage em;
            for (int i=0; i<MAX_PLAYERS; i++){
                int n= read_effort(effort_pipes[i][0], &em, sizeof(em));
                if (n==sizeof(em)){
                    if (em.team==TEAM1) sum_t1 += em.weighted_effort;
                    else sum_t2 += em.weighted_effort;
                }
            }
            
            printf("[Round %d, sec %d] T1=%d, T2=%d\n", total_rounds, t+1, sum_t1, sum_t2);

            // Update the graphics state
            for (int i=0; i<TEAM_SIZE; i++) {
                if (i < c1) {
                    graphics_msg.team1Energies[t1[i].id] = t1[i].energy;
                }
                if (i < c2) {
                    graphics_msg.team2Energies[t2[i].id] = t2[i].energy;
                }
            }
            
            graphics_msg.team1EffortSum = sum_t1;
            graphics_msg.team2EffortSum = sum_t2;
            
            // Send real-time updates to graphics
            write(graphics_pipe[1], &graphics_msg, sizeof(graphics_msg));

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

        printf("=== Winner of round %d is Team %d ===\n", total_rounds, round_winner + 1);

        // Update and send final round winner info
        graphics_msg.roundWinner = round_winner + 1; // Convert to 1-based for display
        write(graphics_pipe[1], &graphics_msg, sizeof(graphics_msg));

        // Stop all players from pulling
        for (int i = 0; i < MAX_PLAYERS; i++) {
            kill(players[i], SIG_STOP);
        }
                
        // Add a delay to ensure players exit the pulling loop
        usleep(100000); // 100ms delay

        // Stop all players from pulling
        for (int i = 0; i < MAX_PLAYERS; i++) {
            kill(players[i], SIG_STOP);
                }
                
        // Add a delay to ensure players exit the pulling loop
        usleep(100000); // 100ms delay

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

        // Give time to view the results before next round
        sleep(2);

        // end conditions
        if (team_scores[TEAM1]>= cfg.max_score || team_scores[TEAM2]>= cfg.max_score) {
            printf("Team %d reached max_score => end game.\n", round_winner+1);
            break;
        }
        if (consecutive_wins[round_winner]>= cfg.consecutive_wins){
            printf("Team %d got %d consecutive => end\n", round_winner+1, consecutive_wins[round_winner]);
            break;
        }
        gettimeofday(&current_time,NULL);
        long elapsed= current_time.tv_sec - start_time.tv_sec;
        if (elapsed>= cfg.max_game_time){
            printf("Time limit => end\n");
            break;
        }
    }

    // Close the pipe to signal end of data
    close(graphics_pipe[1]);
    
    // terminate players
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