#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible
#define RIGHT_WHEEL PB_9
#define RIGHT_WHEEL_BACKWARDS PB_8
#define LEFT_WHEEL PB_7
#define LEFT_WHEEL_BACKWARDS PB_6
#define MOTORFREQ 100
#define POT_RIGHT PA_0
#define POT_LEFT PA_1
#define REFLECT_SENSOR_LEFT PA_4
#define REFLECT_SENSOR_RIGHT PA_5
#define TAPE_THRESH 100
#define POT_KP PA_2
#define STATE_1_SPEED_ADJUST 350
#define ROTOR PB_1
#define SERVO_SLOPE PA_8
#define SERVO_FLAP PA_9
#define SERVO_FREQ 50
#define REFLECT_CAN_1 PA_6
#define REFLECT_CAN_2 PA_7
#define CAN_REFLECT_THRES 80
#define DROP_OFF_SWITCH PA3 

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
volatile bool both_off = false;


TwoWire Wire1(PB11, PB10);// Use STM32 I2C2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

// First OLED setup: SDA to B7 and SCK to B6
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void dropOff_interrupt();
int slopeServoSpeed(float angle);
void slopeServoPosition(int position);
int flapServoSpeed(float angle);
void flapPosition(int position);

void setup()
{
  pinMode(REFLECT_SENSOR_LEFT, INPUT);
  pinMode(REFLECT_SENSOR_RIGHT, INPUT);
  pinMode(RIGHT_WHEEL, OUTPUT);
  pinMode(RIGHT_WHEEL_BACKWARDS, OUTPUT);
  pinMode(LEFT_WHEEL, OUTPUT);
  pinMode(LEFT_WHEEL_BACKWARDS, OUTPUT);
  pinMode(POT_RIGHT, INPUT);
  pinMode(POT_LEFT, INPUT);
  pinMode(POT_KP, INPUT);
  pinMode(ROTOR, OUTPUT);
  pinMode(SERVO_SLOPE, OUTPUT);
  pinMode(REFLECT_CAN_1, INPUT);
  pinMode(REFLECT_CAN_2, INPUT);
  pinMode(DROP_OFF_SWITCH, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(DROP_OFF_SWITCH), dropOff_interrupt, RISING);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("You got this Angel! <3");
  display.display();
  flapPosition(0);
  slopeServoPosition(1);
}

void loop() {

  // flap motor speed
  pwm_start(ROTOR, MOTORFREQ, 2222, RESOLUTION_12B_COMPARE_FORMAT);

  int pot_right = analogRead(POT_RIGHT);
  int pot_left = analogRead(POT_LEFT);
  int speed_r = 4 * pot_right;
  int speed_l = 4 * pot_left;
  int kp = analogRead(POT_KP);
  int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
  int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);

  // Using the OLED screen to display some variables
  if (loopcount > 100){
    flapPosition(0);
    display.clearDisplay();
    // checking tape following sensors
    display.setCursor(0,0);
    display.print("Reflec L: ");
    display.println(reflectance_left);
    display.print("Reflec R: ");
    display.println(reflectance_right);
    display.print("Speed L:");
    display.println(speed_l);
    display.print("Speed R:");
    display.println(speed_r);
    display.print("kp: ");
    display.println(kp);
    display.display();
    loopcount = 0;
  }

  // int speed_right = analogRead(POT_RIGHT);
  // int speed_left = analogRead(POT_LEFT);
  
  // going straight, both are on the tape
  if (reflectance_right > TAPE_THRESH && reflectance_left > TAPE_THRESH){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 750, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 1150, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = 0;
    both_off = false;
  }

  // right off, left on. Boost right wheel.
  else if (reflectance_right <= TAPE_THRESH && reflectance_left > TAPE_THRESH){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
	  pwm_start(RIGHT_WHEEL, MOTORFREQ, 750 + 600, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 1150 - 600, RESOLUTION_12B_COMPARE_FORMAT);
	  error_state = 1;
    both_off = false;
  }

  // left off, right on. Boost left wheel. 
  else if(reflectance_left <= TAPE_THRESH && reflectance_right > TAPE_THRESH){
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 750 - 600, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 1150 + 700, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = -1;
    both_off = false;
  }

  // both off, left was last on. Boost right wheel. 
  else if (reflectance_right <= TAPE_THRESH && reflectance_left <= TAPE_THRESH && last_error_state == 1){
    pwm_start(LEFT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 2200, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 1400, RESOLUTION_12B_COMPARE_FORMAT);
    both_off = true;
  }

  // both off, right was last on. Boost left wheel.
  else if (reflectance_right <= TAPE_THRESH && reflectance_left <= TAPE_THRESH && last_error_state == -1){
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 2200, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 1600, RESOLUTION_12B_COMPARE_FORMAT);
    both_off = true;
  }

  last_error_state = error_state;

  // Code to control servo that guides cans to their holster.
  if (!both_off){
    int can1 = analogRead(REFLECT_CAN_1);
    int can2 = analogRead(REFLECT_CAN_2);

    if (!can1_in && !can2_in){
      slopeServoPosition(1);
    }

    if (can1 <= CAN_REFLECT_THRES && !can1_in){
      delay(100);
      slopeServoPosition(2);
      can1_in = true;
    }

    if (can2 <= CAN_REFLECT_THRES && can1_in && !can2_in){
      delay(100);
      slopeServoPosition(3);
      can2_in = true;
    } 
  }

  loopcount++;  
}

// **************************************** Method section *********************************************

// ********** Converting an angle to a servo speed **********
int slopeServoSpeed(float angle) {
  return (int) angle * (2000.0/180.0) + 400.0;
}

int flapServoSpeed(float angle){
  return (int) angle * (2000.0/180.0) + 700.0;
}

// ********** Sending specific angle/speed to the servo slope for the holsters **********
void slopeServoPosition(int position){
  if (position == 1){
    servo_speed = slopeServoSpeed(170.0);
    servo_angle = 170.0;
    pwm_start(SERVO_SLOPE, SERVO_FREQ, servo_speed, MICROSEC_COMPARE_FORMAT);
  }
  else if (position == 2){
    servo_speed = slopeServoSpeed(145.0);
    servo_angle = 145.0;
    pwm_start(SERVO_SLOPE, SERVO_FREQ, servo_speed, MICROSEC_COMPARE_FORMAT);
  }
  else if (position == 3){
    servo_speed = slopeServoSpeed(125.0);
    servo_angle = 125.0;
    pwm_start(SERVO_SLOPE, SERVO_FREQ, servo_speed, MICROSEC_COMPARE_FORMAT);
  }
}

// ********** Sending specific angle/speed to the flap servo to hold/drop the cans **********
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

// ********** DROP OFF method when microswitch gets pressed at drop off **********
void dropOff_interrupt(){
  // turning the flap motor and wheels off
  if(digitalRead(DROP_OFF_SWITCH) == LOW && (can1_in || can2_in)){
    pwm_start(ROTOR, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL_BACKWARDS, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL_BACKWARDS, MOTORFREQ, 0 , RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    // opening the flap aka DROPPING 3 CANSSSS!! :D
    delay(222);
    flapPosition(1);
    slopeServoPosition(1);
  }
}



