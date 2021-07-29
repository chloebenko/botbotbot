#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible
#define RIGHT_WHEEL PB_9
#define RIGHT_WHEEL_BACKWARDS PB_8
// #define LEFT_WHEEL PB_8
#define LEFT_WHEEL PB_7
#define LEFT_WHEEL_BACKWARDS PB_6
// #define MOTOR2_LEFT PA_7
#define MOTORFREQ 100
#define POT_RIGHT PA_0
#define POT_LEFT PA_1
#define REFLECT_SENSOR_LEFT PA_4
#define REFLECT_SENSOR_RIGHT PA_5
#define TAPE_RIGHT_THRESH 100
#define TAPE_LEFT_THRESH 100
#define POT_KP PA_2
#define STATE_1_SPEED_ADJUST 350
#define ROTOR PB_1
#define SERVO_SLOPE PA_8
#define SERVO_FLAP PA_9
#define SERVO_FREQ 50
#define REFLECT_CAN_1 PA_6
#define REFLECT_CAN_2 PA_7
#define CAN_REFLECT_THRES 80

volatile int last_error_state = 0;
volatile int error_state = 0;
volatile int loopcount = 0;
volatile int servo_speed = 400;
volatile float servo_angle = 180.0;
volatile float flap_angle = 0.0;
volatile int flap_speed = 600;
volatile int servo_position = 1;
volatile int flap_position = 0;
volatile bool can1_in = false;
volatile bool can2_in = false;
volatile bool start = true;
volatile int back_test = 0;

TwoWire Wire1(PB11, PB10);// Use STM32 I2C2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

// First OLED setup
// SDA to B7 and SCK to B6
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void handle_interrupt();
int slopeServoSpeed(float angle);
void slopeServoPosition(int position);
int flapServoSpeed(float angle);
void flapPosition(int position);

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

