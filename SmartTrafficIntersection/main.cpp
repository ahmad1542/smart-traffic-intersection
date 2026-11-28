#include <GL/freeglut.h>
#include <chrono>
#include <algorithm>
#include <vector>
#include <random>
#include <cstdio>

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

	bool waiting = false;
};

std::vector<Vehicle> vehicles;

int totalPassedVehicles = 0;

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

float adaptiveGreenDuration = 8.0f;

// Forward declarations
int countWaitingVehicles(SignalGroup group);
float calculateGreenDuration(SignalGroup group);

// Vehicle speeds (units per second)
float car1Speed = 100.0f;
float car2Speed = 80.0f;

float car3Speed = 90.0f;
float car4Speed = 70.0f;

// Time of the previous update
auto lastUpdateTime = std::chrono::steady_clock::now();

const float DEFAULT_CAMERA_X = 400.0f;
const float DEFAULT_CAMERA_Y = 300.0f;
const float DEFAULT_CAMERA_ZOOM = 450.0f;

// Current camera state
float cameraX = DEFAULT_CAMERA_X;
float cameraY = DEFAULT_CAMERA_Y;
float cameraZoom = DEFAULT_CAMERA_ZOOM;

// Camera limits
const float MIN_ZOOM = 300.0f;
const float MAX_ZOOM = 520.0f;
const float CAMERA_MOVE_SPEED = 35.0f;

int windowWidth = 800;
int windowHeight = 600;

bool isPaused = false;

float simulationSpeed = 1.0f;

const float MIN_SIMULATION_SPEED = 0.25f;
const float MAX_SIMULATION_SPEED = 3.0f;
const float SPEED_STEP = 0.25f;

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
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *character);
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

const char* getSignalText(SignalGroup group)
{
	if (group == SignalGroup::NorthSouth)
	{
		if (currentPhase == TrafficPhase::NS_GREEN)
			return "GREEN";

		if (currentPhase == TrafficPhase::NS_YELLOW)
			return "YELLOW";
	}
	else
	{
		if (currentPhase == TrafficPhase::EW_GREEN)
			return "GREEN";

		if (currentPhase == TrafficPhase::EW_YELLOW)
			return "YELLOW";
	}

	return "RED";
}

void drawAdaptiveIndicator()
{
	if (currentMode != TrafficMode::Automatic)
		return;

	int waitingNS =
		countWaitingVehicles(SignalGroup::NorthSouth);

	int waitingEW =
		countWaitingVehicles(SignalGroup::EastWest);

	char text[100];

	// Display waiting vehicle counts
	snprintf(
		text,
		sizeof(text),
		"Waiting: N-S %d | E-W %d",
		waitingNS,
		waitingEW
	);

	glColor3f(0.12f, 0.16f, 0.20f);
	drawRectangle(10.0f, 465.0f, 260.0f, 500.0f);

	glColor3f(1.0f, 1.0f, 1.0f);
	drawText(20.0f, 478.0f, text);
}

void drawDashboard()
{
	char text[100];

	// Dashboard background
	glColor3f(0.12f, 0.16f, 0.20f);
	drawRectangle(10.0f, 395.0f, 260.0f, 590.0f);

	// Dashboard title
	glColor3f(0.40f, 0.90f, 0.60f);
	drawText(20.0f, 574.0f, "TRAFFIC MONITOR");

	// Reset text color to white
	glColor3f(1.0f, 1.0f, 1.0f);

	// Traffic mode
	const char* mode =
		currentMode == TrafficMode::Automatic
		? "AUTOMATIC" : "MANUAL";

	snprintf(text, sizeof(text),
		"Mode: %s", mode);

	drawText(20.0f, 558.0f, text);

	// Traffic density
	const char* density = "NORMAL";

	switch (currentDensity)
	{
	case TrafficDensity::Low:
		density = "LOW";
		break;

	case TrafficDensity::Normal:
		density = "NORMAL";
		break;

	case TrafficDensity::High:
		density = "HIGH";
		break;
	}

	snprintf(text, sizeof(text),
		"Density: %s", density);

	drawText(20.0f, 542.0f, text);

	// North-South signal
	snprintf(text, sizeof(text),
		"North-South: %s",
		getSignalText(SignalGroup::NorthSouth));

	drawText(20.0f, 526.0f, text);

	// East-West signal
	snprintf(text, sizeof(text),
		"East-West: %s",
		getSignalText(SignalGroup::EastWest));

	drawText(20.0f, 510.0f, text);

	// Allocated green duration
	if (currentMode == TrafficMode::Automatic)
	{
		snprintf(text, sizeof(text),
			"Allocated green: %.1f s",
			adaptiveGreenDuration);
	}
	else
	{
		snprintf(text, sizeof(text),
			"Green timing: MANUAL");
	}

	drawText(20.0f, 494.0f, text);

	// Number of active vehicles
	snprintf(text, sizeof(text),
		"Active vehicles: %zu",
		vehicles.size());

	drawText(20.0f, 478.0f, text);

	// Waiting vehicles
	int waitingNS =
		countWaitingVehicles(SignalGroup::NorthSouth);

	int waitingEW =
		countWaitingVehicles(SignalGroup::EastWest);

	snprintf(text, sizeof(text),
		"Waiting: N-S %d | E-W %d",
		waitingNS, waitingEW);

	drawText(20.0f, 462.0f, text);

	// Total passed vehicles
	snprintf(text, sizeof(text),
		"Vehicles passed: %d",
		totalPassedVehicles);

	drawText(20.0f, 446.0f, text);

	// Simulation status
	const char* status = isPaused ? "PAUSED" : "RUNNING";

	snprintf(text, sizeof(text),
		"Simulation: %s", status);

	drawText(20.0f, 427.0f, text);

	// Simulation speed
	snprintf(text, sizeof(text),
		"Speed: %.2fx", simulationSpeed);

	drawText(20.0f, 409.0f, text);
}

