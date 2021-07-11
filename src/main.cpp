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
#define TAPE_RIGHT_THRESH 44
#define TAPE_LEFT_THRESH 50

volatile int lasterror = 0;
volatile int kp = 5;
volatile int kd = 0;
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

  if (loopcount > 5){
    display.clearDisplay();
    int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);
    int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
    int pot_right = analogRead(POT_RIGHT);
    int pot_left = analogRead(POT_LEFT);
    display.setCursor(0,0);
    display.println("Reflec L: ");
    display.println(reflectance_left);
    display.println("Reflec R: ");
    display.println(reflectance_right);
    display.println("Speed L: ");
    display.println(pot_left);
    display.println("Speed R: ");
    display.println(pot_right);
    display.display();
    loopcount = 0;
  }

  int speed_right = analogRead(POT_RIGHT);
  int speed_left = analogRead(POT_LEFT);
  int reflectance_right = analogRead(REFLECT_SENSOR_RIGHT);
  int reflectance_left = analogRead(REFLECT_SENSOR_LEFT);

  int error_right = reflectance_right - TAPE_RIGHT_THRESH;
  int error_left = reflectance_left - TAPE_LEFT_THRESH;

  if (error_right < 0){
    int p_right = kp * -error_right;
    pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right + p_right, RESOLUTION_12B_COMPARE_FORMAT);
  }

  if (error_left < 0){
    int p_left = kp * -error_left;
    pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left + p_left, RESOLUTION_12B_COMPARE_FORMAT);
  }
  // !!!!!BE CAREFUL ABOUT HAVING 2 PWMs ON FOR THE SAME MOTOR!!!!! 

  pwm_start(RIGHT_WHEEL, MOTORFREQ, speed_right, RESOLUTION_12B_COMPARE_FORMAT);
  pwm_start(LEFT_WHEEL, MOTORFREQ, speed_left, RESOLUTION_12B_COMPARE_FORMAT);

  delay(6);

  loopcount++;
}