void setup()
{
  pinMode(REFLECT_SENSOR_LEFT, INPUT);
  pinMode(REFLECT_SENSOR_RIGHT, INPUT);
  // pinMode(LEFT_WHEEL, OUTPUT);
  pinMode(RIGHT_WHEEL, OUTPUT);
  pinMode(RIGHT_WHEEL_BACKWARDS, OUTPUT);
  pinMode(LEFT_WHEEL, OUTPUT);
  pinMode(LEFT_WHEEL_BACKWARDS, OUTPUT);
  // pinMode(MOTOR2_LEFT, OUTPUT);
  pinMode(POT_RIGHT, INPUT);
  pinMode(POT_LEFT, INPUT);
  pinMode(POT_KP, INPUT);
  pinMode(ROTOR, OUTPUT);
  pinMode(SERVO_SLOPE, OUTPUT);
  pinMode(REFLECT_CAN_1, INPUT);
  pinMode(REFLECT_CAN_2, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 
  // Displays Adafruit logo by default. call clearDisplay immediately if you don't want this.
  //display.display();
  display.clearDisplay();
  delay(500);
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("You got this Angel! <3");
  display.display();
  // value after motorfreq determines the dutycycle
  // ex: 2000 gives around 50% duty cycle (2000/4096(max BP value))
  flapPosition(0);
  slopeServoPosition(1);
}

void loop() {

  /* if (start){
    flapPosition(0);
    slopeServoPosition(1);
    start = false;
  } */

  
  if (loopcount > 100){
    display.clearDisplay();
    int pot_kp = analogRead(POT_KP);
    int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);
    int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
    int pot_right = analogRead(POT_RIGHT);
    int pot_left = analogRead(POT_LEFT);
    display.setCursor(0,0);
    display.print("Reflec L: ");
    display.println(reflectance_left);
    display.print("Reflec R: ");
    display.println(reflectance_right);
    // display.print("Thres L: ");
    display.print("Speed L:");
    display.println(pot_left);
    // display.print("Thres R: ");
    display.print("Speed R:");
    display.println(pot_right);
    display.print("kp: ");
    display.println(pot_kp);
    display.display();
    loopcount = 0;
  }

  // int speed_right = analogRead(POT_RIGHT);
  // int speed_left = analogRead(POT_LEFT);
  int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
  int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);
  int kp = analogRead(POT_KP);
  int speed_right = analogRead(POT_RIGHT);
  int speed_left = analogRead(POT_LEFT);
  int thres_right = TAPE_RIGHT_THRESH;
  int thres_left = TAPE_LEFT_THRESH;

  // !!!!!BE CAREFUL ABOUT HAVING 2 PWMs ON FOR THE SAME MOTOR!!!!! 

  // going straight, both are on the tape
  if (reflectance_right > thres_left && reflectance_left > thres_left){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = 0;
  }

  // right off, left on. Boost right wheel.
  else if (reflectance_right <= thres_right && reflectance_left > thres_left){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
	  pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right + STATE_1_SPEED_ADJUST, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left - STATE_1_SPEED_ADJUST, RESOLUTION_12B_COMPARE_FORMAT);
	  error_state = 1;
  }

  // left off, right on. Boost left wheel. 
  else if(reflectance_left <= thres_left && reflectance_right > thres_right){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right - STATE_1_SPEED_ADJUST, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left + STATE_1_SPEED_ADJUST, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = -1;
  }

  // both off, left was last on. Boost right wheel. 
  else if (reflectance_right <= thres_right && reflectance_left <= thres_left && last_error_state == 1){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right + kp, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
  }

  // both off, right was last on
  else if (reflectance_right <= thres_right && reflectance_left <= thres_left && last_error_state == -1){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT); 
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left + kp, RESOLUTION_12B_COMPARE_FORMAT);
  }

  last_error_state = error_state;

  delay(11);

  int can1 = analogRead(REFLECT_CAN_1);
  int can2 = analogRead(REFLECT_CAN_2);

  if (!can1_in && !can2_in){
    slopeServoPosition(1);
  }

  if (can1 <= CAN_REFLECT_THRES && !can1_in){
    slopeServoPosition(2);
    can1_in = true;
  }

  if (can2 <= CAN_REFLECT_THRES && can1_in && !can2_in){
    slopeServoPosition(3);
    can2_in = true;
  } 

  // ********************** TESTING ROLLING BACKWARDS ********************

  /* if (back_test == 0){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left, RESOLUTION_12B_COMPARE_FORMAT);
  }

  else if (back_test == 1){
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, speed_right, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left, RESOLUTION_12B_COMPARE_FORMAT);
  }

  else if (back_test == 2){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, speed_left, RESOLUTION_12B_COMPARE_FORMAT);
  }

  delay(5000);

  back_test++;

  if (back_test >=3){
    back_test = 0;
  } */

  
  /* int can1 = analogRead(REFLECT_CAN_1);
  int can2 = analogRead(REFLECT_CAN_2);
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Can 1: ");
  display.println(can1);
  display.print("Can 2: ");
  display.println(can2);
  display.display(); */

  /* if (can1_in && can2_in){
    delay(3000);
    flapPosition(1);
  } */

  loopcount++;  
}

// **************************************** Method section *********************************************

// ********** Converting an angle to a servo speed 
int slopeServoSpeed(float angle) {
  return (int) angle * (2000.0/180.0) + 400.0;
}

int flapServoSpeed(float angle){
  return (int) angle * (2000.0/180.0) + 700.0;
}

// ********** Sending specific angle/speed to my servo slope for the holsters
void slopeServoPosition(int position){
  if (position == 1){
    servo_speed = slopeServoSpeed(180.0);
    servo_angle = 180.0;
    pwm_start(SERVO_SLOPE, SERVO_FREQ, servo_speed, MICROSEC_COMPARE_FORMAT);
  }
  else if (position == 2){
    servo_speed = slopeServoSpeed(160.0);
    servo_angle = 160.0;
    pwm_start(SERVO_SLOPE, SERVO_FREQ, servo_speed, MICROSEC_COMPARE_FORMAT);
  }
  else if (position == 3){
    servo_speed = slopeServoSpeed(142.0);
    servo_angle = 142.0;
    pwm_start(SERVO_SLOPE, SERVO_FREQ, servo_speed, MICROSEC_COMPARE_FORMAT);
  }
}

// ********** Sending specific angle/speed to my flap servo to hold/drop the holsters
void flapPosition(int position){
  if (position == 1){
    flap_speed = flapServoSpeed(0.0);
    pwm_start(SERVO_FLAP, SERVO_FREQ, flap_speed, MICROSEC_COMPARE_FORMAT);
  }
  else if (position == 0){
    flap_speed = flapServoSpeed(90.0);
    pwm_start(SERVO_FLAP, SERVO_FREQ, flap_speed, MICROSEC_COMPARE_FORMAT);
  }
}



