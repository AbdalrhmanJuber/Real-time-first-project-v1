#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>  // for gettimeofday

#include "constant.h"  // your comm.h
#include "config.h"
#include "pipe.h"

int pipes[MAX_PLAYERS][2];
pid_t players[MAX_PLAYERS];

GameConfig cfg;               // from config.txt
int team_scores[2] = {0, 0};  // overall scores across rounds
int consecutive_wins[2] = {0, 0};

void spawn_players() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        int team_id   = (i < TEAM_SIZE) ? TEAM1 : TEAM2;
        int player_id = (team_id == TEAM1) ? i : i - TEAM_SIZE;

        int init_energy = rand() % (cfg.energy_max - cfg.energy_min + 1) + cfg.energy_min;
        int decay       = rand() % (cfg.decay_max - cfg.decay_min + 1) + cfg.decay_min;

        // Prepare arguments for execl
        char buf_id[16], buf_team[16], buf_decay[16], buf_energy[16], buf_writefd[16];
        sprintf(buf_id, "%d", player_id);
        sprintf(buf_team, "%d", team_id);
        sprintf(buf_decay, "%d", decay);
        sprintf(buf_energy, "%d", init_energy);
        sprintf(buf_writefd, "%d", pipes[i][1]);

        pid_t pid = fork();
        if (pid == 0) {
            // child
            close(pipes[i][0]);  // Child doesn't read from the pipe
            execl("./player", "./player",
                  buf_id, buf_team, buf_decay, buf_energy, buf_writefd,
                  (char *)NULL);
            perror("execl failed");
            exit(1);
        } else {
            // parent
            players[i] = pid;
            close(pipes[i][1]); // Parent doesn't write to this pipe
        }
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    // Load config
    if (load_config(argv[1], &cfg) != 0) {
        return 1;
    }

    // Create pipes
    if (init_pipes(pipes, MAX_PLAYERS) < 0) {
        return 1;
    }

    // Spawn children
    spawn_players();

    // Track entire simulation time
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);

    int total_rounds = 0;
    int last_winner = -1;  // used to track consecutive wins

    while (1) {
        total_rounds++;
        printf("\n===== START ROUND %d =====\n", total_rounds);

        // 1) Signal players to align
        for (int i = 0; i < MAX_PLAYERS; i++) {
            kill(players[i], SIG_READY);
        }
        sleep(1); // let them reorder

        // 2) Signal players to start pulling
        for (int i = 0; i < MAX_PLAYERS; i++) {
            kill(players[i], SIG_PULL);
        }

        // 3) Round loop => continues until threshold is crossed or total time is up
        int round_winner = -1;
        while (round_winner < 0) {
            // Sleep 1 second => gather efforts
            sleep(1);

            int sum_team1 = 0;
            int sum_team2 = 0;
            EffortMessage em;

            // read from each player's pipe
            for (int i = 0; i < MAX_PLAYERS; i++) {
                int n = read_effort(pipes[i][0], &em, sizeof(em));
                if (n > 0) {
                    if (em.team == TEAM1) sum_team1 += em.weighted_effort;
                    else                 sum_team2 += em.weighted_effort;
                }
            }

            printf("[Round %d] T1=%d, T2=%d\n", total_rounds, sum_team1, sum_team2);

            // Check threshold => if a team crosses threshold => end this round
            if (sum_team1 >= cfg.win_threshold) {
                round_winner = TEAM1;
                break;
            }
            if (sum_team2 >= cfg.win_threshold) {
                round_winner = TEAM2;
                break;
            }

            // Also check if entire simulation time is exhausted
            gettimeofday(&current_time, NULL);
            long elapsed_sec = current_time.tv_sec - start_time.tv_sec;
            if (elapsed_sec >= cfg.max_game_time) {
                printf("Time limit reached => end game.\n");
                // If we must end the *entire* simulation, we pick the round winner 
                // by whichever team is ahead in sum
                if (sum_team1 > sum_team2)      round_winner = TEAM1;
                else if (sum_team2 > sum_team1) round_winner = TEAM2;
                else                           round_winner = (rand() % 2);
                break;
            }
        } // End while( round_winner < 0 )

        // If we got here, we have a valid round_winner
        printf("=== Winner of round %d is Team %d ===\n", total_rounds, round_winner);
        team_scores[round_winner]++;

        // Check consecutive wins
        if (round_winner == last_winner) {
            consecutive_wins[round_winner]++;
        } else {
            // reset counts
            consecutive_wins[TEAM1] = 0;
            consecutive_wins[TEAM2] = 0;
            consecutive_wins[round_winner] = 1;
        }
        last_winner = round_winner;

        // Now check if we end entire simulation
        // 1) if user-defined time is done => we already ended
        // 2) if a team's score >= max_score
        if (team_scores[TEAM1] >= cfg.max_score || team_scores[TEAM2] >= cfg.max_score) {
            printf("Team %d reached max_score => end simulation.\n", round_winner);
            break;
        }

        // 3) if consecutive wins reached
        if (consecutive_wins[round_winner] >= cfg.consecutive_wins) {
            printf("Team %d got %d consecutive wins => end simulation.\n",
                   round_winner, consecutive_wins[round_winner]);
            break;
        }

        // Also re-check total time again (just in case)
        gettimeofday(&current_time, NULL);
        long elapsed_sec = current_time.tv_sec - start_time.tv_sec;
        if (elapsed_sec >= cfg.max_game_time) {
            printf("Time limit reached => end simulation.\n");
            break;
        }
    }

    // Terminate all child processes
    for (int i = 0; i < MAX_PLAYERS; i++) {
        kill(players[i], SIG_TERMINATE);
    }
    for (int i = 0; i < MAX_PLAYERS; i++) {
        wait(NULL);
    }

    // Final results
    printf("\n==== Final Score ====\n");
    printf("Team1=%d, Team2=%d\n", team_scores[TEAM1], team_scores[TEAM2]);
    printf("Bye!\n");

    return 0;
}