void drawDashboardOverlay()
{
	// Save the world projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// Use window coordinates for the dashboard
	glOrtho(0, windowWidth, 0, windowHeight, -1, 1);

	// Save the model-view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Make the dashboard readable on large screens
	float uiScale = 1.3f;

	// Move the dashboard to the upper-left corner
	glTranslatef(
		0.0f,
		windowHeight - 600.0f * uiScale,
		0.0f
	);

	glScalef(uiScale, uiScale, 1.0f);

	// Draw the dashboard
	drawDashboard();

	// Restore the original matrices
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
}

// Draw the entire scene
void display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	// 1. Draw the roads
	glColor3f(0.20f, 0.22f, 0.25f);

	// Long vertical road
	drawRectangle(300, -600, 500, 1200);

	// Long horizontal road
	drawRectangle(-800, 200, 1600, 400);

	glColor3f(1.0f, 0.85f, 0.20f);

	// Vertical center lines
	drawRectangle(398, -600, 402, 200);
	drawRectangle(398, 400, 402, 1200);

	// Horizontal center lines
	drawRectangle(-800, 298, 300, 302);
	drawRectangle(500, 298, 1600, 302);

	glColor3f(1.0f, 1.0f, 1.0f);

	// Vertical boundaries
	drawRectangle(300, -600, 303, 200);
	drawRectangle(497, -600, 500, 200);
	drawRectangle(300, 400, 303, 1200);
	drawRectangle(497, 400, 500, 1200);

	// Horizontal boundaries
	drawRectangle(-800, 200, 300, 203);
	drawRectangle(-800, 397, 300, 400);
	drawRectangle(500, 200, 1600, 203);
	drawRectangle(500, 397, 1600, 400);

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

	// Draw dashboard independently of the camera
	drawDashboardOverlay();

	glutSwapBuffers();
}

