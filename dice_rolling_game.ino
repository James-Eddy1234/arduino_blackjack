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
bool gameOver = false;
static int winCounter = 0;
static int loseCounter = 0;
static int tieCounter = 0;
unsigned long gameOverTime = 0;


// --- Functions ---
int rollTheDice() {
  return random(1, 7);
}

// show digit at a postition
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

// used to reset the game to allow for continued playing
void resetGame() {
  rollSum = 0;
  dealerSum = 0;
  rollValue = 0;
  dealerRoll = 0;

  hitPressed = false;
  standPressed = false;

  for (int i = 0; i<4; i++) {
    digits[i] = 0;
  }
}


void setup() {

  oled.begin();
  oled.setFont(u8g2_font_ncenB08_tr);

  pinMode(hitPin, INPUT_PULLUP);
  pinMode(standPin, INPUT_PULLUP);

  randomSeed(analogRead(A0)); // used for better randomness with roll values

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

    // roll dice
    if (hitState == 0 && hitPressed == false) {
      rollValue = rollTheDice();
      rollSum += rollValue;

      displayRollSum(rollSum); // update 7-seg display
    }

    if (standState == 0) {
      standPressed = true;
    }

    // happens if you stand or go past 21
    if (!gameOver && (standPressed == true || rollSum >= 21)) {
      while (dealerSum < 17) {
        dealerRoll = rollTheDice();
        dealerSum += dealerRoll;
      }

      displayDealerSum(dealerSum);
      gameOver = true;
    }

    if (gameOver) {

      // distances to 21
      int playerTo21 = 21 - rollSum;
      int dealerTo21 = 21 - dealerSum;
      
      if (gameOver && gameOverTime == 0) {

        gameOverTime = millis();

        // win/lose conditions
        if (rollSum > 21) {
          loseCounter += 1;
        } else if (rollSum == 21 && dealerSum != 21) {
          winCounter += 1;
        } else if (dealerSum > 21) {
          winCounter += 1;
        } else if (playerTo21 < dealerTo21) {
          winCounter += 1;
        } else if (dealerTo21 < playerTo21) {
          loseCounter += 1;
        } else {
          tieCounter += 1;
        }
      }

      if (millis() - gameOverTime > 2000) {
        resetGame();
        gameOver = false;
        gameOverTime = 0;
      }
    }

    // OLED display
    oled.clearBuffer();

    // print win counter
    oled.setCursor(0,10);
    oled.print("W:");
    oled.setCursor(15, 10);
    oled.print(winCounter);

    // print lose counter
    oled.setCursor(50, 10);
    oled.print("L:");
    oled.setCursor(65, 10);
    oled.print(loseCounter);

    // print tie counter
    oled.setCursor(100, 10);
    oled.print("T:");
    oled.setCursor(115, 10);
    oled.print(tieCounter);

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