#include <FastLED.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#define NUM_LEDS 100
#define DATA_PIN 3
#define WIDTH 12
#define HEIGHT 8
#define START_OFFSET 4

int brightness = 255;
bool autoB = true;
int hue = 0;
bool autoHue = true;
bool faded = false;
uint32_t ms;
int minB = 7; // minimum brightness for auto brightness
int buttonUp = 0;
int buttonDown = 0;
int buttonFN = 0;
int pot;

CRGB leds[NUM_LEDS];

uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  if( y & 0x01) {
    // Odd rows run backwards
    uint8_t reverseX = (WIDTH - 1) - x;
    i = (y * WIDTH) + reverseX;
  } else {
    // Even rows run forwards
    i = (y * WIDTH) + x;
  }
  return (i+START_OFFSET);
}

void setup() {
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  Serial.begin(9600);

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     leds[0] = CHSV(0, 255, 255); // red if it hasn't been set
  else
     leds[0] = CHSV(96, 255, 255); // green if all good

  Serial.println(RTC.temperature() / 4.);

  pinMode(6, INPUT); // button up
  pinMode(8, INPUT); // button down
  pinMode(10, INPUT); // button function

  //changeTime(1);
  
  for( byte y = 0; y < HEIGHT; y++) {    // random blue
    for( byte x = 0; x < WIDTH; x++) {
      leds[XY(x, y)]  = CHSV(171, 255, 255);
    }
  }
}

void changeTime(int change) {
  unsigned long t = now() + change;
  RTC.set(t);
  setTime(t);
}

void loop()
{
  ms = millis();
  if(!faded || (ms < 3000)) { // fade in (because millis resets after 50 days only do it once)
    FastLED.setBrightness(scale8(brightness, (ms * 256) / 3000));
    faded = true;
  }
  else {
    buttonUp = digitalRead(6);
    buttonDown = digitalRead(8);
    buttonFN = digitalRead(10);
    int pot = analogRead(A0);
    
    if (buttonUp) {
      changeTime(1+(buttonFN*60*60));
      fadeToBlackBy(leds, NUM_LEDS, 150);
      leds[2] = CRGB::Green;
      delay(200);
    }
    if (buttonDown) {
      changeTime(-1-(buttonFN*60*60));
      fadeToBlackBy(leds, NUM_LEDS, 150);
      leds[2] = CRGB::Red;
      delay(200);
    }
    if (buttonFN) {
      leds[3] = CRGB::Blue;
      autoHue = pot > 1000;
      hue = map(pot, 0, 1000, 0, 255);
    }
    else {
      autoB = pot > 1000;
      brightness = map(pot, 0, 1000, 0, 255);
    }
    
    if(autoB) {
      if(hour() < 8) brightness = minB;
      else if(hour() < 10) brightness = minB+ (((((hour()-8)*3600) + (minute()*60) + second())*(uint32_t)(255-minB))/7200);
      else if(hour() < 21) brightness = 255;
      else brightness = 255 - (((((hour()-21)*3600) + (minute()*60) + second())*(uint32_t)(255-minB))/10800);
    }
    
    FastLED.setBrightness(brightness);
    writeTime(hour(),minute());
  }
  FastLED.show();
}

void writeTime(int hours, int mins) {
  fadeToBlackBy(leds, NUM_LEDS, 15);
  int c1, c2, c3, c4;
  splitNumber(hours,c1,c2);
  splitNumber(mins,c3,c4);

  if(autoHue) hue = ((long)(((long)(hour() >= 8 ? hour()-8 : 16+hour())*3600)+((minute()*60)+second()))*(uint32_t)255)/86400;
  
  writeNumber(0,c1,CHSV(hue, 255, 255));
  writeNumber(1,c2,CHSV(hue+30, 255, 255));
  writeNumber(2,c3,CHSV(hue, 255, 255));
  writeNumber(3,c4,CHSV(hue+30, 255, 255));

  for(int i = 0; i<WIDTH; i++) { // bottom row
    if(second()>=(i*5) && second()<((i*5)+5)) leds[XY(i,7)] = CHSV((int)((i*255)/WIDTH), 200, 255);
  }
  
  FastLED.show();
}

