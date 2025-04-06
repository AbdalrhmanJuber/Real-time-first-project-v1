/**
 * Rope Pulling Game Visualization
 * 
 * This file implements the OpenGL visualization of the rope pulling game:
 * - Displays team information and player energy values 
 * - Shows animated player characters with energy bars
 * - Visualizes the rope position based on team efforts
 * - Displays round and game statistics
 */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

#include "constant.h"

// Team data
static float team1[TEAM_SIZE] = {80, 70, 60, 50};  // Blue team
static float team2[TEAM_SIZE] = {60, 55, 40, 35};  // Red team

// Game state
static int round_number = 0;
static int sum_t1 = 0;
static int sum_t2 = 0;
static int round_winner = 0;  // 0=none, 1=Team1, 2=Team2

// Rope movement
static float currentOffset = 0.0f;
static float targetOffset = 0.0f;
static char status_message[50] = "";

// Pipe for reading from parent process
static int pipe_fd = -1;

// Game end state
static int game_over = 0;
static int game_winner = 0;
static int final_score_team1 = 0;
static int final_score_team2 = 0;

// Animation variables
static float cloudX = -1.0f;  // Cloud position 
static float bobFrame = 0.0f; // Player bobbing animation

/**
 * Draw gradient sky-to-grass background
 */
void drawGradientBackground() {
    glBegin(GL_QUADS);
    glColor3f(0.6f, 0.85f, 1.0f); // Sky color
    glVertex2f(-1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glColor3f(0.3f, 0.7f, 0.3f);  // Grass color
    glVertex2f(1.0f, -1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();
}

/**
 * Draw the sun
 */
void drawSun() {
    glColor3f(1.0f, 1.0f, 0.0f);
    float cx = -0.85f, cy = 0.85f, r = 0.08f;
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; ++i) {
        float theta = i * 3.14159f / 180.0f;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

/**
 * Draw a cloud at given position
 */
void drawCloud(float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f);
    // Draw 3 overlapping circles
    for (int i = -1; i <= 1; ++i) {
        glBegin(GL_POLYGON);
        for (int j = 0; j < 360; ++j) {
            float theta = j * 3.14159f / 180.0f;
            glVertex2f(x + i * 0.05f + 0.04f * cosf(theta),
                    y + 0.03f * sinf(theta));
        }
        glEnd();
    }
}

/**
 * Comparison function for descending sort of energy values
 */
int compareDesc(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa < fb) - (fa > fb);
}

/**
 * Draw energy value text
 */
void drawEnergyText(float x, float y, float energy) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.0f", energy);
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x - 0.01f, y + 0.03f);
    for (int i = 0; buffer[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
    }
}

/**
 * Draw player ID text
 */
void drawPlayerIdText(float x, float y, int playerId) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "ID:%d", playerId);
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x - 0.01f, y - 0.16f);  // Position below energy bar
    for (int i = 0; buffer[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, buffer[i]);
    }
}

/**
 * Draw energy bar under player
 */
void drawEnergyBar(float x, float y, float energy) {
    float maxEnergy = 100.0f; // Maximum energy value
    float eRatio = energy / maxEnergy;
    if (eRatio < 0.0f) eRatio = 0.0f;
    if (eRatio > 1.0f) eRatio = 1.0f;

    float barWidth = 0.04f * eRatio;
    // Bar background (gray)
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.02f, y - 0.12f);
        glVertex2f(x + 0.02f, y - 0.12f);
        glVertex2f(x + 0.02f, y - 0.13f);
        glVertex2f(x - 0.02f, y - 0.13f);
    glEnd();
    // Energy portion (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.02f, y - 0.12f);
        glVertex2f(x - 0.02f + barWidth, y - 0.12f);
        glVertex2f(x - 0.02f + barWidth, y - 0.13f);
        glVertex2f(x - 0.02f, y - 0.13f);
    glEnd();
}

/**
 * Draw player character with bobbing animation if round in progress
 */
