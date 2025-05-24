#include <Servo.h>

#include <avdweb_AnalogReadFast.h>
#include <CytronMotorDriver.h>



// Configure the motor driver
CytronMD leftMotor(PWM_PWM, 3, 4);   // PWM 1A = Pin 3, PWM 1B = Pin 9.
CytronMD rightMotor(PWM_PWM, 11, 12); // PWM 2A = Pin 10, PWM 2B = Pin 11.


Servo clawServo;
Servo liftServo;
Servo cameraServo;

//Declare pin shortcuts
#define left_IR A1
#define right_IR A2
#define leftmost_IR A3
#define clawServoPin 6
#define liftServoPin 2
#define cameraServoPin 13

//---     VARIABLE DECLARATION      ---/

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
unsigned long startTime;
bool junctionDetect;
bool hardCodedCompletion;
// constants to calibrate 

// IR Thresholds, ESSENTIAL FOR MAPPED VALUES
const int left_IR_Threshold = 650; 
const int right_IR_Threshold = 800;
const int leftmost_IR_Threshold = 600;
const int left_IR_Minimum = 70;
const int right_IR_Minimum = 40;
const int leftmost_IR_Minimum = 80;
int baseSpeed = 110;
float KP = 0.24 ; // proportional - how sensitive to turn every time
float KI = 0.0; // integral - how much correction from past error, reducing steady state error over time
float KD = 0.9; // derivative - how much correction from differences from error - pastError over time



//---     FUNCTION DECLARATION      ---/
void pid_loop();
void calibration_code();
bool Junction();
bool whiteJunction();
void lineFollowTimed(unsigned long duration);
void lineFollowJunction();
void turnLeft(unsigned long duration);
void turnRight(unsigned long duration);
void moveForward(unsigned long duration);
void moveBackward(unsigned long duration);
void liftPosition(int position, int reduceSpeed);
void clawPosition(int position, int reduceSpeed);
void route1();
void route2();
void route3();
void routeForBox1();
void routeForBox2();
void routeForBox3();
void processCommand(String command);
void scanForObject(String desiredObject);

void setup() {
  // set IRs to INPUT
  pinMode(left_IR, INPUT);
  pinMode(right_IR, INPUT);
  pinMode(leftmost_IR, INPUT);
  liftServo.attach(liftServoPin);
  clawServo.attach(clawServoPin);
  cameraServo.attach(cameraServoPin);
  liftServo.write(0);
  clawServo.write(0); // 90 is close, 0 is open
  cameraServo.write(90);
  junctionDetect = false;
  hardCodedCompletion = false;
  Serial.begin(9600);
  delay(500);
  route3();

}
void loop() {
  // reads from serial port if data is available
  /*
  if (Serial.available() > 0) {
    // Read the incoming data as a string
    String data = Serial.readStringUntil('\n'); // Read until newline character

    // Process the received command
    processCommand(data);
  }
*/

  // calibration_code();
  
  if (!hardCodedCompletion) {
    /*
    // get out of box
    moveForward(850);
    Serial.println("bing bong forward");
    delay(1000);
    */
    // route1();
    
    // liftPosition(0, 10);
    //clawPosition(90, 10);
    hardCodedCompletion = true;
  }

}


//--- Hard-coded Routes ---/

void route1() {
    turnLeft(430);
    Serial.println("left bing bong");
    delay(400);
    junctionDetect = false;
    lineFollowJunction();
    delay(500);
    moveForward(150);
    delay(500);
    // claw logic
    clawPosition(90, 3);
    delay(2000);
    turnLeft(800);
    Serial.println("turn around bang");
    delay(1000);
    junctionDetect = false;
    lineFollowWhiteJunction();
    Serial.println("found tray");
    delay(3000);
    moveForward(300);
    delay(2000);
    // turnRight(400);
    Serial.println("turned to tray");
    delay(2000);
    clawPosition(0, 3);
    delay(2000);
    Serial.println("released claw");
    moveBackward(150);
    delay(2000);
    turnLeft(415);
    Serial.println("moved back and turned toward line");
}

void route2() {
  Serial.println("route 2 start!");
  lineFollowJunction();
  delay(1000);
  moveForward(250);
  delay(1000);
  lineFollowJunction();
  turnLeft(100);
  delay(2000);
  moveForward(250);
  clawPosition(90, 3);
  delay(2000);
  turnLeft(720);
  delay(500);
  moveForward(250);
  delay(500);
  lineFollowWhiteJunction();
  clawPosition(0, 3);
  delay(2000);
  turnLeft(800);

}

