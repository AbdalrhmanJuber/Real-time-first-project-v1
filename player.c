#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "comm.h"
#include "pipe.h"     // if using pipes
// #include "fifo.h"   // if you prefer named FIFOs
#include "config.h"

static PlayerData me;   // Info about this player
static int write_fd = -1;  // For sending EffortMessage to referee

// Called when we get SIG_READY
void on_ready(int sig) {
    // TODO: handle alignment logic
    // e.g. reorder location based on who has highest energy
    // For now, just print
    printf("[Player %d, Team %d] Received SIG_READY\n", me.id, me.team);
}

// Called when we get SIG_PULL
void on_pull(int sig) {
    // TODO: start pulling. Possibly start a timer or set a flag
    printf("[Player %d, Team %d] Received SIG_PULL\n", me.id, me.team);
}

// Example: simulate each second
void do_one_second_of_play() {
    // Decrement energy
    if (!me.is_fallen) {
        me.energy -= me.decay_rate;
        if (me.energy < 0) me.energy = 0;
    }

    // Possibly check if we "fell"
    // TODO: random chance
    // if fell, me.is_fallen = 1 => me.energy = 0 => rejoin after random time

    // Weighted effort = energy * (1 + location),
    // e.g. location=0 => multiplier=1, location=1 => mult=2, etc.
    int multiplier = 1 + me.location;
    int weighted = me.energy * multiplier;

    // Send EffortMessage to parent
    EffortMessage msg;
    msg.player_id = me.id;
    msg.team = me.team;
    msg.weighted_effort = weighted;

    write_effort(write_fd, &msg, sizeof(msg));
}

int main(int argc, char *argv[])
{
    srand(time(NULL) ^ getpid());

    // Expect arguments: [0]=binary, [1]=id, [2]=team, [3]=decay, [4]=init_energy, [5]=write_fd
    me.id       = atoi(argv[1]);
    me.team     = atoi(argv[2]);
    me.decay_rate = atoi(argv[3]);
    me.energy   = atoi(argv[4]);
    write_fd    = atoi(argv[5]);

    me.is_fallen = 0;
    me.location  = me.id; // naive: location = id

    // Register signal handlers
    signal(SIG_READY, on_ready);
    signal(SIG_PULL, on_pull);

    // Each second, do some logic
    while (1) {
        // Could wait for a small period
        sleep(1);
        do_one_second_of_play();
    }

    return 0;
}
