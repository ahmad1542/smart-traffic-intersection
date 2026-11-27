#include <GL/freeglut.h>
#include <chrono>
#include <algorithm>
#include <vector>
#include <random>

enum class Direction
{
	North,
	South,
	East,
	West
};

struct Vehicle
{
	float x;
	float y;
	float speed;
	float scale;
	float angle;

	Direction direction;

	float red;
	float green;
	float blue;
};

std::vector<Vehicle> vehicles;

// Maximum number of active vehicles
const int MAX_VEHICLES = 30;

enum class TrafficDensity
{
	Low,
	Normal,
	High
};

TrafficDensity currentDensity = TrafficDensity::Normal;

float spawnInterval = 1.5f;
float spawnTimer = 0.0f;

// Select which direction spawns next
int nextSpawnDirection = 0;

// Random number generator
std::mt19937 randomGenerator(std::random_device{}());

const float NORTHBOUND_STOP_Y = 105.0f;
const float SOUTHBOUND_STOP_Y = 485.0f;

const float EASTBOUND_STOP_X = 205.0f;
const float WESTBOUND_STOP_X = 595.0f;

const float SAFE_GAP = 15.0f;
const float CAR_HALF_LENGTH = 35.0f;

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

enum class TrafficMode
{
	Automatic,
	Manual
};

enum class SignalGroup
{
	NorthSouth,
	EastWest
};

TrafficMode currentMode = TrafficMode::Automatic;

SignalGroup requestedGroup = SignalGroup::NorthSouth;

const float MIN_MANUAL_GREEN = 3.0f;

// Vehicle speeds (units per second)
float car1Speed = 100.0f;
float car2Speed = 80.0f;

float car3Speed = 90.0f;
float car4Speed = 70.0f;

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

void drawCar(const Vehicle& car)
{
	glPushMatrix();

	// Position and rotate the entire car
	glTranslatef(car.x, car.y, 0.0f);
	glRotatef(car.angle, 0.0f, 0.0f, 1.0f);
	glScalef(car.scale, car.scale, 1.0f);

	// Wheels
	glColor3f(0.08f, 0.08f, 0.08f);

	drawRectangle(-20, -28, -14, -12);
	drawRectangle(14, -28, 20, -12);
	drawRectangle(-20, 12, -14, 28);
	drawRectangle(14, 12, 20, 28);

	// Car body
	glColor3f(car.red, car.green, car.blue);
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

void drawText(float x, float y, const char* text)
{
	glRasterPos2f(x, y);

	for (const char* character = text; *character; ++character)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *character);
	}
}

void drawDensityIndicator()
{
	const char* densityText = "";

	switch (currentDensity)
	{
	case TrafficDensity::Low:
		densityText = "Traffic Density: LOW";
		break;

	case TrafficDensity::Normal:
		densityText = "Traffic Density: NORMAL";
		break;

	case TrafficDensity::High:
		densityText = "Traffic Density: HIGH";
		break;
	}

	// Dark background panel
	glColor3f(0.12f, 0.16f, 0.20f);
	drawRectangle(10.0f, 555.0f, 230.0f, 590.0f);

	// White text
	glColor3f(1.0f, 1.0f, 1.0f);
	drawText(20.0f, 568.0f, densityText);
}

