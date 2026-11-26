
#include <GL/freeglut.h>

// Draw the entire scene
void display()
{
    // Clear the window
    glClear(GL_COLOR_BUFFER_BIT);

    // Set the road color (dark gray)
    glColor3f(0.2f, 0.2f, 0.2f);

    // Draw a vertical road
    glBegin(GL_QUADS);
    glVertex2f(350.0f, 0.0f);
    glVertex2f(450.0f, 0.0f);
    glVertex2f(450.0f, 600.0f);
    glVertex2f(350.0f, 600.0f);
    glEnd();

    // Display the rendered image
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
