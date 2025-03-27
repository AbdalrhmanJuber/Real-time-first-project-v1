#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "constant.h"
#include "pipe.h"
#include "config.h"

static PlayerData me;
static int write_fd_effort = -1;  // Child->Parent for EffortMessage or EnergyReply
static int read_fd_loc    = -1;  // Parent->Child location assignment

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
        printf("[Child %d, Team %d] Assigned location = %d\n", me.id, me.team, me.location);
    }
}

// Called when we get SIG_READY
void on_ready(int sig) {
    // Now that location is assigned, just log
    printf("[Player %d, Team %d] SIG_READY => location=%d, energy=%d\n",
           me.id, me.team, me.location, me.energy);
}

// Called when we get SIG_PULL
void on_pull(int sig) {
    printf("[Player %d, Team %d] SIG_PULL => Start pulling\n", me.id, me.team);
}

// Called when we get SIG_TERMINATE
void on_terminate(int sig) {
    printf("[Player %d, Team %d] Terminating...\n", me.id, me.team);
    fflush(stdout);
    exit(0);
}

// Each second, do pulling logic
void do_one_second_of_play() {
    // same falling/energy logic
    if (me.is_fallen) {
        me.fall_time_left--;
        if (me.fall_time_left <= 0) {
            me.is_fallen = 0;
            int regained = (rand() % 10) + 1;
            me.energy += regained;
            printf("[Player %d, Team %d] Recovered from fall. energy=%d\n",
                   me.id, me.team, me.energy);
        }
    } else {
        me.energy -= me.decay_rate;
        if (me.energy < 0) me.energy = 0;
        int r = rand() % 100;
        if (r < 5) {
            me.is_fallen = 1;
            me.energy = 0;
            int min_recover = 2;
            int max_recover = 5;
            me.fall_time_left = (rand() % (max_recover - min_recover + 1)) + min_recover;
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

int main(int argc, char *argv[])
{
    srand(time(NULL) ^ getpid());

    // Expecting:
    // argv[1]=player_id, argv[2]=team, argv[3]=decay, argv[4]=energy
    // argv[5]=fd for EFFORT writing
    // argv[6]=fd for LOCATION reading
    me.id         = atoi(argv[1]);
    me.team       = atoi(argv[2]);
    me.decay_rate = atoi(argv[3]);
    me.energy     = atoi(argv[4]);
    write_fd_effort= atoi(argv[5]);
    read_fd_loc   = atoi(argv[6]);

    me.is_fallen   = 0;
    me.location    = 0;
    me.fall_time_left = 0;

    // Register signals
    signal(SIG_ENERGY_REQ, on_energy_req);
    signal(SIG_SET_LOC,    on_set_loc);
    signal(SIG_READY,      on_ready);
    signal(SIG_PULL,       on_pull);
    signal(SIG_TERMINATE,  on_terminate);

    // main loop
    while (1) {
        sleep(1);
        do_one_second_of_play();
    }
    return 0;
}
