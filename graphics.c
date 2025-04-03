// #include <GL/glut.h>
// #include <math.h>
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>

// // بيانات اللاعبين
// float team1[4] = {80, 70, 60, 50};
// float team2[4] = {60, 55, 40, 35};
// char winner[10] = "T1";

// // إزاحة الحبل واللاعبين
// float currentOffset = 0.0;
// float targetOffset = 0.0;

// // قراءة البيانات من الملف
// void readDataFromFile() {
//     FILE* file = fopen("visual.txt", "r");
//     if (file == NULL) return;

//     fscanf(file, "T1: %f %f %f %f\n", &team1[0], &team1[1], &team1[2], &team1[3]);
//     fscanf(file, "T2: %f %f %f %f\n", &team2[0], &team2[1], &team2[2], &team2[3]);
//     fscanf(file, "WINNER: %s\n", winner);

//     fclose(file);

//     float sum1 = team1[0] + team1[1] + team1[2] + team1[3]; // الأزرق
//     float sum2 = team2[0] + team2[1] + team2[2] + team2[3]; // الأحمر

//     // ✅ تعديل الاتجاه: الحبل يتحرك نحو الفريق الأقوى
//     targetOffset = (sum2 - sum1) / 400.0; // from config.txt later ...
// }

// // عرض الطاقة فوق اللاعب
// void drawEnergy(float x, float y, float energy) {
//     char buffer[10];
//     snprintf(buffer, sizeof(buffer), "%.0f", energy);
//     glColor3f(0.0, 0.0, 0.0);
//     glRasterPos2f(x - 0.01, y + 0.03);
//     for (int i = 0; buffer[i] != '\0'; i++) {
//         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
//     }
// }

// // رسم لاعب
// void drawPlayer(float x, float y, int flipped, float r, float g, float b, float energy) {
//     if (energy <= 0.1)
//         glColor3f(0.6, 0.6, 0.6);
//     else
//         glColor3f(1.0, 0.8, 0.6);

//     glBegin(GL_POLYGON);
//     for (int i = 0; i < 360; i++) {
//         float theta = i * 3.14159 / 180;
//         glVertex2f(x + 0.02 * cos(theta), y + 0.02 * sin(theta));
//     }
//     glEnd();

//     glColor3f(r, g, b);
//     glBegin(GL_QUADS);
//         glVertex2f(x - 0.02, y - 0.02);
//         glVertex2f(x + 0.02, y - 0.02);
//         glVertex2f(x + 0.02, y - 0.08);
//         glVertex2f(x - 0.02, y - 0.08);
//     glEnd();

//     glColor3f(0.0, 0.0, 0.0);
//     glBegin(GL_LINES);
//         glVertex2f(x - 0.02, y - 0.04);
//         glVertex2f(x - 0.06 * flipped, y - 0.04);
//         glVertex2f(x + 0.02, y - 0.04);
//         glVertex2f(x + 0.06 * flipped, y - 0.04);
//     glEnd();

//     drawEnergy(x, y, energy);
// }

// // عرض نص
// void drawText(float x, float y, const char *text) {
//     glRasterPos2f(x, y);
//     for (int i = 0; text[i] != '\0'; i++) {
//         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
//     }
// }

// void display() {
//     glClear(GL_COLOR_BUFFER_BIT);

//     // رسم الحبل في المنتصف
//     glColor3f(0.4, 0.2, 0.0);
//     glBegin(GL_QUADS);
//         glVertex2f(-0.9 + currentOffset, 0.0 - 0.01);
//         glVertex2f(0.9 + currentOffset, 0.0 - 0.01);
//         glVertex2f(0.9 + currentOffset, 0.0 + 0.01);
//         glVertex2f(-0.9 + currentOffset, 0.0 + 0.01);
//     glEnd();

//     // الفريق الأزرق
//     for (int i = 0; i < 4; i++) {
//         float x = -0.7 + i * 0.15 + currentOffset;
//         drawPlayer(x, 0.05, 1, 0.0, 0.0, 1.0, team1[i]);
//     }

//     // الفريق الأحمر
//     for (int i = 0; i < 4; i++) {
//         float x = 0.7 - i * 0.15 + currentOffset;
//         drawPlayer(x, 0.05, -1, 1.0, 0.0, 0.0, team2[i]);
//     }

//     // عرض الفائز
//     glColor3f(0.0, 0.0, 0.0);
//     char msg[50];
//     snprintf(msg, sizeof(msg), "Winner: %s", winner);
//     drawText(-0.1, 0.85, msg);

//     glutSwapBuffers();
// }

// // مؤقت قراءة البيانات
// void timer(int value) {
//     readDataFromFile();
//     glutTimerFunc(1000, timer, 0);
// }

// // تحديث ناعم للحركة
// void update(int value) {
//     float speed = 0.002;
//     if (fabs(currentOffset - targetOffset) > speed) {
//         currentOffset += (currentOffset < targetOffset) ? speed : -speed;
//     }
//     glutPostRedisplay();
//     glutTimerFunc(16, update, 0);
// }

// void init() {
//     glClearColor(1.0, 1.0, 1.0, 1.0);
// }

// int main(int argc, char** argv) {
//     glutInit(&argc, argv);
//     glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
//     glutInitWindowSize(800, 600);
//     glutCreateWindow("Rope Pulling Game Visualization");
//     init();
//     glutDisplayFunc(display);
//     glutTimerFunc(1000, timer, 0);
//     glutTimerFunc(16, update, 0);
//     glutMainLoop();
//     return 0;
// }