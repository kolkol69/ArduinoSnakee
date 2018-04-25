#include "LedControl.h" // LedControl library is used for controlling a LED matrix. Find it using Library Manager or download zip here: https://github.com/wayoda/LedControl


/*
 * ==========================================================
 * ======================INITIAL CONFIG======================
 * ==========================================================
 */

// there are defined all the pins
struct Pin {
  static const short joystickX = A0;   // joystick X axis
  static const short joystickY = A1;   // joystick Y axis
  static const short CLK = 10;   // clock for LED matrix
  static const short CS  = 11;  // chip-select for LED matrix
  static const short DIN = 12; // data-in for LED matrix
};

// LED matrix brightness: between 0(the darkest) and 15( the brightest )
const short led_brightness = 10;

// lower = faster message scrolling
const short messageSpeed = 5;

// initial snake length (1...63, recommended 3)
const short initialSnakeLength = 3;


void setup() {
  Serial.begin(115200);  // set the same baud rate on your Serial Monitor
  initialize();         // initialize pins & LED matrix
  calibrateJoystick(); // calibrate the joystick home 
  showWelcomeMsg(); // scrolls the 'snake' message around the matrix
}


void loop() {
  generateFood();    // if there is no food, generate one
  scanJoystick();    // watches joystick movements & blinks with food
  calculateSnake();  // calculates snake parameters
  handleGameStates(); // check for win\loose
}

/*
 * ==========================================================
 * =================SOME ADDITIONAL VARIABLES================
 * ==========================================================
 */
LedControl matrix(Pin::DIN, Pin::CLK, Pin::CS, 1);

struct Point {
  int row = 0, col = 0;
  Point(int row = 0, int col = 0): row(row), col(col) {}
};

struct Coordinate {
  int x = 0, y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};

bool win = false;
bool gameOver = false;

// primary snake head coordinates (snake head), it will be randomly generated
Point snake;

// food is not anywhere yet
Point food(-1, -1);

// construct with default values in case the user turns off the calibration
Coordinate joystickHome(500, 500);

// snake parameters
int snakeLength = initialSnakeLength;
int snakeSpeed = 500; // the bigger number the slower snake moves
int snakeDirection = 0; // if it is 0, the snake does not move

// direction constants
const short up     = 1;
const short right  = 2;
const short down   = 3; 
const short left   = 4; 

// threshold where movement of the joystick will be accepted
const int joystickThreshold = 160;

// the age array: holds an 'age' of the every pixel in the matrix. If age > 0, it glows.
// on every frame, the age of all lit pixels is incremented.
// when the age of some pixel exceeds the length of the snake, it goes out.
// age 1 is added in the current snake direction next to the last position of the snake head.
// This way we can controll the "movement"" effect of the snake. When its moving left
// every pixel is incrementing and changing position, the first one becomes +1 and 
// starts to glow and the last one is no more glowing, so it looks like
// snake moved one pixel to the left 
int age[8][8] = {};


/*
 * ==========================================================
 * ===================FUNCTION DECLARATION===================
 * ==========================================================
 */
// if there is no food, generate one, also check for victory
void generateFood() {
  if (food.row == -1 || food.col == -1) {
    // self-explanatory
    if (snakeLength >= 64) {
      win = true;
      // prevent the food generator from running, in this case it would run forever, because it will not be able to find a pixel without a snake
      return; 
    }

    // generate food until it is in the right position
    do {
      food.col = random(8);
      food.row = random(8);
    } while (age[food.row][food.col] > 0);// check if there food is not generating on top of the snake's body
  }
}

// watches joystick movements & blinks with food
void scanJoystick() {
  int previousDirection = snakeDirection; // save the last direction
  long timestamp = millis() + snakeSpeed; // when the next frame will be rendered

  while (millis() < timestamp) {   
    // determine the direction of the snake
    analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold ? snakeDirection = down    : 0;
    analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold ? snakeDirection = up  : 0;
    analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold ? snakeDirection = right  : 0;
    analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold ? snakeDirection = left : 0;

    // ignore directional change by 180 degrees (no effect for non-moving snake)
    snakeDirection + 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;
    snakeDirection - 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;

    // intelligently blink with the food
    matrix.setLed(0, food.row, food.col, millis() % 100 < 50 ? 1 : 0);
  }
}


