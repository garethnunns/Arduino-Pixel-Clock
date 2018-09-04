#include <FastLED.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#define PIN_POT 0 // potentiometer (input)
#define PIN_DATA 3 // LEDs data pin (output)
#define PIN_BUTTON_UP 6 // button to increment clock (input)
#define PIN_BUTTON_DOWN 8 // button to decrement clock (input)
#define PIN_BUTTON_FN 10 // button to change the function of the inputs
#define START_OFFSET 0 // where the matrix starts
#define STATUS_LED_AUTOB 96 // whether auto brightness is enabled
#define STATUS_LED_AUTOHUE 97 // whether auto hue is enabled
#define STATUS_LED_TIME_ADJUST 98 // indicated which way the time is being adjusted
#define STATUS_LED 99 // shows an error at start up and indicates if function button is enabled

#define NUM_LEDS 100
#define WIDTH 12 // width of the matrix
#define HEIGHT 8 // height of the matrix
#define MATRIX_Y_REFLECT true // whether to reflect the matrix over the Y axis
#define MIN_BRIGHTNESS 7 // minimum brightness for auto brightness
#define MAX_BRIGHTNESS 255 // minimum brightness for auto brightness

uint8_t brightness = 255, hue = 0;
bool autoHue = true, autoB = true faded = false;
uint32_t ms;
CRGB leds[NUM_LEDS];

uint16_t XY (uint8_t x, uint8_t y)
{ 
  uint16_t i;
  if ( (MATRIX_Y_REFLECT ^ y) & 0x01) {
    // Odd rows run backwards
    uint8_t reverseX = (WIDTH - 1) - x;
    i = (y * WIDTH) + reverseX;
  } 
  else {
    // Even rows run forwards
    i = (y * WIDTH) + x;
  }
  
  return (i+START_OFFSET);
}

void setup() {
  FastLED.addLeds<WS2811, PIN_DATA, RGB>(leds, NUM_LEDS);
  Serial.begin(9600);

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     leds[STATUS_LED] = CRGB::Red; // red if it hasn't been set
  else
     leds[STATUS_LED] = CRGB::Green; // green if all good

  Serial.println(RTC.temperature() / 4.);

  pinMode(PIN_BUTTON_UP, INPUT); // button up
  pinMode(PIN_BUTTON_DOWN, INPUT); // button down
  pinMode(PIN_BUTTON_FN, INPUT); // button function

  //changeTime(1);
  
  for( byte y = 0; y < HEIGHT; y++) { // purple at start up
    for( byte x = 0; x < WIDTH; x++) {
      leds[XY(x, y)]  = CRGB::Purple;
    }
  }
}

void changeTime(int change) {
  // change the time by the specified number of seconds
  unsigned long t = now() + change;
  RTC.set(t);
  setTime(t);
}

void loop()
{
  ms = millis();
  if (!faded || (ms < 3000)) { // fade in
    FastLED.setBrightness(scale8(brightness, (ms * 256) / 3000));
    faded = true;
  }
  else {
    int buttonUp = digitalRead(PIN_BUTTON_UP);
    int buttonDown = digitalRead(PIN_BUTTON_DOWN);
    int buttonFN = digitalRead(PIN_BUTTON_FN);
    uint16_t pot = analogRead(PIN_POT); // potentiometer
    
    if (buttonUp) {
      changeTime(1+(buttonFN*60*60)); // either increase hours or minutes
      fadeToBlackBy(leds, NUM_LEDS, 150);
      leds[STATUS_LED_TIME_ADJUST] = CRGB::Yellow;
      delay(100*(buttonFN*2));
    }
    if (buttonDown) {
      changeTime(-1-(buttonFN*60*60)); // either decrease hours or minutes
      fadeToBlackBy(leds, NUM_LEDS, 150);
      leds[STATUS_LED_TIME_ADJUST] = CRGB::Purple;
      delay(100*(buttonFN*2));
    }
    
    if (buttonFN) {
      leds[STATUS_LED] = CRGB::Blue;
      
      autoHue = pot > 1000; // if you turn it up all the way it enables auto hue
      hue = map(pot, 0, 1000, 0, 255);
    }
    else {
      autoB = pot > 1000; // if you turn it up all the way it enables auto brightness
      brightness = map(pot, 0, 1000, 0, 255);
    }
    
    if (autoB) {
      leds[STATUS_LED_AUTOB] = CRGB::Blue;
      
      if(hour() < 8) brightness = MIN_BRIGHTNESS;
      else if(hour() < 10) brightness = MIN_BRIGHTNESS + (((((hour()-8)*3600) + (minute()*60) + second())*(uint32_t)(255-MIN_BRIGHTNESS))/7200); // TODO: replace with map
      else if(hour() < 21) brightness = 255;
      else brightness = 255 - (((((hour()-21)*3600) + (minute()*60) + second())*(uint32_t)(255-MIN_BRIGHTNESS))/10800); // TODO replace with map
    }

    if (autoHue) {
      leds[STATUS_LED_AUTOHUE] = CRGB::Blue;

      // red at 8 then through the spectrum from there
      hue = ((long)(((long)(hour() >= 8 ? hour()-8 : 16+hour())*3600)+((minute()*60)+second()))*(uint32_t)255)/86400;
    }
    
    FastLED.setBrightness(brightness);
    writeTime(hour(),minute());
  }
  FastLED.show();
}

void writeTime(int hours, int mins) {
  // output the time to the LEDS
  
  fadeToBlackBy(leds, NUM_LEDS, 15);
  int c1, c2, c3, c4;
  splitNumber(hours,c1,c2);
  splitNumber(mins,c3,c4);
  
  writeNumber(0,c1,CHSV(hue, 255, 255));
  writeNumber(1,c2,CHSV(hue+30, 255, 255));
  writeNumber(2,c3,CHSV(hue, 255, 255));
  writeNumber(3,c4,CHSV(hue+30, 255, 255));

  float seconds_ind_length = 60 / WIDTH; // how many seconds each of the bottom represent

  for (int i = 0; i<WIDTH; i++) // bottom row
    if (second() >= (i*seconds_ind_length) && second() < ((i*seconds_ind_length)+seconds_ind_length)) 
      leds[XY(i,HEIGHT-1)] = CHSV((int)((i*255)/WIDTH), 200, 255); // rainbow colours across the bottom
  
  FastLED.show();
}

void splitNumber(int num, int &one, int &two) {
  // split number into two digits
  one = (int)(num/10)%10;
  two = num % 10;
}

void writeNumber(int pos, int num, CRGB col) {
  // write an individual num to a pos with the specifeied colour (col)
  
  pos *= 3; // because width of 12 (each digit 3 wide)
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

