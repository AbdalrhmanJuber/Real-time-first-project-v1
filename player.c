#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/signal.h>

#include "constant.h"
#include "pipe.h"
#include "config.h"

static int max_energy = 100; // max energy for the player
static int min_energy = 0; // min energy for the player
static PlayerData me;
static int write_fd_effort = -1;
static int read_fd_loc = -1;
static int decay_min = 1;
static int decay_max = 2;
static int recover_min = 1;
static int recover_max = 2;
static volatile sig_atomic_t pulling = 0;
static int initial_energy;

// Signal handlers prototypes
void on_energy_req(int sig);
void on_set_loc(int sig);
void on_ready(int sig);
void on_terminate(int sig);
void on_stop(int sig);
void on_reset_energy(int sig);
void on_pull(int sig);

void setup_signal_handlers() {
    struct sigaction sa;
    
    // SIG_PULL handler
    sa.sa_handler = on_pull;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIG_PULL, &sa, NULL);

    // SIG_STOP handler
    sa.sa_handler = on_stop;
    sigaction(SIG_STOP, &sa, NULL);

    // Other signals
    signal(SIG_ENERGY_REQ, on_energy_req);
    signal(SIG_SET_LOC, on_set_loc);
    signal(SIG_READY, on_ready);
    signal(SIG_TERMINATE, on_terminate);
    signal(SIG_RESET_ENERGY, on_reset_energy);
}



// Called when the parent requests energy
void on_energy_req(int sig) {
    EnergyReply er;
    er.player_id = me.id;
    er.team      = me.team;
    er.energy    = me.energy; // raw energy
    write_effort(write_fd_effort, &er, sizeof(er));
}

// Called when we get SIG_SET_LOC
void on_set_loc(int sig) {
    // Read location from parent's location pipe
    LocationMessage lm;
    int bytes = read_effort(read_fd_loc, &lm, sizeof(lm));
    if (bytes == sizeof(lm) && lm.player_id == me.id) {
        me.location = lm.location;
        printf("[Player %d, Team %d] Assigned location = %d\n", me.id, me.team, me.location);
    }
}

// Called when we get SIG_READY
void on_ready(int sig) {
    // Now that location is assigned, just log
    printf("[Player %d, Team %d] SIG_READY => location=%d, energy=%d\n",
           me.id, me.team, me.location, me.energy);
}


// Called when we get SIG_TERMINATE
void on_terminate(int sig) {
    printf("[Player %d, Team %d] Terminating...\n", me.id, me.team);
    fflush(stdout);
    exit(0);
}


void on_reset_energy(int sig) {
    me.energy =  rand() % (max_energy - min_energy + 1) + min_energy;
    me.is_fallen = 0;
    me.fall_time_left = 0;
}

void do_one_second_of_play() {
    if (!pulling) return;

    if (me.is_fallen) {
        me.fall_time_left--;
        if (me.fall_time_left <= 0) {
            me.is_fallen = 0;
            me.energy = (rand() % 50) + 50;
            printf("[Player %d, Team %d] Recovered from fall. energy=%d\n",
                   me.id, me.team, me.energy);
        }
    } else {
        int random_decay = (rand() % (decay_max - decay_min + 1)) + decay_min;
        me.energy -= random_decay;
        
        if (me.energy <= 0 || (rand() % 100) < 10) {
            me.energy = me.energy < 0 ? 0 : me.energy;
            me.is_fallen = 1;
            me.fall_time_left = (rand() % (recover_max - recover_min + 1)) + recover_min;
            printf("[Player %d, Team %d] Fell. Will recover in %d sec\n",
                   me.id, me.team, me.fall_time_left);
        }
    }

    EffortMessage msg = {
        .player_id = me.id,
        .team = me.team,
        .weighted_effort = me.is_fallen ? 0 : me.energy * (1 + me.location)
    };
    write_effort(write_fd_effort, &msg, sizeof(msg));
}


// Signal handlers
void on_pull(int sig) {
    printf("[Player %d, Team %d] SIG_PULL => Start pulling\n", me.id, me.team);
    pulling = 1;
    while (pulling) {
        sleep(1);
        do_one_second_of_play();
    }
}


// Add SIG_STOP handler
void on_stop(int sig) {
    pulling = 0;
    printf("[Player %d, Team %d] Stopped pulling\n", me.id, me.team); // Optional debug
}



int main(int argc, char *argv[]) {
    srand(time(NULL) ^ getpid());
    
    

    // Initialize player data
    me.id = atoi(argv[1]);
    me.team = atoi(argv[2]);
    me.decay_rate = atoi(argv[3]);
    me.energy = initial_energy = atoi(argv[4]);
    write_fd_effort = atoi(argv[5]);
    read_fd_loc = atoi(argv[6]);

    if (argc > 7) decay_min = atoi(argv[7]);
    if (argc > 8) decay_max = atoi(argv[8]);
    if (argc > 9) recover_min = atoi(argv[9]);
    if (argc > 10) recover_max = atoi(argv[10]);
    if (argc > 11) max_energy = atoi(argv[11]);
    if (argc > 12) min_energy = atoi(argv[12]);

    me.is_fallen = 0;
    me.location = 0;
    me.fall_time_left = 0;


    setup_signal_handlers();

    while(1) {
        pause();
    }

    return 0;
}