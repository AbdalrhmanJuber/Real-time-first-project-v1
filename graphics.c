#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

#include "constant.h"

// Store each team's energy values
static float team1[TEAM_SIZE] = {80, 70, 60, 50};
static float team2[TEAM_SIZE] = {60, 55, 40, 35};

// Game state tracking
static int round_number = 0;
static int sum_t1 = 0;
static int sum_t2 = 0;
static int round_winner = 0; // 0=none, 1=team1, 2=team2

// Visualization variables
static float currentOffset = 0.0;
static float targetOffset = 0.0;
static char status_message[50] = "";

// Pipe from parent
static int pipe_fd = -1;

int compareDesc(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa < fb) - (fa > fb);
}

void drawEnergy(float x, float y, float energy) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.0f", energy);
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2f(x - 0.01, y + 0.03);
    for (int i = 0; buffer[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
    }
}

void drawPlayer(float x, float y, int flipped, float r, float g, float b, float energy) {
    if (energy <= 0.1)
        glColor3f(0.6, 0.6, 0.6);
    else
        glColor3f(1.0, 0.8, 0.6);

    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float theta = i * 3.14159 / 180;
        glVertex2f(x + 0.02 * cos(theta), y + 0.02 * sin(theta));
    } 
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.02, y - 0.02);
        glVertex2f(x + 0.02, y - 0.02);
        glVertex2f(x + 0.02, y - 0.08);
        glVertex2f(x - 0.02, y - 0.08);
    glEnd();

    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINES);
        glVertex2f(x - 0.02, y - 0.04);
        glVertex2f(x - 0.06 * flipped, y - 0.04);
        glVertex2f(x + 0.02, y - 0.04);
        glVertex2f(x + 0.06 * flipped, y - 0.04);
    glEnd();

    drawEnergy(x, y, energy);
}

void drawText(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw rope
    glColor3f(0.4, 0.2, 0.0);
    glBegin(GL_QUADS);
        glVertex2f(-0.9 + currentOffset, 0.0 - 0.01);
        glVertex2f(0.9 + currentOffset, 0.0 - 0.01);
        glVertex2f(0.9 + currentOffset, 0.0 + 0.01);
        glVertex2f(-0.9 + currentOffset, 0.0 + 0.01);
    glEnd();

    // Sort energies for display
    float sorted1[TEAM_SIZE], sorted2[TEAM_SIZE];
    memcpy(sorted1, team1, sizeof(team1));
    memcpy(sorted2, team2, sizeof(team2));
    qsort(sorted1, TEAM_SIZE, sizeof(float), compareDesc);
    qsort(sorted2, TEAM_SIZE, sizeof(float), compareDesc);

    // Draw team 1 players (blue)
    for (int i = 0; i < TEAM_SIZE; i++) {
        float x = -0.7 + i * 0.15 + currentOffset;
        drawPlayer(x, 0.05, 1, 0.0, 0.0, 1.0, team1[i]);
    }

    // Draw team 2 players (red)
    for (int i = 0; i < TEAM_SIZE; i++) {
        float x = 0.7 - i * 0.15 + currentOffset;
        drawPlayer(x, 0.05, -1, 1.0, 0.0, 0.0, team2[i]);
    }

    // Draw round info
    char roundStr[50];
    snprintf(roundStr, sizeof(roundStr), "Round %d", round_number);
    drawText(-0.9, 0.9, roundStr);

    // Draw effort sums
    char effortStr[100];
    snprintf(effortStr, sizeof(effortStr), "Team 1: %d | Team 2: %d", sum_t1, sum_t2);
    drawText(-0.25, 0.8, effortStr);

    // Draw round winner if there is one
    if (round_winner == 1) {
        drawText(0.5, 0.9, "Team 1 Wins!");
    } else if (round_winner == 2) {
        drawText(0.5, 0.9, "Team 2 Wins!");
    }
    
    // Draw status message if any
    if (status_message[0] != '\0') {
        drawText(-0.1, 0.7, status_message);
    }

    glutSwapBuffers();
}

void update(int value) {
    // Check for new data from pipe
    GraphicsMessage msg;
    
    // Non-blocking read from pipe
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    FD_SET(pipe_fd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    while (select(pipe_fd + 1, &readfds, NULL, NULL, &tv) > 0) {
        if (read(pipe_fd, &msg, sizeof(msg)) == sizeof(msg)) {
            // Update our local state with received data
            round_number = msg.roundNumber;
            
            for (int i = 0; i < TEAM_SIZE; i++) {
                team1[i] = msg.team1Energies[i];
                team2[i] = msg.team2Energies[i];
            }
            
            sum_t1 = msg.team1EffortSum;
            sum_t2 = msg.team2EffortSum;
            round_winner = msg.roundWinner;
            
            // Update target offset based on current sums
            if (sum_t1 != 0 || sum_t2 != 0) {
                targetOffset = (sum_t2 - sum_t1) / (float)(sum_t1 + sum_t2) * 0.3;
            }
            
            if (round_winner == 1) {
                snprintf(status_message, sizeof(status_message), "Team 1 Wins Round %d!", round_number);
            } else if (round_winner == 2) {
                snprintf(status_message, sizeof(status_message), "Team 2 Wins Round %d!", round_number);
            } else {
                snprintf(status_message, sizeof(status_message), "Round %d in progress", round_number);
            }
        } else {
            // EOF or error
            break;
        }
        
        // Check if there's more data
        FD_ZERO(&readfds);
        FD_SET(pipe_fd, &readfds);
    }

    // Animate rope position
    float speed = 0.002;
    if (fabs(currentOffset - targetOffset) > speed) {
        currentOffset += (currentOffset < targetOffset) ? speed : -speed;
    } else {
        currentOffset = targetOffset; // Snap to target when close
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'q' || key == 'Q' || key == 27) { // 27 is ESC key
        exit(0);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Rope Pulling Game Visualization");

    // Get pipe fd from command line
    if (argc > 1) {
        pipe_fd = atoi(argv[1]);
        printf("Graphics: Received pipe fd %d\n", pipe_fd);
        
        // Optional: set non-blocking mode
        int flags = fcntl(pipe_fd, F_GETFL, 0);
        fcntl(pipe_fd, F_SETFL, flags | O_NONBLOCK);
    } else {
        fprintf(stderr, "No pipe fd provided\n");
        return 1;
    }

    glClearColor(1.0, 1.0, 1.0, 1.0);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}