void route3() {
  Serial.println("route 3 start!");
  clawPosition(0, 3);
  delay(2000);
  lineFollowJunction();
  moveForward(450);
  delay(1000);
  turnLeft(450);
  delay(2000);
  lineFollowJunction();
  Serial.println("end of route 3");
  routeForBox1();
  /*
  clawPosition(90, 3); //grab ts
  delay(2000);
  turnLeft(600);
  delay(1000);
  lineFollowWhiteJunction();
  moveForward(450);
  delay(2000);
  turnLeft(300);
  clawPosition(0, 3);
  */
  



}

void routeForBox1() {
  Serial.println("route for box 1");
  turnLeft(350);
  delay(1000);
  moveForward(1050);
  delay(1000);
  clawPosition(90, 4);
  delay(1000);
  turnLeft(250);
  moveForward(1700);
  
  
  Serial.println("box 1 complete");
}

void routeForBox2() {
  Serial.println("route for box 2");
  // Add your hardcoded logic for HOTDOG here
}

void routeForBox3() {
  Serial.println("route for box 3");
  // Add your hardcoded logic for SANDWICH here
}




//--- Arduino-Specific Functions -- -/

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
  int leftmost_IR_value = analogReadFast(leftmost_IR);

  // adjusts values between 0 - 100 as both of them have differing thresholds, keeps within same range so PID error translates for both
  int leftAdjusted = map(left_IR_value, left_IR_Minimum, left_IR_Threshold, 0, 100);
  int rightAdjusted = map(right_IR_value, right_IR_Minimum, right_IR_Threshold, 0, 100);
  int leftmostAdjusted = map(leftmost_IR_value, leftmost_IR_Minimum, leftmost_IR_Threshold, 0, 100);

  leftAdjusted = constrain(leftAdjusted, 0, 100);
  rightAdjusted = constrain(rightAdjusted, 0, 100);
  leftmostAdjusted = constrain(leftmostAdjusted, 0, 100);

  Serial.print("Left: ");
  Serial.print(left_IR_value);
  Serial.print(" Left adjusted: ");
  Serial.print(leftAdjusted);
  if (left_IR_value > left_IR_Threshold) {
    Serial.print(" Black");
  } else {
    Serial.print(" White");
  }

  Serial.print(" | Right: ");
  Serial.print(right_IR_value);
  Serial.print(" Right adjusted: ");
  Serial.print(rightAdjusted);
  if (right_IR_value > right_IR_Threshold) {
    Serial.print(" Black");
  } else {
    Serial.print(" White");
  }

  Serial.print(" | Leftmost: ");
  Serial.print(leftmost_IR_value);
  Serial.print(" Leftmost adjusted: ");
  Serial.print(leftmostAdjusted);
  if (leftmost_IR_value > leftmost_IR_Threshold) {
    Serial.println(" Black");
  } else {
    Serial.println(" White");
  }
}


// checks if both IRs detect black
bool Junction() {
  int leftmost = analogReadFast(leftmost_IR);
  int left = analogReadFast(left_IR);
  int right = analogReadFast(right_IR);

  Serial.println("why is it like that");

  // Detects a left-side junction if leftmost sensor sees black
  if (leftmost >= (leftmost_IR_Threshold-50)) {
    return true;
  }
  // Detects if both left and right sensors see black
  if ((left >= (left_IR_Threshold-50)) && (right >= (right_IR_Threshold-50))) {
    return true;
  }
  return false;
}

bool whiteJunction(unsigned long duration) {
  int leftmost = analogReadFast(leftmost_IR);
  int left = analogReadFast(left_IR);
  int right = analogReadFast(right_IR);
  Serial.println("Checking for white junction...");
  // Detects if both left and right sensors see white
  if ((left <= (left_IR_Minimum + 150)) && (right <= (right_IR_Minimum + 150))) {
    if (startTime == 0) {
      startTime = millis(); // Start the timer when the condition is first met
    }
    if (millis() - startTime >= duration) {
      return true; // Return true if the condition is met for the stipulated duration
    }
  } else {
    startTime = 0; // Reset the timer if the condition is not met
  }

  return false; // Return false if the condition is not met for the stipulated duration
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
  junctionDetect = false;
  while (!Junction()) { // If junction not detected, keep following the line
    pid_loop();
    Serial.print("still junctioning...");
  }
    junctionDetect = true;
    leftMotor.setSpeed(0);
    rightMotor.setSpeed(0);

    Serial.println("junction detected!");
  
}

