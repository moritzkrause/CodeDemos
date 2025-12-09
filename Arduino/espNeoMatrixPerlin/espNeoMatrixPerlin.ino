
#include <FastLED.h>

// matrix size
uint8_t Width  = 16;
uint8_t Height = 16;
uint8_t CentreX =  (Width / 2) - 1;
uint8_t CentreY = (Height / 2) - 1;

// NUM_LEDS = Width * Height
#define NUM_LEDS      256
#define BRIGHTNESS    100
CRGB leds[NUM_LEDS];



/*
 * NOISE UNITS
 */
// max movement speed
float speeed  = 3;
// total field scale – "resolution" with a lower number generating "bigger blobs" but also more flickering, 12 works fine
int sclX    =  2;
int sclY    =  2;
// "speed" of animation
int medianX =  120; // 120
int medianY =  120; // 120

int intensity = 6;

int c_low = 0;

int ts_start =   0;   //   0
int ts_low   =  64;   //  64
int ts_mid   = 128;   // 128
int ts_high  = 192;   // 192
int ts_end   = 255;   // 255

DEFINE_GRADIENT_PALETTE( pit ) {
  ts_start, c_low, c_low, c_low,
  ts_low,     255,   255,   255,  // b r g
  ts_mid,   c_low, c_low, c_low,
  ts_high,    255,   255,   255,  // b r g
  ts_end,   c_low, c_low, c_low
};




/*
 * VALUES
 */
int IAQ = 25; // 0 - 500

int IAQ_Range = 2;

/*
 * SETUP
 */
 
void setup() {
  Serial.begin(115200);

  // Adjust this for you own setup. Use the hardware SPI pins if possible.
  // On Teensy 3.1/3.2 the pins are 11 & 13
  // Details here: https://github.com/FastLED/FastLED/wiki/SPI-Hardware-or-Bit-banging
  // In case you see flickering / glitching leds, reduce the data rate to 12 MHZ or less
  // LEDS.addLeds<APA102, 11, 13, BGR, DATA_RATE_MHZ(24)>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.addLeds<WS2811,0,BGR>(leds,NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxRefreshRate(400);
}

// parameters and buffer for the noise array
#define NUM_LAYERS 1
uint32_t x[NUM_LAYERS];
uint32_t y[NUM_LAYERS];
uint32_t z[NUM_LAYERS];
uint32_t scale_x[NUM_LAYERS];
uint32_t scale_y[NUM_LAYERS];
uint8_t  noise[1][16][16];




/*
 * LOOP
 */

void loop() {

  noise_noise1();

  // check the Serial Monitor to see how many fps you get
  EVERY_N_MILLIS(4000) {
    //Serial.println(LEDS.getFPS());
  }
}



/*
 * CALC & HELPERS
 */


// indexing within a zigzag matrix
uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;
  if ( y & 0x01) {
    uint8_t reverseX = (Width - 1) - x;
    i = (y * Width) + reverseX;
  } else {
    i = (y * Width) + x;
  }
  return i;
}

// cheap correction with gamma 2.0 - makes it look more saturated
void adjust_gamma()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    if(IAQ_Range == 4) { // init
      leds[i].r = dim8_video(leds[i].r)/10;   // blue  10
      leds[i].g = dim8_video(leds[i].g)/10;   // red   10
      leds[i].b = dim8_video(leds[i].b)/10;   // green 10
    }
    else if(IAQ_Range == 0) { // v bad
      leds[i].r = dim8_video(leds[i].r)/60;   // blue  60
      leds[i].g = dim8_video(leds[i].g)/3;    // red    3
      leds[i].b = dim8_video(leds[i].b)/80;   // green 80
    }
    else if(IAQ_Range == 1) { // okay
      leds[i].r = dim8_video(leds[i].r)/30;   // blue  30
      leds[i].g = dim8_video(leds[i].g)/12;   // red   12
      leds[i].b = dim8_video(leds[i].b)/14;   // green 14
    }
    else if(IAQ_Range == 2) { // noice
      leds[i].r = dim8_video(leds[i].r)/32;   // blue  28
      leds[i].g = dim8_video(leds[i].g)/40;   // red   40
      leds[i].b = dim8_video(leds[i].b)/6;    // green  6
    }
  }
}

// perlin noise controlled & modulated by itself
void noise_noise1() {

  CRGBPalette16 Pal( pit );

  //modulate the position so that it increases/decreases x
  //the "-255" defines the median moving direction
  x[0] = x[0] + (speeed * noise[0][0][0]) - medianX;
  //modulate the position so that it increases/decreases y
  //(here based on the top right pixel - it could be any position else)
  y[0] = y[0] + (speeed * noise[0][Width-1][0]) - medianY;
  //z just in one direction but with the additional "1" to make sure to never get stuck
  //in case the movement is stopped by a crazy parameter (noise data) combination
  //(here based on the down left pixel - it could be any position else)
  z[0] += 1 + ((noise[0][0][Height-1]) / intensity);      // / 2 - 6
  //set the scaling based on left and right pixel of the middle line
  //here you can set the range of the zoom in both dimensions
  // scale_x[0] = 8000 + (noise[0][0][CentreY] * 16);
  // scale_y[0] = 8000 + (noise[0][Width-1][CentreY] * 16);
  scale_x[0] = 1000 * sclX + (noise[0][0][CentreY] * 16);
  scale_y[0] = 1000 * sclY + (noise[0][Width-1][CentreY] * 16);

  //calculate the noise data
  uint8_t layer = 0;
  for (uint8_t i = 0; i < Width; i++) {
    uint32_t ioffset = scale_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < Height; j++) {
      uint32_t joffset = scale_y[layer] * (j - CentreY);
      uint16_t data = inoise16(x[layer] + ioffset, y[layer] + joffset, z[layer]);
      // limit the 16 bit results to the interesting range
      if (data < 11000) data = 11000;
      if (data > 51000) data = 51000;
      // normalize
      data = data - 11000;
      // scale down that the result fits into a byte
      data = data / 161;
      // store the result in the array
      noise[layer][i][j] = data;
    }
  }

  //map the colors
  for (uint8_t y = 0; y < Height; y++) {
    for (uint8_t x = 0; x < Width; x++) {
      //I will add this overlay CRGB later for more colors
      //it´s basically a rainbow mapping with an inverted brightness mask
      CRGB overlay = CHSV(noise[0][y][x], 255, noise[0][x][y]);
      //here the actual colormapping happens - note the additional colorshift caused by the down right pixel noise[0][15][15]
      leds[XY(x, y)] = ColorFromPalette( Pal, noise[0][Width-1][Height-1] + noise[0][x][y]) + overlay;
    }
  }

  //make it looking nice
  adjust_gamma();

  //and show it!
  FastLED.show();
}
