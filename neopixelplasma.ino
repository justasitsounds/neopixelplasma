/*
    Multicolored Plasma for the Arduino Micro-Controller and NeoPixel Shield
    Copyright (C) 2013 John Ericksen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/.
*/
/* 
    This is a derivative work that is combined from two sources:
    
    fscale function and microphone smaple code copied from:

    Adafruit's LED Ampli-Tie-with-Flora project:
    https://github.com/adafruit/LED-Ampli-Tie-with-Flora
    Copyright (c) 2012, Adafruit Industries
    All rights reserved.

*/

#include <Adafruit_NeoPixel.h>
#include <math.h>

#define MIC_PIN   A0  // Microphone is attached to this analog pin
#define SAMPLE_WINDOW   10  // Sample window for average level
#define INPUT_FLOOR 10 //Lower range of analogRead input
#define INPUT_CEILING 1023 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)

unsigned int sample;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, 6, NEO_GRB + NEO_KHZ800);

// Convenient 2D point structure
struct Point {
  float x;
  float y;
};

float phase = 0.2;
float phaseIncrement = 0.12;  // Controls the speed of the moving points. Higher == faster. I like 0.08 .
float colorStretch = 0.15;    // Higher numbers will produce tighter color bands. I like 0.11 .

float lissX = 4.5;
float lissY = 4.0;

int rowCount = 10;
int columnCount = 15;

void setup() {
  
  analogReference(EXTERNAL);

  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

// This function is called every frame.
void loop() {
  //Mic sampling
  unsigned long startMillis= millis();  // Start of sample window
  float peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  float scalar;
  
  // collect data for length of sample window (in mS)
  while (millis() - startMillis < SAMPLE_WINDOW)
  {
    sample = analogRead(MIC_PIN);
    if (sample < 1024)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  scalar = (fscale(INPUT_FLOOR, INPUT_CEILING, 0, 75, peakToPeak, 1) + 25)/100;
  Serial.println(scalar);
  
  phase += scalar*phaseIncrement;
  
  // The two points move along Lissajious curves, see: http://en.wikipedia.org/wiki/Lissajous_curve
  // We want values that fit the LED grid: x values between 0..13, y values between 0..8 .
  // The sin() function returns values in the range of -1.0..1.0, so scale these to our desired ranges.
  // The phase value is multiplied by various constants; I chose these semi-randomly, to produce a nice motion.
  Point p1 = { (sin(phase*1.000)+1.0) * lissX, (sin(phase*1.310)+1.0) * lissY };
  Point p2 = { (sin(phase*1.770)+1.0) * lissX, (sin(phase*2.865)+1.0) * lissY };
  Point p3 = { (sin(phase*0.250)+1.0) * lissX, (sin(phase*0.750)+1.0) * lissY };
  
  byte row, col;
  
  // For each row...
  for( row=0; row<rowCount; row++ ) {
    float row_f = float(row);  // Optimization: Keep a floating point value of the row number, instead of recasting it repeatedly.
    
    // For each column...
    for( col=0; col<columnCount; col++ ) {
      float col_f = float(col);  // Optimization.
      
      // Calculate the distance between this LED, and p1.
      Point dist1 = { col_f - p1.x, row_f - p1.y };  // The vector from p1 to this LED.
      float distance1 = sqrt( dist1.x*dist1.x + dist1.y*dist1.y );
      
      // Calculate the distance between this LED, and p2.
      Point dist2 = { col_f - p2.x, row_f - p2.y };  // The vector from p2 to this LED.
      float distance2 = sqrt( dist2.x*dist2.x + dist2.y*dist2.y );
      
      // Calculate the distance between this LED, and p3.
      Point dist3 = { col_f - p3.x, row_f - p3.y };  // The vector from p3 to this LED.
      float distance3 = sqrt( dist3.x*dist3.x + dist3.y*dist3.y );
      
      // Warp the distance with a sin() function. As the distance value increases, the LEDs will get light,dark,light,dark,etc...
      // You can use a cos() for slightly different shading, or experiment with other functions. Go crazy!
      float color_1 = distance1;  // range: 0.0...1.0
      float color_2 = distance2;
      float color_3 = distance3;
      float color_4 = (cos( distance1 * distance2 * colorStretch * scalar )) + 2.0 * 0.5;
      
      // Square the color_f value to weight it towards 0. The image will be darker and have higher contrast.
      color_1 *= color_1 * color_4 * scalar;
      color_2 *= color_2 * color_4 * scalar;
      color_3 *= color_3 * color_4 * scalar;
      color_4 *= color_4;
 
      // Scale the color up to 0..7 . Max brightness is 7.
      //strip.setPixelColor(col + (columnCount * row), strip.Color(color_4, color_2, color_1));
      strip.setPixelColor(col + (columnCount * row), strip.Color(color_1, color_2, color_3));
    }
  }
  strip.show();
}


float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
   Serial.println(curve * 100, DEC);   // multplleg_y by 100 to preserve resolution  
   Serial.println(); 
   */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}
