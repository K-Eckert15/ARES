#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// PCA9685 default I2C address
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// ============================================================
// SERVO SETTINGS
// ============================================================

const uint8_t SERVO_CHANNEL = 0;

// Default reciprocation endpoints
float minAngleDeg = -50.0;
float maxAngleDeg = 50.0;

// Travel speed in degrees per second
float speedDegPerSec = 100.0;

// Servo pulse calibration
const uint16_t SERVO_MIN_US = 600;
const uint16_t SERVO_CENTER_US = 1600;
const uint16_t SERVO_MAX_US = 2600;

// Mechanical range represented by the pulse settings
const float SERVO_MECHANICAL_RANGE_DEG = 180.0;

// Servo update timing
const unsigned long UPDATE_INTERVAL_MS = 20;

// Pause at each endpoint
unsigned long endpointPauseMs = 250;

// ============================================================
// MOTION VARIABLES
// ============================================================

float currentAngleDeg = 0.0;

int direction = 1;

bool motionRunning = false;
bool endpointPause = false;

unsigned long previousUpdateMs = 0;
unsigned long endpointReachedMs = 0;

// ============================================================
// SERIAL VARIABLES
// ============================================================

const uint8_t SERIAL_BUFFER_SIZE = 64;

char serialBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialBufferIndex = 0;

// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

void readSerial();
void processCommand(char *command);

void startMotion();
void stopMotion();
void updateMotion();

void moveServoToAngle(float angleDeg);
float constrainServoAngle(float angleDeg);

void printHelp();
void printStatus();

char *trimWhitespace(char *text);
void convertToUppercase(char *text);

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  Wire.begin();

  pwm.begin();
  pwm.setPWMFreq(50);

  // Give the PCA9685 time to stabilize
  delay(100);

  // Start centered and stopped
  currentAngleDeg = 0.0;
  moveServoToAngle(currentAngleDeg);

  previousUpdateMs = millis();

  Serial.println();
  Serial.println("======================================");
  Serial.println(" PCA9685 Servo Reciprocation Controller");
  Serial.println("======================================");
  Serial.println();
  Serial.println("Servo initialized at 0 degrees.");
  Serial.println("Motion is STOPPED.");
  Serial.println();

  printHelp();
  printStatus();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  readSerial();

  if (motionRunning)
  {
    updateMotion();
  }
}

// ============================================================
// MOTION CONTROL
// ============================================================

void startMotion()
{
  if (motionRunning)
  {
    Serial.println("Motion is already running.");
    return;
  }

  // Make sure the current angle is inside the endpoints
  currentAngleDeg = constrain(
    currentAngleDeg,
    minAngleDeg,
    maxAngleDeg
  );

  // Choose the correct initial direction
  if (currentAngleDeg >= maxAngleDeg)
  {
    direction = -1;
  }
  else if (currentAngleDeg <= minAngleDeg)
  {
    direction = 1;
  }

  endpointPause = false;
  previousUpdateMs = millis();

  motionRunning = true;

  // Re-send the current position to the PCA9685
  moveServoToAngle(currentAngleDeg);

  Serial.println("Reciprocating motion STARTED.");

  Serial.print("Moving between ");
  Serial.print(minAngleDeg, 2);
  Serial.print(" and ");
  Serial.print(maxAngleDeg, 2);
  Serial.println(" degrees.");
}

void stopMotion()
{
  motionRunning = false;
  endpointPause = false;

  Serial.print("Motion STOPPED at ");
  Serial.print(currentAngleDeg, 2);
  Serial.println(" degrees.");
}

void updateMotion()
{
  unsigned long currentTimeMs = millis();

  // Handle endpoint pause
  if (endpointPause)
  {
    if (currentTimeMs - endpointReachedMs >= endpointPauseMs)
    {
      direction = -direction;
      endpointPause = false;
      previousUpdateMs = currentTimeMs;
    }

    return;
  }

  // Wait until the next update interval
  if (currentTimeMs - previousUpdateMs < UPDATE_INTERVAL_MS)
  {
    return;
  }

  unsigned long elapsedMs =
    currentTimeMs - previousUpdateMs;

  previousUpdateMs = currentTimeMs;

  // Prevent a very large motion jump if execution was delayed
  if (elapsedMs > 100)
  {
    elapsedMs = 100;
  }

  float elapsedSeconds =
    elapsedMs / 1000.0;

  float angleStep =
    speedDegPerSec *
    elapsedSeconds *
    direction;

  currentAngleDeg += angleStep;

  // Check the maximum endpoint
  if (currentAngleDeg >= maxAngleDeg)
  {
    currentAngleDeg = maxAngleDeg;

    endpointPause = true;
    endpointReachedMs = currentTimeMs;
  }

  // Check the minimum endpoint
  else if (currentAngleDeg <= minAngleDeg)
  {
    currentAngleDeg = minAngleDeg;

    endpointPause = true;
    endpointReachedMs = currentTimeMs;
  }

  moveServoToAngle(currentAngleDeg);

  Serial.print("Angle: ");
  Serial.println(currentAngleDeg, 2);
}