// calculate snake movement data
void calculateSnake() {
  switch (snakeDirection) {
  case up:
    snake.row--;
    fixEdge();
    matrix.setLed(0, snake.row, snake.col, 1);
    break;

  case right:
    snake.col++;
    fixEdge();
    matrix.setLed(0, snake.row, snake.col, 1);
    break;

  case down:
    snake.row++;
    fixEdge();
    matrix.setLed(0, snake.row, snake.col, 1);
    break;

  case left:
    snake.col--;
    fixEdge();
    matrix.setLed(0, snake.row, snake.col, 1);
    break;

  default: // if the snake is not moving, exit
    return;
  }

  // if there is any age (snake body), this will cause the end of the game (snake must be moving)
  if (age[snake.row][snake.col] != 0 && snakeDirection != 0) {
    gameOver = true;
    return;
  }

  // check if the food was eaten
  if (snake.row == food.row && snake.col == food.col) {
    snakeLength++;
    // make game more interesting and harder to play. Speeding up the snake everytime you eat food:)
    snakeSpeed > 100 ? snakeSpeed = snakeSpeed - 10 : 100;
    // reset food 
    food.row = -1; 
    food.col = -1;
  }

  // increment ages if all lit leds
  updateAges();

  // change the age of the snake head from 0 to 1
  age[snake.row][snake.col]++;
}


// causes the snake to appear on the other side of the screen if it gets out of the edge
void fixEdge() {
  snake.col < 0 ? snake.col += 8 : 0;
  snake.col > 7 ? snake.col -= 8 : 0;
  snake.row < 0 ? snake.row += 8 : 0;
  snake.row > 7 ? snake.row -= 8 : 0;
}


// increment ages if all lit leds, turn off too old ones depending on the length of the snake
void updateAges() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      // if the led is lit, increment it's age
      if (age[row][col] > 0 ) {
        age[row][col]++;
      }

      // if the age exceeds the length of the snake, switch it off
      if (age[row][col] > snakeLength) {
        matrix.setLed(0, row, col, 0);
        age[row][col] = 0;
      }
    }
  }
}


void handleGameStates() {
  if (gameOver || win) {
    unrollSnake();

    showScoreMsg(snakeLength-3);

    if (gameOver) showGameOverMsg();
    else if (win) showWinMessage();

    // re-init the game
    win = false;
    gameOver = false;
    snake.row = random(8);
    snake.col = random(8);
    snakeSpeed = 500;
    food.row = -1;
    food.col = -1;
    snakeLength = initialSnakeLength;
    snakeDirection = 0;
    memset(age, 0, sizeof(age[0][0]) * 8 * 8);
    matrix.clearDisplay(0);
  }
}


void unrollSnake() {
  // switch off the food LED
  matrix.setLed(0, food.row, food.col, 0);

  delay(600);

  for (int i = 1; i <= snakeLength; i++) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (age[row][col] == i) {
          matrix.setLed(0, row, col, 0);
          delay(100);
        }
      }
    }
  }
}


// calibrate the joystick home for 10 times
void calibrateJoystick() {
  Coordinate values;

  for (int i = 0; i < 10; i++) {
    values.x += analogRead(Pin::joystickX);
    values.y += analogRead(Pin::joystickY);
  }

  joystickHome.x = values.x / 10;
  joystickHome.y = values.y / 10;
}


void initialize() {
  matrix.shutdown(0, false);
  matrix.setIntensity(0, led_brightness);
  matrix.clearDisplay(0);

  randomSeed(analogRead(A5));
  snake.row = random(8);
  snake.col = random(8);
  snakeSpeed = 500;
}

// --------------------------------------------------------------- //
// -------------------------- messages --------------------------- //
// --------------------------------------------------------------- //


const PROGMEM bool snakeMsg[8][56] = {
  //SNAKE
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,0,1,1,1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0}
};

const PROGMEM bool gameOverMsg[8][90] = {
  // GAME OVER !
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,0,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,1,1,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0}
};

const PROGMEM bool winnerMsg[8][66] = {
  // WINNER !!!
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,0,0,0,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,1,1,1,1,0,1,1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,0,1,0,0,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,0,0,0,1,1,0,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,0,0}
};

const PROGMEM bool scoreMsg[8][58] = {
  // SCORE:
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0}
};

