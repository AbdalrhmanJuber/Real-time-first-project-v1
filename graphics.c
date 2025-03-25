#ifdef USE_OPENGL
#include <GL/glut.h>
#include <stdio.h>

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw a line for the rope
    glBegin(GL_LINES);
        glVertex2f(-0.8f, 0.0f);
        glVertex2f( 0.8f, 0.0f);
    glEnd();

    // TODO: draw your teams/players
    // e.g. rectangles or circles

    glFlush();
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(600, 400);
    glutCreateWindow("Rope Pulling");
    glClearColor(0.8f, 0.8f, 1.0f, 1.0f); // light-blue background
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
#endif
