#include <avdweb_AnalogReadFast.h>
#include <CytronMotorDriver.h>

// Configure the motor driver
CytronMD leftMotor(PWM_PWM, 3, 9);   // PWM 1A = Pin 3, PWM 1B = Pin 9.
CytronMD rightMotor(PWM_PWM, 10, 11); // PWM 2A = Pin 10, PWM 2B = Pin 11.



//Declare pin shortcuts
#define left_IR A1
#define right_IR A2


/*---     VARIABLE DECLARATION      ---*/

// PID variables
float target, error, integral, derivative, pastError, correction;

// motor speed adjustable variables
float leftMotorSpeed, rightMotorSpeed;

// constants to calibrate 

// IR Thresholds, ESSENTIAL FOR MAPPED VALUES
const int left_IR_Threshold = 240; 
const int right_IR_Threshold = 500;
const int left_IR_Minimum = 90;
const int right_IR_Minimum = 310;
int baseSpeed = 160;
float KP = 0.6; // proportional - how sensitive to turn every time
float KI = 0; // integral - how much correction from past error, reducing steady state error over time
float KD = 1; // derivative - how much correction from differences from error - pastError over time


/*---     FUNCTION DECLARATION      ---*/
void pid_loop();
void calibration_code();
void Degrees(int distance);
void Junction();



void setup() {
  // set IRs to INPUT
  pinMode(left_IR, INPUT);
  pinMode(right_IR, INPUT);
  // set digital pin interrupts for encoders (TBC)
  Serial.begin(9600);
}

void loop() {
  int left_IR_value = analogReadFast(left_IR);
  int right_IR_value = analogReadFast(right_IR); 

  // adjusts values between 0 - 100 as both of them have differing thresholds, keeps within same range so PID error translates for both
  int leftAdjusted = map(left_IR_value, left_IR_Minimum, left_IR_Threshold, 0, 100);
  int rightAdjusted = map(right_IR_value, right_IR_Minimum, right_IR_Threshold, 0, 100);
  
  leftAdjusted = constrain(leftAdjusted, 0, 100);
  rightAdjusted = constrain(rightAdjusted, 0, 100);

  // PID adjustments
  error = leftAdjusted - rightAdjusted; // check how far off line
  integral += error; // add current error
  derivative = error - pastError; // finds how much error has changed since last adjustment
  pastError = error; // stores past error
  
  // speed adjustment required
  correction = (KP * error) + (KI * integral) + (KD * derivative);

  Serial.print("Correction: ");
  Serial.print(correction);

  // implements speed adjustment (DOUBLE CHECK THAT ITS NOT FLIPPED (i.e. ITS ACTUALLY baseSpeed - result instead??) ) 
  leftMotorSpeed = baseSpeed - correction;
  rightMotorSpeed = baseSpeed + correction;

  // checks motor speeds being fed
  Serial.print(". Left: ");
  Serial.print(leftMotorSpeed);
  Serial.print("Right: ");
  Serial.println(rightMotorSpeed);

  leftMotor.setSpeed(leftMotorSpeed);
  rightMotor.setSpeed(rightMotorSpeed);


/*
  

*/

}

void pid_loop() {
  int left_IR_value = analogReadFast(left_IR);
  int right_IR_value = analogReadFast(right_IR); 

  // adjusts values between 0 - 100 as both of them have differing thresholds, keeps within same range so PID error translates for both
  int leftAdjusted = map(left_IR_value, left_IR_Minimum, left_IR_Threshold, 0, 100);
  int rightAdjusted = map(right_IR_value, right_IR_Minimum, right_IR_Threshold, 0, 100);
  
  leftAdjusted = constrain(leftAdjusted, 0, 100);
  rightAdjusted = constrain(rightAdjusted, 0, 100);

  // PID adjustments
  error = leftAdjusted - rightAdjusted; // check how far off line
  integral += error; // add current error
  derivative = error - pastError; // finds how much error has changed since last adjustment
  pastError = error; // stores past error
  
  // speed adjustment required
  correction = (KP * error) + (KI * integral) + (KD * derivative);

  Serial.print("Correction: ");
  Serial.print(correction);

  // implements speed adjustment (DOUBLE CHECK THAT ITS NOT FLIPPED (i.e. ITS ACTUALLY baseSpeed - result instead??) ) 
  leftMotorSpeed = baseSpeed - correction;
  rightMotorSpeed = baseSpeed + correction;

  // checks motor speeds being fed
  Serial.print(". Left: ");
  Serial.print(leftMotorSpeed);
  Serial.print("Right: ");
  Serial.println(rightMotorSpeed);

  leftMotor.setSpeed(leftMotorSpeed);
  rightMotor.setSpeed(rightMotorSpeed);

}

void calibration_code() {

  int left_IR_value = analogReadFast(left_IR);
  int right_IR_value = analogReadFast(right_IR); 

  // adjusts values between 0 - 100 as both of them have differing thresholds, keeps within same range so PID error translates for both
  int leftAdjusted = map(left_IR_value, left_IR_Minimum, left_IR_Threshold, 0, 100);
  int rightAdjusted = map(right_IR_value, right_IR_Minimum, right_IR_Threshold, 0, 100);
  
  leftAdjusted = constrain(leftAdjusted, 0, 100);
  rightAdjusted = constrain(rightAdjusted, 0, 100);


  
  bool leftIsBlack = left_IR_value > left_IR_Threshold;
  bool rightIsBlack = right_IR_value > right_IR_Threshold;
  Serial.print("Left: ");
  Serial.print(left_IR_value);
  Serial.print("Left adjusted: ");
  Serial.print(leftAdjusted);
   if (left_IR_value > left_IR_Threshold) { // checks that the current IR value is above the measured threshold which determines black/white
    Serial.print("Black");
  } else {
    Serial.print("White");
  }
  Serial.print("  Right: ");
  Serial.print(right_IR_value);
  Serial.print("Right adjusted: ");
  Serial.print(rightAdjusted);
  if (right_IR_value > right_IR_Threshold) {
    Serial.println("Right: Black");
  } else {
    Serial.println("Right: White");
  }

}

// function to track line for certain distance specified
void Degrees(int distance) {
  Serial.println("test");
}

// checks if both IRs detect black
void Junction() {
  if ((analogRead(left_IR) > left_IR_Threshold) && (analogReadFast(right_IR) > right_IR_Threshold)) {
    return true;
  }
}
/*
white:

Left: 47  Right: 51
Left: 47  Right: 51
Left: 47  Right: 51
Left: 47  Right: 51
Left: 47  Right: 51

Left: 209  Right: 485
Left: 208  Right: 486
Left: 209  Right: 485
Left: 209  Right: 485

black:

Left: 300  Right: 560
Left: 300  Right: 560
Left: 299  Right: 560

Left: 273  Right: 340
Left: 273  Right: 340
Left: 273  Right: 339
Left: 273  Right: 339

Left: 319  Right: 508
Left: 319  Right: 508
Left: 319  Right: 508
Left: 319  Right: 508
Left: 319  Right: 508

etc:

Left: 193  Right: 505
Left: 193  Right: 505
Left: 193  Right: 505
*/
