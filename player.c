#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "comm.h"
#include "pipe.h"
#include "config.h"

static PlayerData me;   // Info about this player
static int write_fd = -1;  // For sending EffortMessage to referee

// Called when we get SIG_READY
void on_ready(int sig) {
    // Naive approach: reorder location based on local energy,
    // but we don't know teammates' energies, so let's just randomize location.
    me.location = rand() % 4;
    printf("[Player %d, Team %d] SIG_READY => location set to %d (energy=%d)\n",
           me.id, me.team, me.location, me.energy);

    // If we had actual team comparison, we'd gather all players' energies, sort, etc.
}

// Called when we get SIG_PULL
void on_pull(int sig) {
    // Start pulling => might do nothing but let's just log
    printf("[Player %d, Team %d] SIG_PULL => Start pulling\n", me.id, me.team);
}

// Called when we get SIG_TERMINATE
void on_terminate(int sig) {
    printf("[Player %d, Team %d] Terminating...\n", me.id, me.team);
    exit(0);
}

// Each second, do pulling logic
void do_one_second_of_play()
{
    // If we are fallen, decrement fall_time_left
    if (me.is_fallen) {
        me.fall_time_left--;
        if (me.fall_time_left <= 0) {
            // Recovered
            me.is_fallen = 0;
            // Let's assume we rejoin with random energy between 1..(current + 10)
            // or you can set it to some fraction of old energy.
            int regained = (rand() % 10) + 1;
            me.energy += regained;
            printf("[Player %d, Team %d] Recovered from fall! New energy=%d\n", me.id, me.team, me.energy);
        }
    } else {
        // Decrement energy only if not fallen
        me.energy -= me.decay_rate;
        if (me.energy < 0) me.energy = 0;

        // Random chance to fall: e.g. 10%
        int r = rand() % 100;
        if (r < 10) { // 10% chance
            me.is_fallen = 1;
            me.energy = 0;
            // random recovery time in [fall_recover_min..fall_recover_max]
            int min_recover = 2; // Hardcode or store from config
            int max_recover = 5; // Hardcode or store from config
            // For a more robust approach, you'd pass fall recover range to child 
            // just like we pass energy/decay, but for brevity let's hardcode or guess
            me.fall_time_left = (rand() % (max_recover - min_recover + 1)) + min_recover;
            printf("[Player %d, Team %d] Fell! Will recover in %d secs\n", me.id, me.team, me.fall_time_left);
        }
    }

    // Weighted effort => energy * (1 + location)
    int multiplier = 1 + me.location;
    int weighted = me.is_fallen ? 0 : (me.energy * multiplier);

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
    me.id          = atoi(argv[1]);
    me.team        = atoi(argv[2]);
    me.decay_rate  = atoi(argv[3]);
    me.energy      = atoi(argv[4]);
    write_fd       = atoi(argv[5]);

    me.is_fallen   = 0;
    me.location    = me.id; 
    me.fall_time_left = 0;

    // Register signal handlers
    signal(SIG_READY, on_ready);
    signal(SIG_PULL, on_pull);
    signal(SIG_TERMINATE, on_terminate);

    // Infinite loop: each second simulate
    while (1) {
        sleep(1);
        do_one_second_of_play();
    }
    return 0;
}
