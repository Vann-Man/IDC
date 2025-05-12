#include <avdweb_AnalogReadFast.h>
#include <CytronMotorDriver.h>

// Configure the motor driver
CytronMD leftMotor(PWM_PWM, 3, 9);   // PWM 1A = Pin 3, PWM 1B = Pin 9.
CytronMD rightMotor(PWM_PWM, 10, 11); // PWM 2A = Pin 10, PWM 2B = Pin 11.



//Declare pin shortcuts
#define left_IR A1
#define right_IR A2


/*---     VARIABLE DECLARATION      ---*/

// Store parsed data from RPi
String object_name = "";
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
// constants to calibrate 

// IR Thresholds, ESSENTIAL FOR MAPPED VALUES
const int left_IR_Threshold = 780; 
const int right_IR_Threshold = 870;
const int left_IR_Minimum = 30;
const int right_IR_Minimum = 46;
int baseSpeed = 180;
float KP = 0.55; // proportional - how sensitive to turn every time
float KI = 0.0; // integral - how much correction from past error, reducing steady state error over time
float KD = 1.3; // derivative - how much correction from differences from error - pastError over time



/*---     FUNCTION DECLARATION      ---*/
void pid_loop();
void calibration_code();
void Degrees(int distance);
bool Junction();



void setup() {
  // set IRs to INPUT
  pinMode(left_IR, INPUT);
  pinMode(right_IR, INPUT);
  Serial.begin(9600);
  while (!Serial) { // Wait for the serial port to connect (for boards like Leonardo)
    ; 
  }
}

void loop() {
  // reads from serial port if data is available
  if (Serial.available() > 0) {
    // Read the incoming data as a string
    String data = Serial.readStringUntil('\n'); // Read until newline character

    // Process the received command
    processCommand(data);
  }
  pid_loop();
}

/*--- Arduino-Specific Functions ---*/

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


// checks if both IRs detect black
bool Junction() {
  if ((analogRead(left_IR) > left_IR_Threshold) && (analogReadFast(right_IR) > right_IR_Threshold)) {
    return true;
  } else {
    return false;
  }
}

void lineFollowTimed(unsigned long duration) {
  unsigned long startTime = millis(); // start time

  while (millis() - startTime < duration) { // while time taken less than duration
    pid_loop();
  }
  leftMotor.setSpeed(0);
  rightMotor.setSpeed(0);
  Serial.println("Stopped following the line after duration.");
}

void lineFollowJunction() {
  if (junctionDetect) {
    // If the junction has already been detected, do nothing
    return;
  if (!Junction()) { // if junction detected and irregardless of whether junction has been detected
    pid_loop();
  } else {
    junctionDetect = true;
    leftMotor.setSpeed(0);
    rightMotor.setSpeed(0);
    Serial.println("junction detected!");
  }
  }
}

/*--- Integration with Raspberry Pi Code Functions ---*/

void processCommand(String command) {

  command.trim();

  String parts[6];
  int currentIndex = 0;
  // Split the command into parts
  for (int i = 0; i < 6; i++) {
    int commaIndex = command.indexOf(',');
    // If no more commas are found, assign the remaining string to parts[i] and break
    if (commaIndex < 0) {
      parts[i] = command; // Assign the remaining string
      break;
    }
    // Extract the part before the comma
    parts[i] = command.substring(0, commaIndex);
    // Remove the extracted part and the comma from the command
    command = command.substring(commaIndex + 1);
  }
  // Assign the parts to variables
  String object_type = parts[0]; // Should be "OBJECT"
  String object_name = parts[1];
  float object_width = parts[2].toFloat();
  float object_height = parts[3].toFloat();
  float object_center_x = parts[4].toFloat();
  float object_center_y = parts[5].toFloat();

  // Debugging output
  Serial.println("Parsed Data:");
  Serial.println("Object Type: " + object_type);
  Serial.println("Object Name: " + object_name);
  Serial.print("Width: ");
  Serial.println(object_width);
  Serial.print("Height: ");
}
