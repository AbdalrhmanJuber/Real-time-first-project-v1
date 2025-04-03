#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "constant.h"
#include "pipe.h"
#include "config.h"

static PlayerData me;
static int write_fd_effort = -1;  // Player->Parent for EffortMessage or EnergyReply
static int read_fd_loc    = -1;  // Parent->Player location assignment
static int decay_min = 1;
static int decay_max = 2;
static int recover_min = 1;
static int recover_max = 2;
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

void do_one_second_of_play() {
    if (me.is_fallen) {
        me.fall_time_left--;
        if (me.fall_time_left <= 0) {
            me.is_fallen = 0;
            int regained = (rand() % 50) + 50;
            me.energy += regained;
            printf("[Player %d, Team %d] Recovered from fall. energy=%d\n",
                   me.id, me.team, me.energy);
        }
    } else {
        // Dynamic decay based on config ranges
        int random_decay = (rand() % (decay_max - decay_min + 1)) + decay_min;
        me.energy -= random_decay;
        int r = rand() % 100;
        
        if (me.energy <= 0 || r < 10 ) {
            // Player falls when energy reaches 0
            me.energy = 0;
            me.is_fallen = 1;
            
            // Dynamic recovery time based on config ranges
            int random_recover = (rand() % (recover_max - recover_min + 1)) + recover_min;
            me.fall_time_left = random_recover;
            
            printf("[Player %d, Team %d] Fell. Will recover in %d sec\n",
                   me.id, me.team, me.fall_time_left);
        }
    }

    // Weighted effort
    int mult = 1 + me.location;
    int weighted = (me.is_fallen ? 0 : me.energy * mult);

    EffortMessage msg;
    msg.player_id      = me.id;
    msg.team           = me.team;
    msg.weighted_effort= weighted;
    write_effort(write_fd_effort, &msg, sizeof(msg));
}

// Called when we get SIG_PULL
void on_pull(int sig) {
    printf("[Player %d, Team %d] SIG_PULL => Start pulling\n", me.id, me.team);
    // main loop
    while (1) {
        sleep(1);
        do_one_second_of_play();
    }
}


int main(int argc, char *argv[])
{
    srand(time(NULL) ^ getpid());

    me.id         = atoi(argv[1]);
    me.team       = atoi(argv[2]);
    me.decay_rate = atoi(argv[3]);
    me.energy     = atoi(argv[4]);
    write_fd_effort= atoi(argv[5]);
    read_fd_loc   = atoi(argv[6]);

    if (argc > 7) decay_min = atoi(argv[7]);
    if (argc > 8) decay_max = atoi(argv[8]);
    if (argc > 9) recover_min = atoi(argv[9]); 
    if (argc > 10) recover_max = atoi(argv[10]);

    me.is_fallen   = 0;
    me.location    = 0;
    me.fall_time_left = 0;

    // Register signals
    signal(SIG_ENERGY_REQ, on_energy_req);
    signal(SIG_SET_LOC,    on_set_loc);
    signal(SIG_READY,      on_ready);
    signal(SIG_PULL,       on_pull);
    signal(SIG_TERMINATE,  on_terminate);


    while(1){
        pause();
    }


    return 0;
}
