#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

float team1[4] = {80, 70, 60, 50};
float team2[4] = {60, 55, 40, 35};
char winner[10] = "T1";

float currentOffset = 0.0;
float targetOffset = 0.0;

int gameTime = 30;
int remainingTime = 30;
int timeUp = 0;

volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigusr2_received = 0;
int freeze_data = 0;
char status_message[50] = "";

int compareDesc(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa < fb) - (fa > fb);
}

void readConfig() {
    FILE* file = fopen("config.txt", "r");
    if (file == NULL) return;
    char key[50];
    while (fscanf(file, "%s", key) != EOF) {
        if (strcmp(key, "max_game_time") == 0) {
            fscanf(file, "%d", &gameTime);
            remainingTime = gameTime;
        } else {
            fscanf(file, "%*s");
        }
    }
    fclose(file);
}

void readDataFromFile() {
    FILE* file = fopen("visual.txt", "r");
    if (file == NULL) return;

    fscanf(file, "T1: %f %f %f %f\n", &team1[0], &team1[1], &team1[2], &team1[3]);
    fscanf(file, "T2: %f %f %f %f\n", &team2[0], &team2[1], &team2[2], &team2[3]);
    fscanf(file, "WINNER: %s\n", winner);
    fclose(file);

    qsort(team1, 4, sizeof(float), compareDesc);
    qsort(team2, 4, sizeof(float), compareDesc);

    float sum1 = team1[0] + team1[1] + team1[2] + team1[3];
    float sum2 = team2[0] + team2[1] + team2[2] + team2[3];

    targetOffset = (sum2 - sum1) / 400.0;
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

    glColor3f(0.4, 0.2, 0.0);
    glBegin(GL_QUADS);
        glVertex2f(-0.9 + currentOffset, 0.0 - 0.01);
        glVertex2f(0.9 + currentOffset, 0.0 - 0.01);
        glVertex2f(0.9 + currentOffset, 0.0 + 0.01);
        glVertex2f(-0.9 + currentOffset, 0.0 + 0.01);
    glEnd();

    float sorted1[4], sorted2[4];
    memcpy(sorted1, team1, sizeof(team1));
    memcpy(sorted2, team2, sizeof(team2));
    qsort(sorted1, 4, sizeof(float), compareDesc);
    qsort(sorted2, 4, sizeof(float), compareDesc);

    for (int i = 0; i < 4; i++) {
        float x = -0.7 + i * 0.15 + currentOffset;
        drawPlayer(x, 0.05, 1, 0.0, 0.0, 1.0, sorted1[i]);
    }

    for (int i = 0; i < 4; i++) {
        float x = 0.7 - i * 0.15 + currentOffset;
        drawPlayer(x, 0.05, -1, 1.0, 0.0, 0.0, sorted2[i]);
    }

    if (status_message[0] != '\0') {
        drawText(-0.1, 0.85, status_message);
    } else {
        char msg[50];
        snprintf(msg, sizeof(msg), "Winner: %s", winner);
        drawText(-0.1, 0.85, msg);
    }

    char timeStr[50];
    if (!timeUp)
        snprintf(timeStr, sizeof(timeStr), "Time Left: %d sec", remainingTime);
    else
        snprintf(timeStr, sizeof(timeStr), "Time is up!");
    drawText(-0.9, 0.9, timeStr);

    float sum1 = team1[0] + team1[1] + team1[2] + team1[3];
    float sum2 = team2[0] + team2[1] + team2[2] + team2[3];
    char effortStr[100];
    snprintf(effortStr, sizeof(effortStr), "T1 Total: %.0f     T2 Total: %.0f", sum1, sum2);
    drawText(-0.25, 0.8, effortStr);

    glutSwapBuffers();
}

void timer(int value) {
    if (!timeUp) {
        if (!freeze_data) {
            readDataFromFile();
        }
        remainingTime--;
        if (remainingTime <= 0) {
            timeUp = 1;
        }
    }
    glutTimerFunc(1000, timer, 0);
}

void update(int value) {
    if (sigusr1_received) {
        printf("Graphics Recieved SIGUSR1\n\n");
        for (int i = 0; i < 4; i++) {
            team1[i] = 0.0f;
            team2[i] = 0.0f;
        }
        strncpy(status_message, "starting a round", sizeof(status_message));
        freeze_data = 1;
        sigusr1_received = 0;
    }
    if (sigusr2_received) {
        printf("Graphics Recieved SIGUSR2\n\n");
        freeze_data = 0;
        readDataFromFile();
        strncpy(status_message, "Pulling the rope", sizeof(status_message));
        sigusr2_received = 0;
    }

    float speed = 0.002;
    if (!timeUp && fabs(currentOffset - targetOffset) > speed) {
        currentOffset += (currentOffset < targetOffset) ? speed : -speed;
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'r' || key == 'R') {
        if (freeze_data) return;
        remainingTime = gameTime;
        timeUp = 0;
        currentOffset = 0.0;
        readDataFromFile();
    }
}

void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

void handle_sigusr2(int sig) {
    sigusr2_received = 1;
}

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Rope Pulling Game Visualization");

    init();
    readConfig();
    readDataFromFile();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(1000, timer, 0);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}