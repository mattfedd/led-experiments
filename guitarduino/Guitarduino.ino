#include <OctoWS2811.h>

const int ledsPerStrip = 30;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
int indexTranslation[ledsPerStrip*8];

const int config = WS2811_GRB | WS2811_800kHz;
const int MSG_SIZE = 12;
const int MAX_PARTICLES = 32; 

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

byte serialByte = 0;
byte serialMessage[MSG_SIZE];
int counter = 0;
long tick = 0;

/*
#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF
*/
// Less intense...

#define RED    0x160000
#define GREEN  0x001600
#define BLUE   0x000016
#define YELLOW 0x101400
#define PINK   0x120009
#define ORANGE 0x100400
#define WHITE  0x101010

struct InputState {
  word whammy_bar;
  byte slider_thing;
  byte b2;
  byte button_mask;
  byte which_keys;
  byte dpad;
};

struct Particle {
  bool active;
  byte x;
  byte y;
  int color;
};

InputState state = {0};
InputState prevState = {0};
Particle particles[MAX_PARTICLES];

void setup() {
  leds.begin();
  Serial.begin(9600);
  setupIndexTranslation();
  leds.show();
}

void loop() {
  getInput();
  
  tick++;
  if(tick % 50 == 0) //approx every 50 ms
  {
    updateDrawMemory();
    draw();
  }
}

void getInput()
{
  if (Serial.available() > 0)
  {
    serialByte = Serial.read();
    serialMessage[counter] = serialByte;
    counter++;

    if(counter == MSG_SIZE) 
    {
      prevState.whammy_bar = state.whammy_bar;
      prevState.slider_thing = state.slider_thing;
      prevState.b2 = state.b2;
      prevState.button_mask = state.button_mask;
      prevState.which_keys = state.which_keys;
      prevState.dpad = state.dpad;

      state.whammy_bar = ((word)(serialMessage[3] << 8) | (word)(serialMessage[2]));
      state.slider_thing = serialMessage[4];
      state.b2 = serialMessage[5];
      state.button_mask = serialMessage[6];
      state.which_keys = serialMessage[7];
      state.dpad = serialMessage[8];

      counter = 0;
    }
  }
  else
  {
    delay(1); // simulate the serial.read() delay
  }
  
}

void updateDrawMemory() 
{
  eraseBoard();

  // show buttons and d pad on last row
  int row = 7;
  int c = 0;
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * ((state.button_mask & (0x1 << 0)) != 0));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * ((state.button_mask & (0x1 << 1)) != 0));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * ((state.button_mask & (0x1 << 3)) != 0));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * ((state.button_mask & (0x1 << 2)) != 0));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * ((state.button_mask & (0x1 << 4)) != 0));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * (state.dpad == 1));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * (state.dpad == 3));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * (state.dpad == 5));
  leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * (state.dpad == 7));

  // whammy bar on second-to-last row
  row = 6;
  c = 0;
  for (int i = 0; i < 16; i++)
  { 
    leds.setPixel(indexTranslation[ledsPerStrip*row + c++], WHITE * (state.whammy_bar > (0xFFFF / 16 * c)));
  }


// TODO make a cool down for how often each row can have a new particle. Right now it spawns new particles IMMEDIATELY if you hold down too long
  int particleIndex = 0;
  if (state.dpad == 1 || state.dpad == 5) // up or down 
  {
    if((state.button_mask & (0x1 << 0)) != 0) // first button, green
    {
      particleIndex = findFreeParticle();
      if(particleIndex < MAX_PARTICLES) 
      {
        particles[particleIndex].active = true;
        particles[particleIndex].x = 0;
        particles[particleIndex].y = 0;
      }
    }
    if((state.button_mask & (0x1 << 1)) != 0) // second button, red
    {
      particleIndex = findFreeParticle();
      if(particleIndex < MAX_PARTICLES) 
      {
        particles[particleIndex].active = true;
        particles[particleIndex].x = 1;
        particles[particleIndex].y = 0;
      }
    }
    if((state.button_mask & (0x1 << 3)) != 0) // third button, yellow. Yes indexes 2 and 3 are swapped
    {
      particleIndex = findFreeParticle();
      if(particleIndex < MAX_PARTICLES) 
      {
        particles[particleIndex].active = true;
        particles[particleIndex].x = 2;
        particles[particleIndex].y = 0;
      }
    }
    if((state.button_mask & (0x1 << 2)) != 0) // fourth button, blue
    {
      particleIndex = findFreeParticle();
      if(particleIndex < MAX_PARTICLES) 
      {
        particles[particleIndex].active = true;
        particles[particleIndex].x = 3;
        particles[particleIndex].y = 0;
      }
    }
    if((state.button_mask & (0x1 << 4)) != 0) // fifth button, orange
    {
      particleIndex = findFreeParticle();
      if(particleIndex < MAX_PARTICLES) 
      {
        particles[particleIndex].active = true;
        particles[particleIndex].x = 4;
        particles[particleIndex].y = 0;
      }
    }
  }

  updateParticles();
}

void draw() 
{
  leds.show();
}


void eraseBoard()
{
  for(int i=0; i<ledsPerStrip*6; i++)
  {
    leds.setPixel(indexTranslation[i], 0);
  }
}

void setupIndexTranslation()
{
  int order[8] = {3,0,1,2,7,4,5,6};
  for (int i=0; i<8; i++)
  {
    for (int j=0; j < 30; j++) 
    {
      indexTranslation[i*30 + j] = order[i]*30 + j;
    }
  }  
}

int findFreeParticle() 
{
  for (int i=0; i<MAX_PARTICLES; i++)
  {
    if(particles[i].active == false)
    {
      return i;
    }
  }
  return MAX_PARTICLES;
}

void updateParticles()
{
  for (int i=0; i<MAX_PARTICLES; i++)
  {
    if(particles[i].active == true)
    {
      leds.setPixel(indexTranslation[ledsPerStrip*particles[i].x + particles[i].y], WHITE);
      particles[i].y++;
    }
    if(particles[i].y >= ledsPerStrip)
    {
      particles[i].active = false;
      particles[i].x = 0;
      particles[i].y = 0;
      particles[i].color = 0;
    }
  }
  
}