// ============================================================
// SERVO OUTPUT
// ============================================================

void moveServoToAngle(float angleDeg)
{
  float halfRangeDeg =
    SERVO_MECHANICAL_RANGE_DEG / 2.0;

  angleDeg = constrain(
    angleDeg,
    -halfRangeDeg,
    halfRangeDeg
  );

  float pulseWidthUs;

  if (angleDeg >= 0.0)
  {
    pulseWidthUs =
      SERVO_CENTER_US +
      (
        angleDeg / halfRangeDeg
      ) *
      (
        SERVO_MAX_US -
        SERVO_CENTER_US
      );
  }
  else
  {
    pulseWidthUs =
      SERVO_CENTER_US -
      (
        (-angleDeg) / halfRangeDeg
      ) *
      (
        SERVO_CENTER_US -
        SERVO_MIN_US
      );
  }

  pwm.writeMicroseconds(
    SERVO_CHANNEL,
    (uint16_t)pulseWidthUs
  );
}

float constrainServoAngle(float angleDeg)
{
  float halfRangeDeg =
    SERVO_MECHANICAL_RANGE_DEG / 2.0;

  return constrain(
    angleDeg,
    -halfRangeDeg,
    halfRangeDeg
  );
}

// ============================================================
// SERIAL INPUT
// ============================================================

void readSerial()
{
  while (Serial.available() > 0)
  {
    char receivedChar = Serial.read();

    // Process the command when Enter is pressed
    if (
      receivedChar == '\n' ||
      receivedChar == '\r'
    )
    {
      if (serialBufferIndex > 0)
      {
        serialBuffer[serialBufferIndex] = '\0';

        processCommand(serialBuffer);

        serialBufferIndex = 0;
      }
    }

    // Handle backspace
    else if (
      receivedChar == '\b' ||
      receivedChar == 127
    )
    {
      if (serialBufferIndex > 0)
      {
        serialBufferIndex--;
      }
    }

    // Store normal characters
    else
    {
      if (
        serialBufferIndex <
        SERIAL_BUFFER_SIZE - 1
      )
      {
        serialBuffer[serialBufferIndex] =
          receivedChar;

        serialBufferIndex++;
      }
      else
      {
        serialBufferIndex = 0;

        Serial.println(
          "ERROR: Command was too long."
        );
      }
    }
  }
}

// ============================================================
// COMMAND PROCESSING
// ============================================================

