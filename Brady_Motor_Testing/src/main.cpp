#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#define OUTPUT_1 PB_1
#define OUTPUT_0 PB_0
#define READ_PIN PA_0

#define FREQ 100
#define SPEED 1100
#define SPEED_THRESH 600
#define SPEED_MULT 1.4
#define LOOP_DELAY 100
#define RUN_DELAY 3000

volatile int speed_0 = 50;

void setup() {
  pinMode(OUTPUT_1,OUTPUT);
  pinMode(OUTPUT_0,OUTPUT);
  // pinMode(READ_PIN, INPUT);

  
}

void loop() {

  // int speed_mult = analogRead(READ_PIN) / 100;
  // int speed_1 = speed_0 * speed_mult;

   
  if(speed_0 < SPEED_THRESH){
    pwm_start(OUTPUT_1, FREQ, speed_0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(OUTPUT_0, FREQ, speed_0, RESOLUTION_12B_COMPARE_FORMAT);
    speed_0+=50;
  }
  // else if(speed_0 == SPEED_THRESH){
  //   delay(RUN_DELAY);
  //   pwm_start(OUTPUT_1, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
  //   pwm_start(OUTPUT_0, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
  // }
  else if(speed_0 < SPEED){
    int speed_1 = SPEED_MULT * speed_0;
    pwm_start(OUTPUT_1, FREQ, speed_1, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(OUTPUT_0, FREQ, speed_0, RESOLUTION_12B_COMPARE_FORMAT);
    speed_0 += 50;

  }
  else if(speed_0 == SPEED){
    delay(RUN_DELAY);
    pwm_start(OUTPUT_1, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(OUTPUT_0, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
  }
  
  delay(LOOP_DELAY);

}