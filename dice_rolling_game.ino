// --- libraries ---
#include <U8g2lib.h>


U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0);


/* --- notes for hardware ---
  OLED display pins:
    A4: SDA
    A5: SCL

  Buttons: 
    Roll dice button: A1
    Stand button: A2

  Wire colors:
    Red: 3.3/5V
    Black: ground
    Yellow: button (power in)
    Blue: SDA/SCL
*/


// --- setting pins ---
const int hitPin = A1;
const int standPin = A2;

const int groundPins[] = {2, 3, 4, 5};
const int nGroundPins = 4;

// seg pins              a , b , c , d, e, f, g
const int lightPins[] = {12, 11, 10, 9, 8, 7, 6};
const int nLightPins = 7;


// --- number patters for 7 segment display ---
// a,b,c,d,e,f,g
const bool numberPatterns[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,0,0,1,1}  // 9
};


// --- Game variables ---
bool hitPressed = false;
bool standPressed = false;
int rollSum = 0;
int rollValue = 0;
int dealerSum = 0;
int dealerRoll = 0;


// --- Functions ---
int rollTheDice() {
  return random(1, 7);
}

// Show ONE digit at ONE position
void showDigit(int digit, int position) {
  // Turn OFF all digits
  for (int i = 0; i < nGroundPins; i++) {
    digitalWrite(groundPins[i], HIGH);
  }

  // Skip blank digits
  if (digit < 0) return;

  // Set segments
  for (int i = 0; i < nLightPins; i++) {
    digitalWrite(lightPins[i], numberPatterns[digit][i]);
  }

  // Turn ON selected digit
  digitalWrite(groundPins[position], LOW);
}

// needed for the next two functions
int digits[4] = {0, 0, 0, 0};
int currentDigit = 0;

void displayRollSum(int number) {
  digits[0] = number / 10;
  digits[1] = number % 10;
}

void displayDealerSum(int number) {
  digits[2] = number / 10;
  digits[3] = number % 10;
}

// this draws the dots on the dice
void drawDots(int x, int y) {
  oled.drawPixel(x, y);
  oled.drawPixel(x+1, y);
  oled.drawPixel(x-1, y);
  oled.drawPixel(x, y+1);
  oled.drawPixel(x, y-1);
}


void setup() {

  oled.begin();
  oled.setFont(u8g2_font_ncenB08_tr);

  pinMode(hitPin, INPUT_PULLUP);
  pinMode(standPin, INPUT_PULLUP);

  for (int i = 0; i < nGroundPins; i++) {
    pinMode(groundPins[i], OUTPUT);
  }

  for (int i = 0; i < nLightPins; i++) {
    pinMode(lightPins[i], OUTPUT);
  }
}


void loop() {
  // --- FAST: 7-segment multiplexing ---
  static unsigned long lastRefresh = 0;
  const int refreshInterval = 500; 

  if (micros() - lastRefresh >= refreshInterval) {
    lastRefresh = micros();

    showDigit(digits[currentDigit], currentDigit);
    currentDigit = (currentDigit + 1) % nGroundPins;
  }
  

  // --- SLOW: logic + OLED ---
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 400) {
    lastUpdate = millis();

    int hitState = digitalRead(hitPin);
    int standState = digitalRead(standPin);

    if (hitState == 0 && hitPressed == false) {
      rollValue = rollTheDice();
      rollSum += rollValue;

      displayRollSum(rollSum); // update 7-seg display
    }

    if (standState == 0) {
      standPressed = true;
    }

    if (standPressed == true || rollSum >= 21) {
      while (dealerSum < 17) {
        dealerRoll = rollTheDice();
        dealerSum += dealerRoll;
      }
      displayDealerSum(dealerSum);
    }

    // OLED display
    oled.clearBuffer();

    // draws the frame of the dice
    oled.drawFrame(42, 10, 42, 44);

    // draws the dots on the dice
    switch (rollValue) {
      case 1: 
        drawDots(63, 31);
        break;
      case 2:
        drawDots(50, 18);
        drawDots(76, 44);
        break;
      case 3: 
        drawDots(50, 18);
        drawDots(63, 31);
        drawDots(76, 44);
        break;
      case 4: 
        drawDots(50, 18);
        drawDots(76, 18);
        drawDots(50, 44);
        drawDots(76, 44);
        break;
      case 5: 
        drawDots(50, 18);
        drawDots(76, 18);
        drawDots(63, 31);
        drawDots(50, 44);
        drawDots(76, 44);
        break;
      case 6: 
        drawDots(50, 18);
        drawDots(76, 18);
        drawDots(50, 31);
        drawDots(76, 31);
        drawDots(50, 44);
        drawDots(76, 44);
        break;
    }

    oled.sendBuffer();

    hitPressed = (hitState == 0);
  }
}