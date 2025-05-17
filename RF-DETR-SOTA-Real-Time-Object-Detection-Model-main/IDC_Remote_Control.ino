#include <PWMServo.h>


#include <avdweb_AnalogReadFast.h>
#include <CytronMotorDriver.h>



// Configure the motor driver
CytronMD leftMotor(PWM_PWM, 3, 9);   // PWM 1A = Pin 3, PWM 1B = Pin 9.
CytronMD rightMotor(PWM_PWM, 10, 11); // PWM 2A = Pin 10, PWM 2B = Pin 11.


PWMServo clawServo;
PWMServo liftServo;
PWMServo cameraServo;

//Declare pin shortcuts
#define left_IR A1
#define right_IR A2
#define leftmost_IR A3
#define clawServoPin 6
#define liftServoPin 13
#define cameraServoPin 12

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
void clawPosition(int position, int reduceSpeed)


void setup() {


}



void loop() {
  
}


/*--- Basic Movement Functions ---*/

void turnLeft(unsigned long duration) {
  //     turnLeft(480); // 90 degree left turn
  unsigned long startTime = millis(); // start time

  while (millis() - startTime < duration) { // while time taken less than duration
    leftMotor.setSpeed(-180);
    rightMotor.setSpeed(180);
  }

  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("completed turn");

}

void turnRight(unsigned long duration) {
  // turnRight(450); 90 degree right turn
  unsigned long startTime = millis(); // start time

  while (millis() - startTime < duration) { // while time taken less than duration
    leftMotor.setSpeed(180);
    rightMotor.setSpeed(-180);
  }

  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped turning right after duration.");

}

void moveForward(unsigned long duration) {
   unsigned long startTime = millis(); // start time

  while (millis() - startTime < duration) { // while time taken less than duration
    leftMotor.setSpeed(180);
    rightMotor.setSpeed(190);
  }

  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped moving straight after duration.");

}

void moveBackward(unsigned long duration) {
   unsigned long startTime = millis(); // start time

  while (millis() - startTime < duration) { // while time taken less than duration
    leftMotor.setSpeed(-180);
    rightMotor.setSpeed(-180);
  }

  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped following the line after duration.");

}

void clawPosition(int position, int reduceSpeed) {
  int currentPosition = clawServo.read(); // Get the current position of the claw servo
  if (currentPosition < position) {
    // Move to the target position incrementally
    for (int i = currentPosition; i <= position; i++) {
      clawServo.write(i);
      delay(reduceSpeed);
    }
  } else {
    // Move to the target position decrementally
    for (int i = currentPosition; i >= position; i--) {
      clawServo.write(i);
      delay(reduceSpeed); 
    }
  }
}

void liftPosition(int position, int reduceSpeed) {
  int currentPosition = liftServo.read(); // Get the current position of the lift servo
  Serial.print("current position: ");

  while (liftServo.read() < position) {
    // Move to the target position incrementally
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