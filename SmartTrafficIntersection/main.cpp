#include <GL/freeglut.h>
#include <chrono>
#include <algorithm>

// Vehicle positions
float car1Y = 80.0f;
float car2Y = 520.0f;

const float NORTHBOUND_STOP_Y = 105.0f;
const float SOUTHBOUND_STOP_Y = 485.0f;

enum class TrafficPhase
{
    NS_GREEN,
    NS_YELLOW,
    ALL_RED_TO_EW,
    EW_GREEN,
    EW_YELLOW,
    ALL_RED_TO_NS
};

TrafficPhase currentPhase = TrafficPhase::NS_GREEN;
float phaseTimer = 0.0f;

const float GREEN_DURATION = 8.0f;
const float YELLOW_DURATION = 2.0f;
const float ALL_RED_DURATION = 1.0f;

// Vehicle speeds (units per second)
float car1Speed = 100.0f;
float car2Speed = 80.0f;

// Time of the previous update
auto lastUpdateTime = std::chrono::steady_clock::now();

void drawRectangle(float x1, float y1, float x2, float y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void drawCar(float x, float y, float angle, float scale)
{
    glPushMatrix();

    // Position and rotate the entire car
    glTranslatef(x, y, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    glScalef(scale, scale, 1.0f);

    // Wheels
    glColor3f(0.08f, 0.08f, 0.08f);

    drawRectangle(-20, -28, -14, -12);
    drawRectangle(14, -28, 20, -12);
    drawRectangle(-20, 12, -14, 28);
    drawRectangle(14, 12, 20, 28);

    // Car body
    glColor3f(0.90f, 0.15f, 0.15f);
    drawRectangle(-16, -35, 16, 35);

    // Front windshield
    glColor3f(0.55f, 0.80f, 0.95f);
    drawRectangle(-12, 10, 12, 25);

    // Rear windshield
    drawRectangle(-12, -25, 12, -12);

    // Headlights
    glColor3f(1.0f, 0.95f, 0.55f);
    drawRectangle(-12, 31, -5, 35);
    drawRectangle(5, 31, 12, 35);

    // Rear lights
    glColor3f(0.55f, 0.02f, 0.02f);
    drawRectangle(-12, -35, -5, -32);
    drawRectangle(5, -35, 12, -32);

    glPopMatrix();
}

void drawTrafficLight(float x, float y, bool vertical)
{
    glPushMatrix();

    glTranslatef(x, y, 0.0f);

    // Rotate the traffic light if needed
    if (!vertical)
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);

    // Traffic light housing
    glColor3f(0.08f, 0.08f, 0.08f);
    drawRectangle(-10, -28, 10, 28);

    bool red = false;
    bool yellow = false;
    bool green = false;

    // Determine which light is active
    if (vertical)
    {
        red = currentPhase != TrafficPhase::NS_GREEN &&
            currentPhase != TrafficPhase::NS_YELLOW;

        yellow = currentPhase == TrafficPhase::NS_YELLOW;
        green = currentPhase == TrafficPhase::NS_GREEN;
    }
    else
    {
        red = currentPhase != TrafficPhase::EW_GREEN &&
            currentPhase != TrafficPhase::EW_YELLOW;

        yellow = currentPhase == TrafficPhase::EW_YELLOW;
        green = currentPhase == TrafficPhase::EW_GREEN;
    }

    // Red indicator
    glColor3f(red ? 1.0f : 0.25f, 0.0f, 0.0f);
    drawRectangle(-7, 11, 7, 23);

    // Yellow indicator
    glColor3f(yellow ? 1.0f : 0.25f,
        yellow ? 0.85f : 0.20f, 0.0f);
    drawRectangle(-7, -6, 7, 6);

    // Green indicator
    glColor3f(0.0f, green ? 1.0f : 0.20f, 0.0f);
    drawRectangle(-7, -23, 7, -11);

    glPopMatrix();
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

    // Draw stop lines for right-hand traffic
    glColor3f(1.0f, 1.0f, 1.0f);

    // North approach - vehicles moving south
    drawRectangle(305, 450, 395, 455);

    // South approach - vehicles moving north
    drawRectangle(405, 145, 495, 150);

    // West approach - vehicles moving east
    drawRectangle(245, 205, 250, 295);

    // East approach - vehicles moving west
    drawRectangle(550, 305, 555, 395);

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

    // Northbound vehicle
    drawCar(450.0f, car1Y, 0.0f, 1.0f);

    // Southbound vehicle
    drawCar(350.0f, car2Y, 180.0f, 0.75f);

    // North and south traffic lights
    drawTrafficLight(285.0f, 460.0f, true);
    drawTrafficLight(515.0f, 140.0f, true);

    // East and west traffic lights
    drawTrafficLight(240.0f, 415.0f, false);
    drawTrafficLight(560.0f, 185.0f, false);

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

void updateTrafficLights(float deltaTime)
{
    phaseTimer += deltaTime;

    switch (currentPhase)
    {
    case TrafficPhase::NS_GREEN:
        if (phaseTimer >= GREEN_DURATION)
        {
            currentPhase = TrafficPhase::NS_YELLOW;
            phaseTimer = 0.0f;
        }
        break;

    case TrafficPhase::NS_YELLOW:
        if (phaseTimer >= YELLOW_DURATION)
        {
            currentPhase = TrafficPhase::ALL_RED_TO_EW;
            phaseTimer = 0.0f;
        }
        break;

    case TrafficPhase::ALL_RED_TO_EW:
        if (phaseTimer >= ALL_RED_DURATION)
        {
            currentPhase = TrafficPhase::EW_GREEN;
            phaseTimer = 0.0f;
        }
        break;

    case TrafficPhase::EW_GREEN:
        if (phaseTimer >= GREEN_DURATION)
        {
            currentPhase = TrafficPhase::EW_YELLOW;
            phaseTimer = 0.0f;
        }
        break;

    case TrafficPhase::EW_YELLOW:
        if (phaseTimer >= YELLOW_DURATION)
        {
            currentPhase = TrafficPhase::ALL_RED_TO_NS;
            phaseTimer = 0.0f;
        }
        break;

    case TrafficPhase::ALL_RED_TO_NS:
        if (phaseTimer >= ALL_RED_DURATION)
        {
            currentPhase = TrafficPhase::NS_GREEN;
            phaseTimer = 0.0f;
        }
        break;
    }
}

bool canEnterIntersection()
{
    return currentPhase == TrafficPhase::NS_GREEN;
}

void updateVehicles(float deltaTime)
{
    // Calculate proposed vehicle positions
    float nextCar1Y = car1Y + car1Speed * deltaTime;
    float nextCar2Y = car2Y - car2Speed * deltaTime;

    // Stop vehicles before their stop lines
    if (!canEnterIntersection())
    {
        // Northbound car
        if (car1Y <= NORTHBOUND_STOP_Y)
        {
            nextCar1Y = std::min(nextCar1Y, NORTHBOUND_STOP_Y);
        }

        // Southbound car
        if (car2Y >= SOUTHBOUND_STOP_Y)
        {
            nextCar2Y = std::max(nextCar2Y, SOUTHBOUND_STOP_Y);
        }
    }

    // Apply new positions
    car1Y = nextCar1Y;
    car2Y = nextCar2Y;

    // Restart cars after leaving the screen
    if (car1Y > 650.0f)
        car1Y = -50.0f;

    if (car2Y < -50.0f)
        car2Y = 650.0f;
}

void update(int value)
{
    // Calculate elapsed time
    auto currentTime = std::chrono::steady_clock::now();

    std::chrono::duration<float> elapsed =
        currentTime - lastUpdateTime;

    float deltaTime = elapsed.count();

    lastUpdateTime = currentTime;

    // Prevent unusually large movement jumps
    if (deltaTime > 0.05f)
        deltaTime = 0.05f;

    // Update the automatic traffic light controller
    updateTrafficLights(deltaTime);
    
    // Update vehicle positions
    updateVehicles(deltaTime);

    // Request a new frame
    glutPostRedisplay();

    // Schedule the next update
    glutTimerFunc(16, update, 0);
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

    // Start vehicle animation
	lastUpdateTime = std::chrono::steady_clock::now();
	glutTimerFunc(16, update, 0);

    // Start the application loop
    glutMainLoop();

    return 0;
}
