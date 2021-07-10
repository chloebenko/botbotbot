#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible
#define MOTOR_RIGHT PB_9
#define MOTOR_LEFT PB_8
#define MOTORFREQ 200
#define REFLEC_SENSOR PA_0
#define POT PA_7

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
  pinMode(REFLEC_SENSOR, INPUT);
  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);
  pinMode(POT, INPUT);

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
  display.clearDisplay();
  int reflectance = analogRead(REFLEC_SENSOR);
  int pot = analogRead(POT);
  display.setCursor(0,0);
  display.println("Reflectance: ");
  display.println(reflectance);
  display.println("Pot: ");
  display.println(pot);
  display.display();
}


// TEST UPLOAD BY CARSON