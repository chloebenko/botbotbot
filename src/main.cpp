
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define R_MOTOR PA_8
#define L_MOTOR PA_9

#define R_SENSOR PA0
#define L_SENSOR PA1


#define FREQ 100
#define L_BASE_SPEED 800
#define SPEED_THRESH 600
#define SPEED_MULT 1.3
#define LOOP_DELAY 100
#define RUN_DELAY 3000

#define RIGHT_ON 0
#define LEFT_ON 1
#define BOTH_OFF_MULT 3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 	-1 // This display does not have a reset pin accessible
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile int R_BASE_SPEED = SPEED_MULT * L_BASE_SPEED;

volatile int offTapeThresh = 70;
volatile int oneOffError = 150; 
volatile double bothOffMult;
volatile int lastOn;

volatile int loopCount = 0;

void motorStart(PinName P1, PinName P2, int finalSpeed);

void setup() {

pinMode(R_MOTOR,OUTPUT);
pinMode(L_MOTOR,OUTPUT);

pinMode(L_SENSOR, INPUT);
pinMode(R_SENSOR, INPUT);

display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE); 

motorStart(R_MOTOR, L_MOTOR, SPEED_THRESH);
}



void loop() {
  int rValue = analogRead(R_SENSOR);
  int lValue = analogRead(L_SENSOR);

  // if(loopCount == 100){
  //   display.setCursor(0,0);
  //   display.println(analogRead(L_SENSOR));
  //   display.println(analogRead(R_SENSOR));
  //   display.display();
  //   display.clearDisplay();
  //   loopCount=0;
  // }

  if(rValue > offTapeThresh && lValue > offTapeThresh){
    pwm_start(L_MOTOR, FREQ, L_BASE_SPEED, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(R_MOTOR, FREQ, L_BASE_SPEED, RESOLUTION_12B_COMPARE_FORMAT);
  }

  else if(rValue <= offTapeThresh && lValue > offTapeThresh){
    lastOn = LEFT_ON;

    pwm_start(L_MOTOR, FREQ, L_BASE_SPEED - oneOffError, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(R_MOTOR, FREQ, L_BASE_SPEED + oneOffError, RESOLUTION_12B_COMPARE_FORMAT);
  }

  else if(rValue > offTapeThresh && lValue <= offTapeThresh){
    lastOn = RIGHT_ON;

    pwm_start(L_MOTOR, FREQ, L_BASE_SPEED + oneOffError, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(R_MOTOR, FREQ, L_BASE_SPEED - oneOffError, RESOLUTION_12B_COMPARE_FORMAT);
  }

  else if(rValue <= offTapeThresh && lValue <= offTapeThresh){
    if(lastOn == RIGHT_ON){
      pwm_start(L_MOTOR, FREQ, L_BASE_SPEED + (oneOffError * BOTH_OFF_MULT), RESOLUTION_12B_COMPARE_FORMAT);
      pwm_start(R_MOTOR, FREQ, L_BASE_SPEED - (oneOffError * BOTH_OFF_MULT), RESOLUTION_12B_COMPARE_FORMAT);
    }
    else if(lastOn == LEFT_ON){
      pwm_start(L_MOTOR, FREQ, L_BASE_SPEED - (oneOffError * BOTH_OFF_MULT), RESOLUTION_12B_COMPARE_FORMAT);
      pwm_start(R_MOTOR, FREQ, L_BASE_SPEED + (oneOffError * BOTH_OFF_MULT), RESOLUTION_12B_COMPARE_FORMAT);
    }
    else{
      pwm_start(L_MOTOR, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
      pwm_start(R_MOTOR, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    }
  }
  else{
      pwm_start(L_MOTOR, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
      pwm_start(R_MOTOR, FREQ, 0, RESOLUTION_12B_COMPARE_FORMAT);
    }

  //   loopCount++;
}



void motorStart(PinName P1, PinName P2, int finalSpeed){
  int speed = 0;

  while(speed < finalSpeed){
    pwm_start(P1, FREQ, speed, RESOLUTION_12B_COMPARE_FORMAT);
    pwm_start(P2, FREQ, speed, RESOLUTION_12B_COMPARE_FORMAT);
    speed += 50;
    delay(LOOP_DELAY);
  }

}