void updateCameraProjection()
{
	float aspect =
		static_cast<float>(windowWidth) / windowHeight;

	float halfWidth = cameraZoom * aspect;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(
		cameraX - halfWidth,
		cameraX + halfWidth,
		cameraY - cameraZoom,
		cameraY + cameraZoom,
		-1.0, 1.0
	);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void reshape(int width, int height)
{
	if (width < 1)
		width = 1;

	if (height < 1)
		height = 1;

	windowWidth = width;
	windowHeight = height;

	glViewport(0, 0, width, height);

	updateCameraProjection();
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

		// Calculate adaptive timing for the current green phase
		if (currentPhase == TrafficPhase::NS_GREEN)
		{
			adaptiveGreenDuration = calculateGreenDuration(SignalGroup::NorthSouth);
			phaseTimer = 0.0f;
		}
		else if (currentPhase == TrafficPhase::EW_GREEN)
		{
			adaptiveGreenDuration = calculateGreenDuration(SignalGroup::EastWest);
			phaseTimer = 0.0f;
		}
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

int countWaitingVehicles(SignalGroup group)
{
	int count = 0;

	for (const Vehicle& car : vehicles)
	{
		if (!car.waiting)
			continue;

		bool northSouth =
			car.direction == Direction::North ||
			car.direction == Direction::South;

		if (group == SignalGroup::NorthSouth && northSouth)
		{
			count++;
		}
		else if (group == SignalGroup::EastWest && !northSouth)
		{
			count++;
		}
	}

	return count;
}

const float MIN_GREEN_DURATION = 5.0f;
const float MAX_GREEN_DURATION = 14.0f;
const float EXTRA_TIME_PER_CAR = 1.5f;

float calculateGreenDuration(SignalGroup group)
{
	int waitingCars = countWaitingVehicles(group);

	float duration =
		MIN_GREEN_DURATION +
		waitingCars * EXTRA_TIME_PER_CAR;

	return std::min(duration, MAX_GREEN_DURATION);
}

void beginTrafficPhase(TrafficPhase nextPhase)
{
	currentPhase = nextPhase;
	phaseTimer = 0.0f;

	// Calculate green duration when automatic mode is active
	if (currentMode != TrafficMode::Automatic)
		return;

	if (nextPhase == TrafficPhase::NS_GREEN)
	{
		adaptiveGreenDuration =
			calculateGreenDuration(SignalGroup::NorthSouth);
	}
	else if (nextPhase == TrafficPhase::EW_GREEN)
	{
		adaptiveGreenDuration =
			calculateGreenDuration(SignalGroup::EastWest);
	}
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
			if (phaseTimer >= adaptiveGreenDuration)
			{
				beginTrafficPhase(TrafficPhase::NS_YELLOW);
			}
		}
		else if (
			requestedGroup == SignalGroup::EastWest &&
			phaseTimer >= MIN_MANUAL_GREEN)
		{
			beginTrafficPhase(TrafficPhase::NS_YELLOW);
		}
		break;
	}

	case TrafficPhase::NS_YELLOW:
	{
		if (phaseTimer >= YELLOW_DURATION)
		{
			beginTrafficPhase(TrafficPhase::ALL_RED_TO_EW);
		}
		break;
	}

	case TrafficPhase::ALL_RED_TO_EW:
	{
		if (phaseTimer >= ALL_RED_DURATION)
		{
			beginTrafficPhase(TrafficPhase::EW_GREEN);
		}
		break;
	}

	case TrafficPhase::EW_GREEN:
	{
		if (currentMode == TrafficMode::Automatic)
		{
			if (phaseTimer >= adaptiveGreenDuration)
			{
				beginTrafficPhase(TrafficPhase::EW_YELLOW);
			}
		}
		else if (
			requestedGroup == SignalGroup::NorthSouth &&
			phaseTimer >= MIN_MANUAL_GREEN)
		{
			beginTrafficPhase(TrafficPhase::EW_YELLOW);
		}
		break;
	}

	case TrafficPhase::EW_YELLOW:
	{
		if (phaseTimer >= YELLOW_DURATION)
		{
			beginTrafficPhase(TrafficPhase::ALL_RED_TO_NS);
		}
		break;
	}

	case TrafficPhase::ALL_RED_TO_NS:
	{
		if (phaseTimer >= ALL_RED_DURATION)
		{
			beginTrafficPhase(TrafficPhase::NS_GREEN);
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
	case 0: // Northbound
		car.direction = Direction::North;
		car.x = 450.0f;
		car.y = -500.0f;
		car.angle = 0.0f;
		break;

	case 1: // Southbound
		car.direction = Direction::South;
		car.x = 350.0f;
		car.y = 1100.0f;
		car.angle = 180.0f;
		break;

	case 2: // Eastbound
		car.direction = Direction::East;
		car.x = -700.0f;
		car.y = 250.0f;
		car.angle = 270.0f;
		break;

	case 3: // Westbound
		car.direction = Direction::West;
		car.x = 1500.0f;
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

void resetSimulation()
{
	// Remove existing vehicles
	vehicles.clear();

	// Reset statistics
	totalPassedVehicles = 0;

	// Reset traffic signals
	currentPhase = TrafficPhase::NS_GREEN;
	phaseTimer = 0.0f;

	// Reset traffic mode
	currentMode = TrafficMode::Automatic;
	requestedGroup = SignalGroup::NorthSouth;

	// Reset traffic density
	setTrafficDensity(TrafficDensity::Normal);

	// Reset spawning direction
	nextSpawnDirection = 0;

	// Reset simulation controls
	isPaused = false;
	simulationSpeed = 1.0f;

	// Recalculate initial green duration
	adaptiveGreenDuration =
		calculateGreenDuration(SignalGroup::NorthSouth);

	// Reset time reference
	lastUpdateTime = std::chrono::steady_clock::now();
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
	// Count vehicles before removing exited cars
	size_t previousCount = vehicles.size();

	vehicles.erase(
		std::remove_if(
			vehicles.begin(),
			vehicles.end(),
			[](const Vehicle& car)
			{
				switch (car.direction)
				{
				case Direction::North:
					return car.y > 1100.0f;

				case Direction::South:
					return car.y < -500.0f;

				case Direction::East:
					return car.x > 1500.0f;

				case Direction::West:
					return car.x < -700.0f;
				}

				return false;
			}),
		vehicles.end());

	// Count how many vehicles have exited
	totalPassedVehicles +=
		static_cast<int>(previousCount - vehicles.size());
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

		// A vehicle is waiting if its movement was blocked
		float actualMovement = allowedProgress - currentProgress;

		car.waiting = actualMovement < movement - 0.01f;

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
	// Calculate elapsed real time
	auto currentTime = std::chrono::steady_clock::now();

	std::chrono::duration<float> elapsed =
		currentTime - lastUpdateTime;

	float deltaTime = elapsed.count();

	lastUpdateTime = currentTime;

	// Prevent large time jumps
	deltaTime = std::min(deltaTime, 0.05f);

	// Update simulation only when running
	if (!isPaused)
	{
		// Apply simulation speed
		float simulationDeltaTime =
			deltaTime * simulationSpeed;

		updateTrafficLights(simulationDeltaTime);

		updateVehicles(simulationDeltaTime);

		removeExitedVehicles();

		updateSpawning(simulationDeltaTime);
	}

	// Refresh the display
	glutPostRedisplay();

	// Continue calling the timer
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

	case 'c':
	case 'C':
		cameraX = DEFAULT_CAMERA_X;
		cameraY = DEFAULT_CAMERA_Y;
		cameraZoom = DEFAULT_CAMERA_ZOOM;

		updateCameraProjection();
		break;

	case ' ':
		// Pause or resume
		isPaused = !isPaused;
		break;

	case '[':
		// Decrease simulation speed
		simulationSpeed = std::max(
			MIN_SIMULATION_SPEED,
			simulationSpeed - SPEED_STEP
		);
		break;

	case ']':
		// Increase simulation speed
		simulationSpeed = std::min(
			MAX_SIMULATION_SPEED,
			simulationSpeed + SPEED_STEP
		);
		break;

	case 'r':
	case 'R':
		// Reset the simulation
		resetSimulation();
		break;

	case 27:
		glutLeaveMainLoop();
		break;
	}

	glutPostRedisplay();
}

void handleMouseWheel(
	int wheel,
	int direction,
	int x,
	int y)
{
	if (direction > 0)
	{
		// Zoom in
		cameraZoom *= 0.9f;
	}
	else if (direction < 0)
	{
		// Zoom out
		cameraZoom *= 1.1f;
	}

	// Keep zoom within safe limits
	cameraZoom = std::clamp(
		cameraZoom,
		MIN_ZOOM,
		MAX_ZOOM
	);

	updateCameraProjection();
	glutPostRedisplay();
}

void handleSpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		cameraY += CAMERA_MOVE_SPEED;
		break;

	case GLUT_KEY_DOWN:
		cameraY -= CAMERA_MOVE_SPEED;
		break;

	case GLUT_KEY_LEFT:
		cameraX -= CAMERA_MOVE_SPEED;
		break;

	case GLUT_KEY_RIGHT:
		cameraX += CAMERA_MOVE_SPEED;
		break;
	}

	// Keep the camera near the intersection
	cameraX = std::clamp(cameraX, 300.0f, 500.0f);
	cameraY = std::clamp(cameraY, 200.0f, 400.0f);

	updateCameraProjection();
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

	// Initialize adaptive traffic light timing
	adaptiveGreenDuration = calculateGreenDuration(SignalGroup::NorthSouth);

	// Set the background to green
	glClearColor(0.25f, 0.55f, 0.30f, 1.0f);

	// Register drawing and resizing callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(handleKeyboard);
	glutMouseWheelFunc(handleMouseWheel);
	glutSpecialFunc(handleSpecialKeys);

	// Start vehicle animation
	lastUpdateTime = std::chrono::steady_clock::now();
	glutTimerFunc(16, update, 0);

	// Start the application loop
	glutMainLoop();

	return 0;
}
