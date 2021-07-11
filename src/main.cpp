#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible
#define RIGHT_WHEEL PB_9
// #define LEFT_WHEEL PB_8
#define LEFT_WHEEL PA_6
// #define MOTOR2_LEFT PA_7
#define MOTORFREQ 200
#define POT_RIGHT PA_0
#define POT_LEFT PA_1
#define REFLECT_SENSOR_LEFT PA_4
#define REFLECT_SENSOR_RIGHT PA_5
#define TAPE_RIGHT_THRESH 50
#define TAPE_LEFT_THRESH 50
#define POT_KP PA_2
#define TAPE_ERROR_THRESH 150

volatile int last_error_state = 0;
volatile int error_state = 0;
volatile int loopcount = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void handle_interrupt();

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

void setup()
{
  pinMode(REFLECT_SENSOR_LEFT, INPUT);
  pinMode(REFLECT_SENSOR_RIGHT, INPUT);
  // pinMode(LEFT_WHEEL, OUTPUT);
  pinMode(RIGHT_WHEEL, OUTPUT);
  pinMode(LEFT_WHEEL, OUTPUT);
  // pinMode(MOTOR2_LEFT, OUTPUT);
  pinMode(POT_RIGHT, INPUT);
  pinMode(POT_LEFT, INPUT);
  pinMode(POT_KP, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 
  // Displays Adafruit logo by default. call clearDisplay immediately if you don't want this.
  display.display();
  delay(2000);

  // Displays "Hello world!" on the screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Yo g");
  display.display();
  // value after motorfreq determines the dutycycle
  // ex: 2000 gives around 50% duty cycle (2000/4096(max BP value))
}

void loop() {

  if (loopcount > 50){
    display.clearDisplay();
    int pot_kp = analogRead(POT_KP);
    int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);
    int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
    int pot_right = analogRead(POT_RIGHT);
    int pot_left = analogRead(POT_LEFT);
    int error_right = reflectance_right - TAPE_RIGHT_THRESH;
    int error_left = reflectance_left - TAPE_LEFT_THRESH;
    display.setCursor(0,0);
    display.print("Reflec L: ");
    display.println(reflectance_left);
    display.print("Reflec R: ");
    display.println(reflectance_right);
    display.print("Speed L: ");
    display.println(pot_left);
    display.print("Speed R: ");
    display.println(pot_right);
    display.print("kp: ");
    display.println(pot_kp);
    display.print("error right: ");
    display.println(error_right);
    display.print("error left: ");
    display.println(error_left);
    display.display();
    loopcount = 0;
  }

  int speed_right = analogRead(POT_RIGHT);
  int speed_left = analogRead(POT_LEFT);
  int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
  int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);
  int kp = analogRead(POT_KP);

  int error_right = reflectance_right - TAPE_RIGHT_THRESH;
  int error_left = reflectance_left - TAPE_LEFT_THRESH;

  // !!!!!BE CAREFUL ABOUT HAVING 2 PWMs ON FOR THE SAME MOTOR!!!!! 

  // going straight, both are on the tape
  if (error_right >= TAPE_ERROR_THRESH && error_left >= TAPE_ERROR_THRESH){
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = 0;
  }

  // right off, left on. Boost right wheel.
  else if (error_right < TAPE_ERROR_THRESH && error_left > TAPE_ERROR_THRESH){
	  pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right + 100, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left - 100, RESOLUTION_12B_COMPARE_FORMAT);
	  error_state = 1;
  }

  // left off, right on. Boost left wheel. 
  else if(error_left < TAPE_ERROR_THRESH && error_right > TAPE_ERROR_THRESH){
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right - 100, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left + 100, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = -1;
  }

  // both off, left was last on. Boost right wheel. 
  else if (error_right < TAPE_ERROR_THRESH && error_left < TAPE_ERROR_THRESH && (last_error_state == 1 || last_error_state == 5)){
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right + kp, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(LEFT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = 5;
    delay(1000);
  }

  // both off, right was last on
  else if (error_right < TAPE_ERROR_THRESH && error_left < TAPE_ERROR_THRESH && (last_error_state == -1 || last_error_state == -5)){
    pwm_start(RIGHT_WHEEL, MOTORFREQ, 0, RESOLUTION_12B_COMPARE_FORMAT); 
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left + kp, RESOLUTION_12B_COMPARE_FORMAT);
    error_state = -5;
    delay(1000);
  }

  last_error_state = error_state;

  delay(6);

  loopcount++;
}

