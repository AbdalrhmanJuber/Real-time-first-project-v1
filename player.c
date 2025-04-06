/**
 * Player process implementation for the Rope Pulling Game
 * Each player has energy and pulls based on location and energy level
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/signal.h>
#include "constant.h"
#include "pipe.h"
#include "config.h"

/* Global variables */
static int max_energy = 100;              // Maximum energy for the player
static int min_energy = 0;                // Minimum energy for the player
static PlayerData me;                     // Player state information
static int write_fd_effort = -1;          // Pipe to write effort to parent
static int read_fd_loc = -1;              // Pipe to read location from parent
static int decay_min = 1;                 // Min energy decay per second
static int decay_max = 2;                 // Max energy decay per second
static int recover_min = 1;               // Min recovery time when fallen
static int recover_max = 2;               // Max recovery time when fallen
static volatile sig_atomic_t pulling = 0; // Flag indicating if player is pulling
static int initial_energy;                // Initial energy value

/* Signal handlers prototypes */
void on_energy_req(int sig);
void on_set_loc(int sig);
void on_ready(int sig);
void on_terminate(int sig);
void on_stop(int sig);
void on_reset_energy(int sig);
void on_pull(int sig);

/**
 * Set up all signal handlers for the player process
 */
void setup_signal_handlers() {
    signal(SIG_PULL, on_pull);
    signal(SIG_STOP, on_stop);
    signal(SIG_ENERGY_REQ, on_energy_req);
    signal(SIG_SET_LOC, on_set_loc);
    signal(SIG_READY, on_ready);
    signal(SIG_TERMINATE, on_terminate);
    signal(SIG_RESET_ENERGY, on_reset_energy);
}

/**
 * Handle energy request from parent - report current energy level
 */
void on_energy_req(int sig) {
    EnergyReply er;
    er.player_id = me.id;
    er.team = me.team;
    er.energy = me.energy;  // Raw energy
    er.location = me.location;
    write_effort(write_fd_effort, &er, sizeof(er));
}

/**
 * Handle location assignment from parent
 */
void on_set_loc(int sig) {
    LocationMessage lm;
    int bytes = read_effort(read_fd_loc, &lm, sizeof(lm));
    if (bytes == sizeof(lm) && lm.player_id == me.id) {
        me.location = lm.location;
        printf("[Player %d, Team %d] Assigned location = %d\n", me.id, me.team, me.location);
    }
}

/**
 * Handle ready signal - game is about to begin
 */
void on_ready(int sig) {
    printf("[Player %d, Team %d] SIG_READY => location=%d, energy=%d\n",
           me.id, me.team, me.location, me.energy);
}

/**
 * Handle termination signal
 */
void on_terminate(int sig) {
    printf("[Player %d, Team %d] Terminating...\n", me.id, me.team);
    fflush(stdout);
    exit(0);
}

/**
 * Reset player energy to random value within configured range
 */
void on_reset_energy(int sig) {
    me.energy = (rand() % (max_energy - min_energy + 1)) + min_energy;
    me.is_fallen = 0;
    me.fall_time_left = 0;
}

/**
 * Simulate one second of gameplay - handle energy decay, falling, recovery
 */
void do_one_second_of_play() {
    if (!pulling)
        return;

    if (me.is_fallen) {
        me.fall_time_left--;
        if (me.fall_time_left <= 0) {
            me.is_fallen = 0;
            me.energy = (rand() % 50) + 50;
            
            // Cap energy at max_energy to prevent exceeding limit
            if (me.energy > max_energy)
                me.energy = max_energy;
                
            printf("[Player %d, Team %d] Recovered from fall. energy=%d\n",
                   me.id, me.team, me.energy);
        }
    } else {
        int random_decay = (rand() % (decay_max - decay_min + 1)) + decay_min;
        me.energy -= random_decay;

        // 10% chance of falling or if energy depletes
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
        .location = me.location,
        .weighted_effort = me.is_fallen ? 0 : me.energy * (1 + me.location)
    };
    write_effort(write_fd_effort, &msg, sizeof(msg));
}

/**
 * Handle pull signal - start pulling
 */
void on_pull(int sig) {
    printf("[Player %d, Team %d] SIG_PULL => Start pulling\n", me.id, me.team);
    pulling = 1;
    while (pulling) {
        sleep(1);
        do_one_second_of_play();
    }
}

/**
 * Handle stop signal - stop pulling
 */
void on_stop(int sig) {
    pulling = 0;
    printf("[Player %d, Team %d] Stopped pulling\n", me.id, me.team); // Optional debug
}

/**
 * Main function - initialize player and wait for signals
 */
int main(int argc, char *argv[]) {
    srand(time(NULL) ^ getpid());

    // Initialize player data
    me.id = atoi(argv[1]);
    me.team = atoi(argv[2]);
    me.decay_rate = atoi(argv[3]);
    me.energy = initial_energy = atoi(argv[4]);
    write_fd_effort = atoi(argv[5]);
    read_fd_loc = atoi(argv[6]);

    if (argc > 7)
        decay_min = atoi(argv[7]);
    if (argc > 8)
        decay_max = atoi(argv[8]);
    if (argc > 9)
        recover_min = atoi(argv[9]);
    if (argc > 10)
        recover_max = atoi(argv[10]);
    if (argc > 11)
        max_energy = atoi(argv[11]);
    if (argc > 12)
        min_energy = atoi(argv[12]);

    me.is_fallen = 0;
    me.location = 0;
    me.fall_time_left = 0;

    setup_signal_handlers();

    while (1) {
        pause();
    }

    return 0;
}