void drawPlayer(float baseX, float baseY, int flipped, float r, float g, float b, 
                float energy, int index, int playerId) {
    // Add bobbing if round is still active
    float offsetY = 0.0f;
    if (round_winner == 0) {
        // Use bobFrame + index to prevent sync between players
        offsetY = 0.005f * sinf(bobFrame + index * 1.0f);
    }

    float x = baseX;
    float y = baseY + offsetY;

    // Gray out player if energy is depleted
    if (energy <= 0.1f)
        glColor3f(0.6f, 0.6f, 0.6f);
    else
        glColor3f(1.0f, 0.8f, 0.6f);

    // Draw head (circle)
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float theta = i * 3.14159f / 180.0f;
        glVertex2f(x + 0.02f * cosf(theta), y + 0.02f * sinf(theta));
    }
    glEnd();

    // Draw eyes
    glColor3f(0, 0, 0);
    glBegin(GL_POINTS);
        glVertex2f(x - 0.007f, y + 0.01f);
        glVertex2f(x + 0.007f, y + 0.01f);
    glEnd();

    // Draw body
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.02f, y - 0.02f);
        glVertex2f(x + 0.02f, y - 0.02f);
        glVertex2f(x + 0.02f, y - 0.08f);
        glVertex2f(x - 0.02f, y - 0.08f);
    glEnd();

    // Draw arms
    glColor3f(0, 0, 0);
    glBegin(GL_LINES);
        glVertex2f(x - 0.02f, y - 0.04f);
        glVertex2f(x - 0.06f * flipped, y - 0.04f);

        glVertex2f(x + 0.02f, y - 0.04f);
        glVertex2f(x + 0.06f * flipped, y - 0.04f);
    glEnd();

    // Draw legs
    glBegin(GL_LINES);
        glVertex2f(x - 0.01f, y - 0.08f);
        glVertex2f(x - 0.01f, y - 0.11f);

        glVertex2f(x + 0.01f, y - 0.08f);
        glVertex2f(x + 0.01f, y - 0.11f);
    glEnd();

    // Draw energy bar and labels
    drawEnergyBar(x, y, energy);
    drawEnergyText(x, y, energy);
    drawPlayerIdText(x, y, playerId);
}

/**
 * Update cloud position
 */
void updateCloud(int value) {
    cloudX += 0.002f;
    if (cloudX > 1.2f) 
        cloudX = -1.2f;
    glutTimerFunc(30, updateCloud, 0);
}

/**
 * Increment bobFrame for player animation
 */
void incrementBobFrame() {
    bobFrame += 0.05f;
}

/**
 * Main update function - reads pipe data and updates game state
 */
