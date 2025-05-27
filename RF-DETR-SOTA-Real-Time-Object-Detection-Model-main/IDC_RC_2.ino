#include <Servo.h>
#include <avdweb_AnalogReadFast.h>
#include <CytronMotorDriver.h>
#include <SoftwareSerial.h>

//Declare button shortcuts
#define FORWARD 'F'
#define BACKWARD 'B'
#define LEFT 'L'
#define RIGHT 'R'
#define CIRCLE 'C'
#define CROSS 'X'
#define TRIANGLE 'T'
#define SQUARE 'S'
#define START 'A'
#define PAUSE 'P'

//Declare pin shortcuts
#define left_IR A1
#define right_IR A2
#define leftmost_IR A3
#define clawServoPin 6
#define liftServoPin 2
#define cameraServoPin 13
#define BT_TX 10
#define BT_RX 9

// === NEW: LED pins ===
#define LED_BANDAGE 7
#define LED_SYRINGE 8
#define LED_GAUZE 9

// Configure the motor driver
CytronMD leftMotor(PWM_PWM, 3, 4);   // PWM 1A = Pin 3, PWM 1B = Pin 9.
CytronMD rightMotor(PWM_PWM, 11, 12); // PWM 2A = Pin 10, PWM 2B = Pin 11.

Servo clawServo;
Servo liftServo;
Servo cameraServo;

// configure Bluetooth 
SoftwareSerial BTSerial(BT_TX, BT_RX); // Maker UNO RX, TX

/*---     VARIABLE DECLARATION      ---*/

// Store parsed data from RPi
String object_type;
String object_name;
float object_width = 0.0;
float object_height = 0.0;
float object_center_x = 0.0;
float object_center_y = 0.0;

// PID variables
float target, error, integral, derivative, pastError, correction;

// motor speed adjustable variables
float leftMotorSpeed, rightMotorSpeed;

// bluetooth variables
boolean BTConnect = false;
char inChar;
char lastCommand = '\0'; // Initialize lastCommand to null character

// NEW: Pi serial buffer
String piBuffer = "";

// miscellaneous variables
unsigned long time_now;
unsigned long start_time;
bool junctionDetect;
bool hardCodedCompletion;

// constants to calibrate 

// IR Thresholds, ESSENTIAL FOR MAPPED VALUES
const int left_IR_Threshold = 630; 
const int right_IR_Threshold = 740;
const int leftmost_IR_Threshold = 600;
const int left_IR_Minimum = 30;
const int right_IR_Minimum = 35;
const int leftmost_IR_Minimum = 50;
int baseSpeed = 100;
float KP = 0.30 ; // proportional - how sensitive to turn every time
float KI = 0.0; // integral - how much correction from past error, reducing steady state error over time
float KD = 1.3; // derivative - how much correction from differences from error - pastError over time

/*---     FUNCTION DECLARATION      ---*/
void turnLeft(unsigned long duration);
void turnRight(unsigned long duration);
void moveForward(unsigned long duration);
void moveBackward(unsigned long duration);
void liftPosition(int position, int reduceSpeed);
void clawPosition(int position, int reduceSpeed);
void executeCommand(char command);

// === NEW: handle Pi response ===
void handlePiResponse(String response) {
  response.trim();
  digitalWrite(LED_BANDAGE, response.indexOf("BANDAGE 0") == -1 ? HIGH : LOW);
  digitalWrite(LED_SYRINGE, response.indexOf("SYRINGE 0") == -1 ? HIGH : LOW);
  digitalWrite(LED_GAUZE,   response.indexOf("GAUZE 0")   == -1 ? HIGH : LOW);
  Serial.print("LEDs updated based on Pi reply: ");
  Serial.println(response);
}

void setup() {
  Serial.begin(9600);  // Set the baud rate for serial communication
  BTSerial.begin(9600);
  liftServo.attach(liftServoPin);
  clawServo.attach(clawServoPin);

  pinMode(LED_BANDAGE, OUTPUT);
  pinMode(LED_SYRINGE, OUTPUT);
  pinMode(LED_GAUZE, OUTPUT);

  Serial.println("setup complete");
}

void loop() {
  if (BTSerial.available()) {
    inChar = BTSerial.read();
    Serial.print("Received command: ");
    Serial.println(inChar);
    executeCommand(inChar);
    delay(10);
  }

  // === NEW: Listen to Pi response
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      handlePiResponse(piBuffer);
      piBuffer = "";
    } else {
      piBuffer += c;
    }
  }
}

/*--- Basic Movement Functions ---*/

void turnLeft(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    leftMotor.setSpeed(-180);
    rightMotor.setSpeed(180);
  }
  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("completed turn");
}

void turnRight(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    leftMotor.setSpeed(180);
    rightMotor.setSpeed(-180);
  }
  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped turning right after duration.");
}

void moveForward(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    leftMotor.setSpeed(180);
    rightMotor.setSpeed(190);
  }
  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped moving straight after duration.");
}

void moveBackward(unsigned long duration) {
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    leftMotor.setSpeed(-180);
    rightMotor.setSpeed(-180);
  }
  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped following the line after duration.");
}

void clawPosition(int position, int reduceSpeed) {
  int currentPosition = clawServo.read();
  if (currentPosition < position) {
    for (int i = currentPosition; i <= position; i++) {
      clawServo.write(i);
      delay(reduceSpeed);
    }
  } else {
    for (int i = currentPosition; i >= position; i--) {
      clawServo.write(i);
      delay(reduceSpeed); 
    }
  }
}

void liftPosition(int position, int reduceSpeed) {
  int currentPosition = liftServo.read();
  Serial.print("current position: ");

  while (liftServo.read() < position) {
    for (int i = currentPosition; i <= position; i++) {
      liftServo.write(i);
      Serial.println(currentPosition);
      delay(reduceSpeed); 
    }
  } 
  
  while (liftServo.read() > position) {
    for (int i = currentPosition; i >= position; i--) {
      liftServo.write(i);
      Serial.println(currentPosition);
      delay(reduceSpeed);
    }
  }
}

void executeCommand(char command) {
  switch (command) {
    case FORWARD:
      leftMotor.setSpeed(255);
      rightMotor.setSpeed(255);
      Serial.println("forward");
      break;
    case BACKWARD:
      leftMotor.setSpeed(-180);
      rightMotor.setSpeed(-180);
      Serial.println("backward");
      break;
    case LEFT:
      leftMotor.setSpeed(-180);
      rightMotor.setSpeed(180);
      break;
    case RIGHT:
      leftMotor.setSpeed(180);
      rightMotor.setSpeed(-180);
      break;
    case CIRCLE:
      liftServo.write(180);
      Serial.print("lift up");
      break;
    case CROSS:
      liftServo.write(0);
      Serial.print("lift down");
      break;
    case TRIANGLE:
      clawServo.write(0);
      Serial.print("claw open");
      break;
    case SQUARE:
      clawServo.write(90);
      Serial.println("claw close");
      Serial.println(clawServo.read());
      break;
    case START:
      Serial.println("#DETECT");
      break;
    case PAUSE:
      break;
    default:
      leftMotor.setSpeed(0);
      rightMotor.setSpeed(0);
      liftServo.write(liftServo.read());
      clawServo.write(clawServo.read());
      Serial.println("defaulting");
      break;
  }
}