const PROGMEM bool digits[][8][8] = {
  {
    // zero
    {0,0,0,0,0,0,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,1,1,0,1,1,1,0},{0,1,1,1,0,1,1,0},{0,1,1,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0}
  },{
    // one
    {0,0,0,0,0,0,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,0,1,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,1,1,1,1,1,1,0}
  },{
    // two 
    {0,0,0,0,0,0,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,1,1,0,0},{0,0,1,1,0,0,0,0},{0,1,1,0,0,0,0,0},{0,1,1,1,1,1,1,0}
  },{
    // three
    {0,0,0,0,0,0,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,1,1,1,0,0},{0,0,0,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0}
  },{
    // four
    {0,0,0,0,0,0,0,0},{0,0,0,0,1,1,0,0},{0,0,0,1,1,1,0,0},{0,0,1,0,1,1,0,0},{0,1,0,0,1,1,0,0},{0,1,1,1,1,1,1,0},{0,0,0,0,1,1,0,0},{0,0,0,0,1,1,0,0}
  },{
    // five
    {0,0,0,0,0,0,0,0},{0,1,1,1,1,1,1,0},{0,1,1,0,0,0,0,0},{0,1,1,1,1,1,0,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0}
  },{
    // six
    {0,0,0,0,0,0,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,1,1,0,0,0,0,0},{0,1,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0}
  },{
    // seven
    {0,0,0,0,0,0,0,0},{0,1,1,1,1,1,1,0},{0,1,1,0,0,1,1,0},{0,0,0,0,1,1,0,0},{0,0,0,0,1,1,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0}
  },{
    // eight  
    {0,0,0,0,0,0,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0}
  },{
    // nine
    {0,0,0,0,0,0,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,1,0},{0,0,0,0,0,1,1,0},{0,1,1,0,0,1,1,0},{0,0,1,1,1,1,0,0}
  }
};


// shows the 'snake' message scrolling around the matrix
void showWelcomeMsg() {
  for (int d = 0; d < sizeof(snakeMsg[0]) - 7; d++) {
    for (int col = 0; col < 8; col++) {
      delay(messageSpeed);
      for (int row = 0; row < 8; row++) {
        // this reads the byte from the PROGMEM and displays it on the screen
        matrix.setLed(0, row, col, pgm_read_byte(&(snakeMsg[row][col + d])));
      }
    }
  }
}

// shows  the 'game over' message scrolling around the matrix
void showGameOverMsg() {
  for (int d = 0; d < sizeof(gameOverMsg[0]) - 7; d++) {
    for (int col = 0; col < 8; col++) {
      delay(messageSpeed);
      for (int row = 0; row < 8; row++) {
        // this reads the byte from the PROGMEM and displays it on the screen
        matrix.setLed(0, row, col, pgm_read_byte(&(gameOverMsg[row][col + d])));
      }
    }
  }
}

// shows the 'win' message scrolling around the matrix
void showWinMessage() {
  for (int d = 0; d < sizeof(winnerMsg[0]) - 7; d++) {
    for (int col = 0; col < 8; col++) {
      delay(messageSpeed);
      for (int row = 0; row < 8; row++) {
        // this reads the byte from the PROGMEM and displays it on the screen
        matrix.setLed(0, row, col, pgm_read_byte(&(winnerMsg[row][col + d])));
      }
    }
  }
}

// shows the 'score' message with numbers scrolling around the matrix
void showScoreMsg(int score) {
  if (score < 0 || score > 99) return;

  // specify score digits
  int second = score % 10;
  int first = (score / 10) % 10;

  for (int d = 0; d < sizeof(scoreMsg[0]) + 2 * sizeof(digits[0][0]); d++) {
    for (int col = 0; col < 8; col++) {
      delay(messageSpeed);
      for (int row = 0; row < 8; row++) {
        if (d <= sizeof(scoreMsg[0]) - 8) {
          matrix.setLed(0, row, col, pgm_read_byte(&(scoreMsg[row][col + d])));
        }

        int c = col + d - sizeof(scoreMsg[0]) + 6; // move 6 px in front of the previous message

        // if the score is < 10, shift out the first digit (zero)
        if (score < 10) c += 8;

        if (c >= 0 && c < 8) {
          if (first > 0) matrix.setLed(0, row, col, pgm_read_byte(&(digits[first][row][c]))); // show only if score is >= 10 (see above)
        } else {
          c -= 8;
          if (c >= 0 && c < 8) {
            matrix.setLed(0, row, col, pgm_read_byte(&(digits[second][row][c]))); // show always
          }
        }
      }
    }
  }
}