void lineFollowWhiteJunction() {
  junctionDetect = false;
  startTime = 0; // Tracks when the condition was first met
  while (!whiteJunction(1000)) { // If junction not detected, keep following the line
    pid_loop();
    Serial.print("still junctioning...");
  }
    junctionDetect = true;
    leftMotor.setSpeed(0);
    rightMotor.setSpeed(0);
    Serial.println("junction detected!");
  
}
//--- Basic Movement Functions ---/

void turnLeft(unsigned long duration) {
  //     turnLeft(480); // 90 degree left tu`rn
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
    rightMotor.setSpeed(180);
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
  int currentPosition = clawServo.read(); // Get the current position of the lift servo
  Serial.print("current position: ");

  while (clawServo.read() < position) {
    // Move to the target position incrementally
    for (int i = currentPosition; i <= position; i++) {
      clawServo.write(i);
      Serial.println(clawServo.read());
      delay(reduceSpeed); 
    }
  } 
  
  while (clawServo.read() > position) {
    for (int i = currentPosition; i >= position; i--) {
      clawServo.write(i);
      Serial.println(clawServo.read());
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
      Serial.println(liftServo.read());
      delay(reduceSpeed); 
    }
  } 
  
  while (liftServo.read() > position) {
    for (int i = currentPosition; i >= position; i--) {
      liftServo.write(i);
      Serial.println(liftServo.read());
      delay(reduceSpeed);

    }
  }
}
//--- Integration with Raspberry Pi Code Functions ---/

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
  object_type = parts[0];
  object_name = parts[1];
  object_width = parts[2].toFloat();
  object_height = parts[3].toFloat();
  object_center_x = parts[4].toFloat();
  object_center_y = parts[5].toFloat(); 

  // Debugging output
  Serial.println("Parsed Data:");
  Serial.println("Object Type: " + object_type);
  Serial.println("Object Name: " + object_name);
  Serial.print("Width: ");
  Serial.println(object_width);
  Serial.print("Height: ");
  Serial.println(object_height);
  Serial.print("Center X: ");
  Serial.println(object_center_x);
  Serial.print("Center Y: ");
  Serial.println(object_center_y);
}

void scanForObject(String desiredObject) {
  int positions[] = {0, 55, 120}; // Camera positions to scan (in degrees)
  bool objectFound = false;
  int detectedPosition = -1; // Variable to store the index of the detected position

  // Set the timeout for Serial.readStringUntil to 7 seconds
  Serial.setTimeout(8000);

  while (!objectFound) { // Keep scanning until the desired object is found
    Serial.println("Starting a new scan...");

    for (int i = 0; i < 3; i++) {
      // Rotate the camera to the current position
      cameraServo.write(positions[i]);
      Serial.print("Camera rotated to position: ");
      Serial.println(positions[i]);
      delay(1000);

      // Clear the buffer before waiting for new data
      Serial.println("Clearing buffer...");
      while (Serial.available() > 0) {
        Serial.read(); // Discard any leftover data
      }

      // Wait for new data from the Raspberry Pi
      Serial.println("Waiting for command...");
      String data = Serial.readStringUntil('\n'); // Read until newline character

      // Debugging output for received data
      Serial.print("Received data: ");
      Serial.println(data);

      // Process the received command only if valid data was received
      if (data.length() > 0) {
        processCommand(data); // Process the received command

        // Check if the detected object matches the desired object
        if (object_name == desiredObject) {
          objectFound = true;
          detectedPosition = i + 1; // Store the index of the detected position
          Serial.print("Desired object found at position: ");
          Serial.println(positions[i]);
          break; // Exit the loop as the desired object is found
        }
      } else {
        Serial.println("No valid data received.");
      }
    }

    if (!objectFound) {
      Serial.println("Desired object not found in this scan. Retrying...");
    }
  }

  // Make a decision based on the detected position
  Serial.println("Executing hardcoded route for the detected position...");
  if (detectedPosition == 1) {
    routeForBox1();
  } else if (detectedPosition == 2) {
    routeForBox2();
  } else if (detectedPosition == 3) {
    routeForBox3();
  } else {
    Serial.println("Error: Invalid position detected.");
  }
}