void processCommand(char *command)
{
  command = trimWhitespace(command);

  convertToUppercase(command);

  // Separate the command from its argument
  char *argument = strchr(command, ' ');

  if (argument != NULL)
  {
    *argument = '\0';

    argument++;

    argument = trimWhitespace(argument);
  }

  // ----------------------------------------------------------
  // START
  // ----------------------------------------------------------

  if (
    strcmp(command, "START") == 0 ||
    strcmp(command, "RUN") == 0
  )
  {
    startMotion();
  }

  // ----------------------------------------------------------
  // STOP
  // ----------------------------------------------------------

  else if (strcmp(command, "STOP") == 0)
  {
    stopMotion();
  }

  // ----------------------------------------------------------
  // ANGLE
  //
  // ANGLE 30 creates endpoints of -30 and +30 degrees.
  // ----------------------------------------------------------

  else if (strcmp(command, "ANGLE") == 0)
  {
    if (
      argument == NULL ||
      argument[0] == '\0'
    )
    {
      Serial.println(
        "ERROR: Enter an angle."
      );

      Serial.println(
        "Example: ANGLE 30"
      );

      return;
    }

    float requestedAngle =
      atof(argument);

    if (requestedAngle < 0.0)
    {
      requestedAngle =
        -requestedAngle;
    }

    float maximumAllowedAngle =
      SERVO_MECHANICAL_RANGE_DEG / 2.0;

    if (
      requestedAngle <= 0.0 ||
      requestedAngle >
      maximumAllowedAngle
    )
    {
      Serial.print(
        "ERROR: Angle must be between 0 and "
      );

      Serial.print(
        maximumAllowedAngle,
        1
      );

      Serial.println(" degrees.");

      return;
    }

    minAngleDeg = -requestedAngle;
    maxAngleDeg = requestedAngle;

    // Keep the current position within the new limits
    currentAngleDeg = constrain(
      currentAngleDeg,
      minAngleDeg,
      maxAngleDeg
    );

    moveServoToAngle(currentAngleDeg);

    endpointPause = false;
    previousUpdateMs = millis();

    Serial.print("Motion range set to ");
    Serial.print(minAngleDeg, 2);
    Serial.print(" through ");
    Serial.print(maxAngleDeg, 2);
    Serial.println(" degrees.");
  }

  // ----------------------------------------------------------
  // MIN
  //
  // Example: MIN -20
  // ----------------------------------------------------------

  else if (strcmp(command, "MIN") == 0)
  {
    if (
      argument == NULL ||
      argument[0] == '\0'
    )
    {
      Serial.println(
        "ERROR: Enter a minimum angle."
      );

      Serial.println(
        "Example: MIN -20"
      );

      return;
    }

    float requestedMinimum =
      constrainServoAngle(
        atof(argument)
      );

    if (
      requestedMinimum >=
      maxAngleDeg
    )
    {
      Serial.println(
        "ERROR: MIN must be below MAX."
      );

      return;
    }

    minAngleDeg = requestedMinimum;

    currentAngleDeg = constrain(
      currentAngleDeg,
      minAngleDeg,
      maxAngleDeg
    );

    moveServoToAngle(currentAngleDeg);

    endpointPause = false;
    previousUpdateMs = millis();

    Serial.print(
      "Minimum angle set to "
    );

    Serial.print(
      minAngleDeg,
      2
    );

    Serial.println(" degrees.");
  }

  // ----------------------------------------------------------
  // MAX
  //
  // Example: MAX 40
  // ----------------------------------------------------------

  else if (strcmp(command, "MAX") == 0)
  {
    if (
      argument == NULL ||
      argument[0] == '\0'
    )
    {
      Serial.println(
        "ERROR: Enter a maximum angle."
      );

      Serial.println(
        "Example: MAX 40"
      );

      return;
    }

    float requestedMaximum =
      constrainServoAngle(
        atof(argument)
      );

    if (
      requestedMaximum <=
      minAngleDeg
    )
    {
      Serial.println(
        "ERROR: MAX must be above MIN."
      );

      return;
    }

    maxAngleDeg = requestedMaximum;

    currentAngleDeg = constrain(
      currentAngleDeg,
      minAngleDeg,
      maxAngleDeg
    );

    moveServoToAngle(currentAngleDeg);

    endpointPause = false;
    previousUpdateMs = millis();

    Serial.print(
      "Maximum angle set to "
    );

    Serial.print(
      maxAngleDeg,
      2
    );

    Serial.println(" degrees.");
  }

  // ----------------------------------------------------------
  // SPEED
  //
  // Example: SPEED 20
  // ----------------------------------------------------------

  else if (strcmp(command, "SPEED") == 0)
  {
    if (
      argument == NULL ||
      argument[0] == '\0'
    )
    {
      Serial.println(
        "ERROR: Enter a speed."
      );

      Serial.println(
        "Example: SPEED 20"
      );

      return;
    }

    float requestedSpeed =
      atof(argument);

    if (requestedSpeed <= 0.0)
    {
      Serial.println(
        "ERROR: Speed must be above zero."
      );

      return;
    }

    speedDegPerSec =
      requestedSpeed;

    previousUpdateMs = millis();

    Serial.print("Speed set to ");
    Serial.print(speedDegPerSec, 2);
    Serial.println(" degrees per second.");
  }

  // ----------------------------------------------------------
  // PAUSE
  //
  // Example: PAUSE 250
  // ----------------------------------------------------------

  else if (strcmp(command, "PAUSE") == 0)
  {
    if (
      argument == NULL ||
      argument[0] == '\0'
    )
    {
      Serial.println(
        "ERROR: Enter a pause time."
      );

      Serial.println(
        "Example: PAUSE 250"
      );

      return;
    }

    long requestedPause =
      atol(argument);

    if (
      requestedPause < 0 ||
      requestedPause > 60000
    )
    {
      Serial.println(
        "ERROR: Pause must be from 0 to 60000 ms."
      );

      return;
    }

    endpointPauseMs =
      (unsigned long)requestedPause;

    Serial.print(
      "Endpoint pause set to "
    );

    Serial.print(
      endpointPauseMs
    );

    Serial.println(" ms.");
  }

  // ----------------------------------------------------------
  // GOTO
  //
  // Stops motion and moves directly to an angle.
  // Example: GOTO 30
  // ----------------------------------------------------------

  else if (
    strcmp(command, "GOTO") == 0 ||
    strcmp(command, "MOVE") == 0
  )
  {
    if (
      argument == NULL ||
      argument[0] == '\0'
    )
    {
      Serial.println(
        "ERROR: Enter a target angle."
      );

      Serial.println(
        "Example: GOTO 30"
      );

      return;
    }

    motionRunning = false;
    endpointPause = false;

    currentAngleDeg =
      constrainServoAngle(
        atof(argument)
      );

    moveServoToAngle(
      currentAngleDeg
    );

    Serial.print(
      "Servo moved to "
    );

    Serial.print(
      currentAngleDeg,
      2
    );

    Serial.println(
      " degrees. Motion is stopped."
    );
  }

  // ----------------------------------------------------------
  // CENTER
  // ----------------------------------------------------------

  else if (
    strcmp(command, "CENTER") == 0 ||
    strcmp(command, "CENTRE") == 0
  )
  {
    motionRunning = false;
    endpointPause = false;

    currentAngleDeg = 0.0;

    moveServoToAngle(
      currentAngleDeg
    );

    Serial.println(
      "Servo centered at 0 degrees."
    );

    Serial.println(
      "Motion is stopped."
    );
  }

  // ----------------------------------------------------------
  // STATUS
  // ----------------------------------------------------------

  else if (strcmp(command, "STATUS") == 0)
  {
    printStatus();
  }

  // ----------------------------------------------------------
  // HELP
  // ----------------------------------------------------------

  else if (
    strcmp(command, "HELP") == 0 ||
    strcmp(command, "?") == 0
  )
  {
    printHelp();
  }

  // ----------------------------------------------------------
  // UNKNOWN COMMAND
  // ----------------------------------------------------------

  else
  {
    Serial.print(
      "Unknown command: "
    );

    Serial.println(command);

    Serial.println(
      "Type HELP for available commands."
    );
  }
}