void splitNumber(int num, int &one, int &two) {
  one = (int)(num/10)%10;
  two = num % 10;
}

void writeNumber(int pos, int num, CRGB col) {
  pos *= 3;
  switch(num) {
    case 0:
      for(int i = 1; i<=5; i++) { // sides
        leds[XY(pos,i)] = col;
        leds[XY(pos+2,i)] = col;
      }
      leds[XY(pos+1,0)] = col; // top
      leds[XY(pos+1,6)] = col; // bottom
      break;
    case 1:
      for(int i = 0; i<=6; i++) // the line
        leds[XY(pos+1,i)] = col;
      leds[XY(pos,1)] = col; // sticky out bit
      // bottom
      leds[XY(pos,6)] = col;
      leds[XY(pos+2,6)] = col;
      break;
    case 2:
      leds[XY(pos,1)] = col;
      leds[XY(pos+1,0)] = col;
      leds[XY(pos+2,1)] = col;
      leds[XY(pos+2,2)] = col;
      leds[XY(pos+1,3)] = col;
      leds[XY(pos,4)] = col;
      leds[XY(pos,5)] = col;
      for(int i = 0; i<=2; i++) // bottom
        leds[XY(pos+i,6)] = col;
      break;
    case 3:
      leds[XY(pos,1)] = col;
      leds[XY(pos+1,0)] = col;
      leds[XY(pos+2,1)] = col;
      leds[XY(pos+2,2)] = col;
      leds[XY(pos+1,3)] = col;
      leds[XY(pos+2,4)] = col;
      leds[XY(pos+2,5)] = col;
      leds[XY(pos+1,6)] = col;
      leds[XY(pos,5)] = col;
      break;
    case 4:
      for(int i = 0; i<=3; i++) // side line
        leds[XY(pos,i)] = col;
      for(int i = 0; i<=6; i++) // main line
        leds[XY(pos+2,i)] = col;
      leds[XY(pos+1,3)] = col; // the cross
      break;
    case 5:
      for(int i = 0; i<=2; i++) // top
        leds[XY(pos+i,0)] = col;
      for(int i = 1; i<=2; i++) // left side
        leds[XY(pos,i)] = col;
      leds[XY(pos+1,2)] = col; // top curve
      for(int i = 3; i<=5; i++) // right side
        leds[XY(pos+2,i)] = col;
      leds[XY(pos+1,6)] = col; // bottom
      leds[XY(pos,6)] = col; // bottom
      break;
    case 6:
      leds[XY(pos+1,0)] = col; // top
      leds[XY(pos+2,1)] = col; // top
      for(int i = 1; i<=5; i++) // left side
        leds[XY(pos,i)] = col;
      leds[XY(pos+1,3)] = col; // bottom
      leds[XY(pos+2,4)] = col; // right side
      leds[XY(pos+2,5)] = col; // right side
      leds[XY(pos+1,6)] = col; // bottom
      break;
    case 7:
      for(int i = 0; i<=2; i++) // top
        leds[XY(pos+i,0)] = col;
      for(int i = 1; i<=3; i++) // right
        leds[XY(pos+2,i)] = col;
      for(int i = 4; i<=6; i++) // middle
        leds[XY(pos+1,i)] = col;
      break;
    case 8:
      for(int i = 0; i<=2; i=i+2) { // the sides
        leds[XY(pos+i,1)] = col;
        leds[XY(pos+i,2)] = col;
        leds[XY(pos+i,4)] = col;
        leds[XY(pos+i,5)] = col;
      }
      leds[XY(pos+1,0)] = col; // top
      leds[XY(pos+1,3)] = col; // middle
      leds[XY(pos+1,6)] = col; // bottom
      break;
    case 9:
      for(int i = 1; i<=5; i++) // the line
        leds[XY(pos+2,i)] = col;
      leds[XY(pos+1,0)] = col; // top
      leds[XY(pos,1)] = col; // left
      leds[XY(pos,2)] = col; // left
      leds[XY(pos+1,3)] = col; // bottom of circle
      leds[XY(pos+1,6)] = col; // bottom
      leds[XY(pos,5)] = col; // end of tail
      break;
  }
}