void drawTrafficModeIndicator()
{
	const char* modeText;

	if (currentMode == TrafficMode::Automatic)
	{
		modeText = "Traffic Mode: AUTOMATIC";
	}
	else
	{
		modeText = "Traffic Mode: MANUAL";
	}

	// Dark panel
	glColor3f(0.12f, 0.16f, 0.20f);
	drawRectangle(10.0f, 510.0f, 230.0f, 545.0f);

	// White text
	glColor3f(1.0f, 1.0f, 1.0f);
	drawText(20.0f, 523.0f, modeText);
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

	for (const Vehicle& car : vehicles)
	{
		drawCar(car);
	}

	// North and south traffic lights
	drawTrafficLight(285.0f, 460.0f, true);
	drawTrafficLight(515.0f, 140.0f, true);

	// East and west traffic lights
	drawTrafficLight(240.0f, 415.0f, false);
	drawTrafficLight(560.0f, 185.0f, false);
	
	// Draw the traffic density indicator
	drawDensityIndicator();

	drawTrafficModeIndicator();

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

void toggleTrafficMode()
{
	if (currentMode == TrafficMode::Automatic)
	{
		currentMode = TrafficMode::Manual;

		// Keep the direction already active or
		// scheduled by the current traffic phase.
		switch (currentPhase)
		{
		case TrafficPhase::NS_GREEN:
		case TrafficPhase::EW_YELLOW:
		case TrafficPhase::ALL_RED_TO_NS:
			requestedGroup = SignalGroup::NorthSouth;
			break;

		default:
			requestedGroup = SignalGroup::EastWest;
			break;
		}
	}
	else
	{
		currentMode = TrafficMode::Automatic;
	}
}

void requestTrafficSignal(SignalGroup group)
{
	// Manual requests are only allowed in manual mode.
	if (currentMode != TrafficMode::Manual)
		return;

	// Store the requested direction.
	requestedGroup = group;
}

void updateTrafficLights(float deltaTime)
{
	phaseTimer += deltaTime;

	switch (currentPhase)
	{
	case TrafficPhase::NS_GREEN:
	{
		if (currentMode == TrafficMode::Automatic)
		{
			if (phaseTimer >= GREEN_DURATION)
			{
				currentPhase = TrafficPhase::NS_YELLOW;
				phaseTimer = 0.0f;
			}
		}
		else if (
			requestedGroup == SignalGroup::EastWest &&
			phaseTimer >= MIN_MANUAL_GREEN)
		{
			currentPhase = TrafficPhase::NS_YELLOW;
			phaseTimer = 0.0f;
		}
		break;
	}

	case TrafficPhase::NS_YELLOW:
	{
		if (phaseTimer >= YELLOW_DURATION)
		{
			currentPhase = TrafficPhase::ALL_RED_TO_EW;
			phaseTimer = 0.0f;
		}
		break;
	}

	case TrafficPhase::ALL_RED_TO_EW:
	{
		if (phaseTimer >= ALL_RED_DURATION)
		{
			currentPhase = TrafficPhase::EW_GREEN;
			phaseTimer = 0.0f;
		}
		break;
	}

	case TrafficPhase::EW_GREEN:
	{
		if (currentMode == TrafficMode::Automatic)
		{
			if (phaseTimer >= GREEN_DURATION)
			{
				currentPhase = TrafficPhase::EW_YELLOW;
				phaseTimer = 0.0f;
			}
		}
		else if (
			requestedGroup == SignalGroup::NorthSouth &&
			phaseTimer >= MIN_MANUAL_GREEN)
		{
			currentPhase = TrafficPhase::EW_YELLOW;
			phaseTimer = 0.0f;
		}
		break;
	}

	case TrafficPhase::EW_YELLOW:
	{
		if (phaseTimer >= YELLOW_DURATION)
		{
			currentPhase = TrafficPhase::ALL_RED_TO_NS;
			phaseTimer = 0.0f;
		}
		break;
	}

	case TrafficPhase::ALL_RED_TO_NS:
	{
		if (phaseTimer >= ALL_RED_DURATION)
		{
			currentPhase = TrafficPhase::NS_GREEN;
			phaseTimer = 0.0f;
		}
		break;
	}
	}
}

bool canEnterIntersection()
{
	return currentPhase == TrafficPhase::NS_GREEN;
}

bool canEastWestEnter()
{
	return currentPhase == TrafficPhase::EW_GREEN;
}

float getProgress(const Vehicle& car)
{
	switch (car.direction)
	{
	case Direction::North:
		return car.y;

	case Direction::South:
		return -car.y;

	case Direction::East:
		return car.x;

	case Direction::West:
		return -car.x;
	}

	return 0.0f;
}

float getAllowedProgress(const Vehicle& car, float desiredProgress) {

	float currentProgress = getProgress(car);
	float allowedProgress = desiredProgress;

	for (const Vehicle& other : vehicles)
	{
		// Ignore the current car
		if (&car == &other)
			continue;

		// Only compare vehicles in the same lane
		if (car.direction != other.direction)
			continue;

		float otherProgress = getProgress(other);

		// Ignore vehicles behind us
		if (otherProgress <= currentProgress)
			continue;

		// Calculate minimum distance between car centers
		float requiredDistance =
			CAR_HALF_LENGTH * car.scale +
			CAR_HALF_LENGTH * other.scale +
			SAFE_GAP;

		// We cannot move too close to the car ahead
		float maxProgress = otherProgress - requiredDistance;

		allowedProgress = std::min(allowedProgress, maxProgress);
	}

	// Never move backward just to maintain the gap
	return std::max(currentProgress, allowedProgress);
}

void spawnVehicle()
{
	// Do not exceed the maximum number of vehicles
	if (vehicles.size() >= MAX_VEHICLES)
		return;

	Vehicle car{};

	// Random vehicle size and speed
	std::uniform_real_distribution<float> speedDist(65.0f, 105.0f);
	std::uniform_real_distribution<float> scaleDist(0.8f, 1.0f);

	car.speed = speedDist(randomGenerator);
	car.scale = scaleDist(randomGenerator);

	// Choose the next spawning direction
	switch (nextSpawnDirection)
	{
	case 0:
		car.direction = Direction::North;
		car.x = 450.0f;
		car.y = -50.0f;
		car.angle = 0.0f;
		break;

	case 1:
		car.direction = Direction::South;
		car.x = 350.0f;
		car.y = 650.0f;
		car.angle = 180.0f;
		break;

	case 2:
		car.direction = Direction::East;
		car.x = -50.0f;
		car.y = 250.0f;
		car.angle = 270.0f;
		break;

	case 3:
		car.direction = Direction::West;
		car.x = 850.0f;
		car.y = 350.0f;
		car.angle = 90.0f;
		break;
	}

	// Move to the next direction for the next attempt
	nextSpawnDirection = (nextSpawnDirection + 1) % 4;

	// Check if the spawning position is safe
	for (const Vehicle& other : vehicles)
	{
		if (car.direction != other.direction)
			continue;

		float distance =
			std::abs(getProgress(car) - getProgress(other));

		float requiredDistance =
			35.0f * car.scale +
			35.0f * other.scale +
			SAFE_GAP;

		if (distance < requiredDistance)
			return;
	}

	// Choose a random vehicle color
	std::uniform_int_distribution<int> colorDist(0, 4);

	switch (colorDist(randomGenerator))
	{
	case 0:
		car.red = 0.9f;
		car.green = 0.15f;
		car.blue = 0.15f;
		break;

	case 1:
		car.red = 0.2f;
		car.green = 0.5f;
		car.blue = 0.9f;
		break;

	case 2:
		car.red = 0.9f;
		car.green = 0.7f;
		car.blue = 0.2f;
		break;

	case 3:
		car.red = 0.6f;
		car.green = 0.3f;
		car.blue = 0.8f;
		break;

	case 4:
		car.red = 0.1f;
		car.green = 0.8f;
		car.blue = 0.3f;
		break;
	}

	// Add the new vehicle
	vehicles.push_back(car);
}

void setTrafficDensity(TrafficDensity density)
{
	currentDensity = density;

	switch (density)
	{
	case TrafficDensity::Low:
		spawnInterval = 3.0f;
		break;

	case TrafficDensity::Normal:
		spawnInterval = 1.5f;
		break;

	case TrafficDensity::High:
		spawnInterval = 0.7f;
		break;
	}

	// Restart the countdown when changing density
	spawnTimer = 0.0f;
}

void updateSpawning(float deltaTime)
{
	spawnTimer += deltaTime;

	if (spawnTimer >= spawnInterval)
	{
		spawnTimer -= spawnInterval;
		spawnVehicle();
	}
}

void removeExitedVehicles()
{
	vehicles.erase(
		std::remove_if(
			vehicles.begin(),
			vehicles.end(),
			[](const Vehicle& car)
			{
				switch (car.direction)
				{
				case Direction::North:
					return car.y > 650.0f;

				case Direction::South:
					return car.y < -50.0f;

				case Direction::East:
					return car.x > 850.0f;

				case Direction::West:
					return car.x < -50.0f;
				}

				return false;
			}),
		vehicles.end());
}

void updateVehicles(float deltaTime)
{
	// Check if any vehicle is crossing the intersection
	bool nsInsideIntersection = false;
	bool ewInsideIntersection = false;

	for (const Vehicle& car : vehicles)
	{
		float halfLength = 35.0f * car.scale;

		if (car.direction == Direction::North ||
			car.direction == Direction::South)
		{
			if (car.y + halfLength > 200.0f &&
				car.y - halfLength < 400.0f)
			{
				nsInsideIntersection = true;
			}
		}
		else
		{
			if (car.x + halfLength > 300.0f &&
				car.x - halfLength < 500.0f)
			{
				ewInsideIntersection = true;
			}
		}
	}

	bool nsCanEnter =
		canEnterIntersection() && !ewInsideIntersection;

	bool ewCanEnter =
		canEastWestEnter() && !nsInsideIntersection;


	// Update every vehicle
	for (Vehicle& car : vehicles)
	{
		float movement = car.speed * deltaTime;

		float currentProgress = getProgress(car);
		float desiredProgress = currentProgress + movement;

		float halfLength = CAR_HALF_LENGTH * car.scale;
		float safetyGap = 5.0f;

		// Determine where the car must stop
		float stopProgress = 0.0f;
		bool canEnter = false;

		switch (car.direction)
		{
		case Direction::North:
			stopProgress = 145.0f - halfLength - safetyGap;
			canEnter = nsCanEnter;
			break;

		case Direction::South:
			stopProgress = -(455.0f + halfLength + safetyGap);
			canEnter = nsCanEnter;
			break;

		case Direction::East:
			stopProgress = 245.0f - halfLength - safetyGap;
			canEnter = ewCanEnter;
			break;

		case Direction::West:
			stopProgress = -(555.0f + halfLength + safetyGap);
			canEnter = ewCanEnter;
			break;
		}

		// Respect the traffic light
		if (!canEnter && currentProgress <= stopProgress)
		{
			desiredProgress =
				std::min(desiredProgress, stopProgress);
		}

		// Respect the vehicle ahead
		float allowedProgress =
			getAllowedProgress(car, desiredProgress);

		// Update the correct coordinate
		switch (car.direction)
		{
		case Direction::North:
			car.y = allowedProgress;
			break;

		case Direction::South:
			car.y = -allowedProgress;
			break;

		case Direction::East:
			car.x = allowedProgress;
			break;

		case Direction::West:
			car.x = -allowedProgress;
			break;
		}
	}
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

	// Remove vehicles that left the scene
	removeExitedVehicles();

	// Spawn new vehicles automatically
	updateSpawning(deltaTime);

	// Request a new frame
	glutPostRedisplay();

	// Schedule the next update
	glutTimerFunc(16, update, 0);
}

void handleKeyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '1':
		setTrafficDensity(TrafficDensity::Low);
		break;

	case '2':
		setTrafficDensity(TrafficDensity::Normal);
		break;

	case '3':
		setTrafficDensity(TrafficDensity::High);
		break;

	case 'm':
	case 'M':
		toggleTrafficMode();
		break;

	case 'n':
	case 'N':
		requestTrafficSignal(SignalGroup::NorthSouth);
		break;

	case 'e':
	case 'E':
		requestTrafficSignal(SignalGroup::EastWest);
		break;

	case 27:
		glutLeaveMainLoop();
		break;
	}

	glutPostRedisplay();
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
	glutKeyboardFunc(handleKeyboard);

	// Start vehicle animation
	lastUpdateTime = std::chrono::steady_clock::now();
	glutTimerFunc(16, update, 0);

	// Start the application loop
	glutMainLoop();

	return 0;
}