// ============================================================
// INFORMATION OUTPUT
// ============================================================

void printHelp()
{
  Serial.println("Available commands:");
  Serial.println();

  Serial.println("START");
  Serial.println("  Start reciprocating movement.");

  Serial.println("STOP");
  Serial.println("  Stop at the current angle.");

  Serial.println("ANGLE 30");
  Serial.println("  Set movement from -30 to +30 degrees.");

  Serial.println("MIN -20");
  Serial.println("  Set the minimum movement endpoint.");

  Serial.println("MAX 40");
  Serial.println("  Set the maximum movement endpoint.");

  Serial.println("SPEED 20");
  Serial.println("  Set speed in degrees per second.");

  Serial.println("PAUSE 250");
  Serial.println("  Set endpoint pause in milliseconds.");

  Serial.println("GOTO 30");
  Serial.println("  Stop and move directly to an angle.");

  Serial.println("CENTER");
  Serial.println("  Stop and move to zero degrees.");

  Serial.println("STATUS");
  Serial.println("  Show current settings.");

  Serial.println("HELP");
  Serial.println("  Show this command list.");

  Serial.println();
}

void printStatus()
{
  Serial.println();
  Serial.println("---------- STATUS ----------");

  Serial.print("Motion: ");

  if (motionRunning)
  {
    Serial.println("RUNNING");
  }
  else
  {
    Serial.println("STOPPED");
  }

  Serial.print("Current angle: ");
  Serial.print(currentAngleDeg, 2);
  Serial.println(" degrees");

  Serial.print("Minimum angle: ");
  Serial.print(minAngleDeg, 2);
  Serial.println(" degrees");

  Serial.print("Maximum angle: ");
  Serial.print(maxAngleDeg, 2);
  Serial.println(" degrees");

  Serial.print("Speed: ");
  Serial.print(speedDegPerSec, 2);
  Serial.println(" degrees/second");

  Serial.print("Endpoint pause: ");
  Serial.print(endpointPauseMs);
  Serial.println(" ms");

  Serial.print("Direction: ");

  if (direction > 0)
  {
    Serial.println("Increasing angle");
  }
  else
  {
    Serial.println("Decreasing angle");
  }

  Serial.println("----------------------------");
  Serial.println();
}

// ============================================================
// STRING UTILITIES
// ============================================================

char *trimWhitespace(char *text)
{
  while (
    *text != '\0' &&
    isspace((unsigned char)*text)
  )
  {
    text++;
  }

  if (*text == '\0')
  {
    return text;
  }

  char *end =
    text + strlen(text) - 1;

  while (
    end > text &&
    isspace((unsigned char)*end)
  )
  {
    end--;
  }

  *(end + 1) = '\0';

  return text;
}

void convertToUppercase(char *text)
{
  while (*text != '\0')
  {
    *text =
      toupper((unsigned char)*text);

    text++;
  }
}