void update(int value) {
    // Update bobbing animation
    incrementBobFrame();

    // Setup non-blocking read
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(pipe_fd, &readfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while (select(pipe_fd + 1, &readfds, NULL, NULL, &tv) > 0) {
        GraphicsMessage msg;
        int bytes = read(pipe_fd, &msg, sizeof(msg));
        if (bytes == sizeof(msg)) {
            // Check if this is a game-end message
            if (msg.roundNumber == -1) {
                game_over = 1;
                game_winner = msg.roundWinner;
                final_score_team1 = msg.team1EffortSum;
                final_score_team2 = msg.team2EffortSum;
                snprintf(status_message, sizeof(status_message), "Game Over!");
            } else {
                // Regular round update
                round_number = msg.roundNumber;
                for (int i = 0; i < TEAM_SIZE; i++) {
                    team1[i] = msg.team1Energies[i];
                    team2[i] = msg.team2Energies[i];
                }
                sum_t1 = msg.team1EffortSum;
                sum_t2 = msg.team2EffortSum;
                round_winner = msg.roundWinner;

                // Calculate rope position based on team efforts
                float total = sum_t1 + sum_t2;
                if (total > 0.1f) {
                    targetOffset = (sum_t2 - sum_t1) / total * 0.3f;
                } else {
                    targetOffset = 0.0f;
                }

                // Update status message
                if (round_winner == 1)
                    snprintf(status_message, sizeof(status_message),
                            "Team 1 Wins Round %d!", round_number);
                else if (round_winner == 2)
                    snprintf(status_message, sizeof(status_message),
                            "Team 2 Wins Round %d!", round_number);
                else
                    snprintf(status_message, sizeof(status_message),
                            "Round %d in progress...", round_number);
            }
        } else {
            // EOF or error
            break;
        }
        FD_ZERO(&readfds);
        FD_SET(pipe_fd, &readfds);
    }

    // Smoothly move rope
    float speed = 0.002f;
    if (fabsf(currentOffset - targetOffset) > speed) {
        currentOffset += (currentOffset < targetOffset) ? speed : -speed;
    } else {
        currentOffset = targetOffset;
    }

    // Trigger redisplay
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

/**
 * Main display function
 */
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw background elements
    drawGradientBackground();
    drawSun();
    drawCloud(cloudX, 0.85f);

    // Draw rope
    glColor3f(0.4f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(-0.9f + currentOffset, 0.0f - 0.01f);
        glVertex2f(0.9f + currentOffset, 0.0f - 0.01f);
        glVertex2f(0.9f + currentOffset, 0.0f + 0.01f);
        glVertex2f(-0.9f + currentOffset, 0.0f + 0.01f);
    glEnd();

    // Copy and sort player energy values
    float sorted1[TEAM_SIZE], sorted2[TEAM_SIZE];
    memcpy(sorted1, team1, sizeof(team1));
    memcpy(sorted2, team2, sizeof(team2));
    qsort(sorted1, TEAM_SIZE, sizeof(float), compareDesc);
    qsort(sorted2, TEAM_SIZE, sizeof(float), compareDesc);

    // Draw blue team
    for (int i = 0; i < TEAM_SIZE; i++) {
        float baseX = -0.7f + i * 0.15f + currentOffset;
        float baseY = 0.05f;
        // flipped=1 => arms pointing right
        drawPlayer(baseX, baseY, 1, 0.0f, 0.0f, 1.0f, team1[i], i, i);
    }

    // Draw red team
    for (int i = 0; i < TEAM_SIZE; i++) {
        float baseX = 0.7f - i * 0.15f + currentOffset;
        float baseY = 0.05f;
        // flipped=-1 => arms pointing left
        drawPlayer(baseX, baseY, -1, 1.0f, 0.0f, 0.0f, team2[i], i, i);
    }

    // Draw round number
    char roundStr[50];
    snprintf(roundStr, sizeof(roundStr), "Round %d", round_number);
    glColor3f(0, 0, 0);
    glRasterPos2f(-0.9f, 0.9f);
    for(int i = 0; roundStr[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, roundStr[i]);
    }

    // Draw team efforts
    char effortStr[100];
    snprintf(effortStr, sizeof(effortStr), "Team 1: %d | Team 2: %d", sum_t1, sum_t2);
    glRasterPos2f(-0.25f, 0.8f);
    for(int i = 0; effortStr[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, effortStr[i]);
    }

    // Display game over screen if game has ended
    if (game_over) {
        // Semi-transparent overlay
        glColor4f(0.9f, 0.9f, 0.9f, 0.8f);
        glBegin(GL_QUADS);
            glVertex2f(-0.6f, 0.4f);
            glVertex2f(0.6f, 0.4f);
            glVertex2f(0.6f, -0.4f);
            glVertex2f(-0.6f, -0.4f);
        glEnd();

        glColor3f(0, 0, 0);
        // Game over text
        glRasterPos2f(-0.5f, 0.3f);
        const char *overText = "GAME OVER";
        for(int i = 0; overText[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, overText[i]);
        }

        // Final score
        char finalScore[100];
        snprintf(finalScore, sizeof(finalScore), "Final Score: T1=%d, T2=%d",
                final_score_team1, final_score_team2);
        glRasterPos2f(-0.5f, 0.2f);
        for(int i = 0; finalScore[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, finalScore[i]);
        }

        // Show winner
        if (game_winner == 1) {
            glRasterPos2f(-0.5f, 0.0f);
            const char* wmsg = "TEAM 1 WINS THE GAME!";
            for(int i = 0; wmsg[i] != '\0'; i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, wmsg[i]);
            }
        } else if (game_winner == 2) {
            glRasterPos2f(-0.5f, 0.0f);
            const char* wmsg = "TEAM 2 WINS THE GAME!";
            for(int i = 0; wmsg[i] != '\0'; i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, wmsg[i]);
            }
        } else {
            glRasterPos2f(-0.5f, 0.0f);
            const char* tieMsg = "THE GAME IS A TIE!";
            for(int i = 0; tieMsg[i] != '\0'; i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, tieMsg[i]);
            }
        }
    } else {
        // Show round winner if determined
        if (round_winner == 1) {
            glRasterPos2f(0.5f, 0.9f);
            const char* msgW = "Team 1 Wins!";
            for(int i = 0; msgW[i] != '\0'; i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, msgW[i]);
            }
        } else if (round_winner == 2) {
            glRasterPos2f(0.5f, 0.9f);
            const char* msgW2 = "Team 2 Wins!";
            for(int i = 0; msgW2[i] != '\0'; i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, msgW2[i]);
            }
        }

        // Display status message
        if (status_message[0] != '\0') {
            glRasterPos2f(-0.1f, 0.7f);
            for(int i = 0; status_message[i] != '\0'; i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, status_message[i]);
            }
        }
    }

    glutSwapBuffers();
}

/**
 * Handle keyboard input
 */
void keyboard(unsigned char key, int x, int y) {
    if (key == 'q' || key == 'Q' || key == 27) { // 27=ESC
        exit(0);
    }
}

/**
 * Setup timer callbacks
 */
void mainUpdateSetup() {
    glutTimerFunc(16, update, 0);      // Rope movement and animations
    glutTimerFunc(30, updateCloud, 0); // Cloud movement
}

/**
 * Main function
 */
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Rope Pulling Game Visualization");

    // Get pipe fd from command line
    if (argc > 1) {
        pipe_fd = atoi(argv[1]);
        // Set non-blocking mode
        int flags = fcntl(pipe_fd, F_GETFL, 0);
        fcntl(pipe_fd, F_SETFL, flags | O_NONBLOCK);
    } else {
        fprintf(stderr, "No pipe fd provided\n");
        return 1;
    }

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    mainUpdateSetup();

    glutMainLoop();
    return 0;
}