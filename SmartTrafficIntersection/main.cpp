#include <GL/freeglut.h>

void drawRectangle(float x1, float y1, float x2, float y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}


// Draw the entire scene

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // 1. Draw the roads
    glColor3f(0.20f, 0.22f, 0.25f);

    // Vertical road
    drawRectangle(300, 0, 500, 600);

    // Horizontal road
    drawRectangle(0, 200, 800, 400);

    // 2. Draw yellow center lines
    glColor3f(1.0f, 0.85f, 0.20f);

    // Vertical road center lines
    drawRectangle(398, 0, 402, 200);
    drawRectangle(398, 400, 402, 600);

    // Horizontal road center lines
    drawRectangle(0, 298, 300, 302);
    drawRectangle(500, 298, 800, 302);

    // 3. Draw white road boundaries
    glColor3f(1.0f, 1.0f, 1.0f);

    // Vertical boundaries
    drawRectangle(300, 0, 303, 200);
    drawRectangle(497, 0, 500, 200);
    drawRectangle(300, 400, 303, 600);
    drawRectangle(497, 400, 500, 600);

    // Horizontal boundaries
    drawRectangle(0, 200, 300, 203);
    drawRectangle(0, 397, 300, 400);
    drawRectangle(500, 200, 800, 203);
    drawRectangle(500, 397, 800, 400);

    // 4. Draw pedestrian crossings
    for (int i = 0; i < 6; i++)
    {
        float offset = i * 30.0f;

        // North crossing
        drawRectangle(315 + offset, 410,
            330 + offset, 440);

        // South crossing
        drawRectangle(315 + offset, 160,
            330 + offset, 190);

        // West crossing
        drawRectangle(260, 215 + offset,
            290, 230 + offset);

        // East crossing
        drawRectangle(510, 215 + offset,
            540, 230 + offset);
    }

    glutSwapBuffers();
}


// Configure the 2D coordinate system
void reshape(int width, int height)
{
    if (height == 0)
        height = 1;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Keep the scene coordinates fixed
    glOrtho(0.0, 800.0, 0.0, 600.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv)
{
    // Initialize FreeGLUT
    glutInit(&argc, argv);

    // Enable double buffering and RGB colors
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // Set window dimensions
    glutInitWindowSize(800, 600);

    // Create the window
    glutCreateWindow("Smart Traffic Intersection");

    // Set the background to green
    glClearColor(0.25f, 0.55f, 0.30f, 1.0f);

    // Register drawing and resizing callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    // Start the application loop
    glutMainLoop();

    return 0;
}
