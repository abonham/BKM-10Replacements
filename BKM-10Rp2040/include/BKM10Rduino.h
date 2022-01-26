#ifndef __BKM10R_H
#define __BKM10R_H

#define COMMANDS_SIZE 35
#define IR_INPUT_PIN 3
#define TX_ENABLE_PIN 6
#define RX_ENABLE_LOW_PIN 7
#define LEARN_ENABLE_PIN 2
#define LEARN_TIMEOUT 3000
#define SLEEP_TIMER 180000
#define CIRCULAR_BUFFER_INT_SAFE
#define SERIAL_LOGGING
#define __ASSERT_USE_STDERR
#define POLL_RATE 33
#define TICK_RATE 100

#include <Arduino.h>
#include "CircularBuffer.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

enum bank
{
  ISW,
  ILE,
  IEN,
  IMT,
  ICC,
  DATA,
  none
};

/*
 * Holds any timestamps required for time based events
 */
struct Timers
{
  unsigned long lastPoll;
  unsigned long learnHold;
  unsigned long lastInput;
};

const byte keydown = 0x44;
const char ISWBank[] = "ISW";
const char IENBank[] = "IEN";

struct RemoteKey
{
  uint16_t address;
  uint8_t code;
  uint8_t id;
};

extern RemoteKey remoteKey;

/*
 * Two byte BKM-10R control code
 *
 * Codes are sent over RS485 as `keydown, group, code`
 */
struct ControlCode
{
  byte group;
  byte code;
};

extern ControlCode controlCode;

struct RotaryEncoder
{
  byte encoderId;
};

struct Command
{
  struct ControlCode cmd;
  bool repeats;
  uint8_t id;
};

extern Command command;

const char power[] = "Power";
const char menu[] = "Menu";
const char enter[] = "Enter";
const char up[] = "Up";
const char down[] = "Down";
const char zero[] = "0";
const char one[] = "1";
const char two[] = "2";
const char three[] = "3";
const char four[] = "4";
const char five[] = "5";
const char six[] = "6";
const char seven[] = "7";
const char eight[] = "8";
const char nine[] = "9";
const char del[] = "Del";
const char ent[] = "Ent";
const char shiftSel[] = "Shift";
const char overscan[] = "Overscan/16:9";
const char hsync[] = "HSync/Sync";
const char vsync[] = "VSync/Blue";
const char mono[] = "Mono/R";
const char apt[] = "Apt/G";
const char comb[] = "Comb/B";
const char f1[] = "F1/F2";
const char f3[] = "F3/F4";
const char safe_area[] = "Safe Area/Address";
const char phase[] = "Phase manual";
const char chroma[] = "Chroma manual";
const char bright[] = "Brigtness manual";
const char contrast[] = "Contrast manual";
const char degauss[] = "Degauss";
const char encoderSwitch[] = "Next rotary";
const char encoderUp[] = "Rotary up";
const char encoderDown[] = "Rotary down";

const char *const names[] = {
    power,
    menu,
    enter,
    up,
    down,
    zero,
    one,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    del,
    ent,
    shiftSel,
    overscan,
    hsync,
    vsync,
    mono,
    apt,
    comb,
    f1,
    f3,
    safe_area,
    phase,
    chroma,
    bright,
    contrast,
    degauss,
    encoderSwitch,
    encoderUp,
    encoderDown,
};

#define BKM_POWER \
  {               \
    0x01, 0x10    \
  }
#define BKM_MENU \
  {              \
    0x02, 0x10   \
  }
#define BKM_ENTER \
  {               \
    0x02, 0x20    \
  }
#define BKM_UP \
  {            \
    0x02, 0x40 \
  }
#define BKM_DOWN \
  {              \
    0x02, 0x80   \
  }
#define BKM_ZERO \
  {              \
    0x00, 0x01   \
  }
#define BKM_ONE \
  {             \
    0x00, 0x02  \
  }
#define BKM_TWO \
  {             \
    0x00, 0x04  \
  }
#define BKM_THREE \
  {               \
    0x00, 0x08    \
  }
#define BKM_FOUR \
  {              \
    0x00, 0x10   \
  }
#define BKM_FIVE \
  {              \
    0x00, 0x20   \
  }
#define BKM_SIX \
  {             \
    0x00, 0x40  \
  }
#define BKM_SEVEN \
  {               \
    0x00, 0x80    \
  }
#define BKM_EIGHT \
  {               \
    0x01, 0x01    \
  }
#define BKM_NINE \
  {              \
    0x01, 0x02   \
  }
#define BKM_DEL \
  {             \
    0x01, 0x04  \
  }
#define BKM_ENT \
  {             \
    0x01, 0x08  \
  }
#define BKM_SHIFT \
  {               \
    0x03, 0x01    \
  }
#define BKM_OVERSCAN \
  {                  \
    0x03, 0x02       \
  } // OVERSCAN / 16:9
#define BKM_H_SYNC \
  {                \
    0x03, 0x04     \
  } // HORIZ SYNC VIEW / SYNC
#define BKM_V_SYNC \
  {                \
    0x03, 0x08     \
  } // VERT SYNC VIEW / BLUE ONLY
#define BKM_MONO \
  {              \
    0x03, 0x10   \
  } // MONO / R
#define BKM_APT \
  {             \
    0x04, 0x01  \
  } // APT / G
#define BKM_COMB \
  {              \
    0x04, 0x02   \
  } // COMB / B
#define BKM_F1 \
  {            \
    0x04, 0x04 \
  } // F1 / F3
#define BKM_F3 \
  {            \
    0x04, 0x08 \
  } // F2 / F4
#define BKM_SAFE_AREA \
  {                   \
    0x04, 0x10        \
  } // SAFE AREA / ADDRESS
#define BKM_PHASE \
  {               \
    0x02, 0x08    \
  } // PHASE MANUAL
#define BKM_CHROMA \
  {                \
    0x02, 0x04     \
  } // CHROMA MANUAL
#define BKM_BRIGHT \
  {                \
    0x02, 0x02     \
  } // BRIGHT MANUAL
#define BKM_CONTRAST \
  {                  \
    0x02, 0x01       \
  } // CONTRAST MANUAL
#define BKM_DEGAUSS \
  {                 \
    0x01, 0x20      \
  } // DEGAUSS

// LED Group 2
#define LED_CONTRAST 0x01
#define LED_BRIGHT 0x02
#define LED_CHROMA 0x04
#define LED_PHASE 0x08

// LED Group 3
#define LED_SHIFT 0x01
#define LED_OVERSCAN 0x02
#define LED_H_SYNC 0x04
#define LED_V_SYNC 0x08
#define LED_MONO 0x10

// LED Group 4
#define LED_APT 0x01
#define LED_COMB 0x02
#define LED_F1 0x04
#define LED_F3 0x08
#define LED_SAFE_AREA 0x10

#define ENCODER_GROUP 0x05 // arbitrary new group for rotary encoders

#define IEN_SWITCH      \
  {                     \
    ENCODER_GROUP, 0x00 \
  }
#define IEN_UP          \
  {                     \
    ENCODER_GROUP, 0x01 \
  }
#define IEN_DOWN        \
  {                     \
    ENCODER_GROUP, 0x02 \
  }

const struct Command commands[] = {
    {BKM_POWER, false, 0},
    {BKM_MENU, false, 1},
    {BKM_ENTER, false, 2},
    {BKM_UP, true, 3},
    {BKM_DOWN, true, 4},
    {BKM_ZERO, false, 5},
    {BKM_ONE, false, 6},
    {BKM_TWO, false, 7},
    {BKM_THREE, false, 8},
    {BKM_FOUR, false, 9},
    {BKM_FIVE, false, 10},
    {BKM_SIX, false, 11},
    {BKM_SEVEN, false, 12},
    {BKM_EIGHT, false, 13},
    {BKM_NINE, false, 14},
    {BKM_DEL, true, 15},
    {BKM_ENT, false, 16},
    {BKM_SHIFT, false, 17},
    {BKM_OVERSCAN, false, 18},
    {BKM_H_SYNC, false, 19},
    {BKM_V_SYNC, false, 20},
    {BKM_MONO, false, 21},
    {BKM_APT, false, 22},
    {BKM_COMB, false, 23},
    {BKM_F1, false, 24},
    {BKM_F3, false, 25},
    {BKM_SAFE_AREA, false, 26},
    {BKM_PHASE, false, 27},
    {BKM_CHROMA, false, 28},
    {BKM_BRIGHT, false, 29},
    {BKM_CONTRAST, false, 30},
    {BKM_DEGAUSS, false, 31},
    {IEN_SWITCH, false, 32},
    {IEN_UP, true, 33},
    {IEN_DOWN, true, 34},
};

struct LEDStatus
{
  enum bank currentBank = none;
  int group2LEDMask = 0;
  int group3LEDMask = 0;
  int group4LEDMask = 0;
  bool needsUpdate = false;
  int selectedEncoder = 0;
};

void logLEDs(LEDStatus *leds);
void handleRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat);
void learnRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat);
void sendCode(ControlCode *code);
void updateIsLearning();
void updateState();
bool equals(RemoteKey lhs, RemoteKey rhs);
void powerSave(Timers *timers, bool *displaySleep);
void dumpNames();
void cancelLearning();
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp);
void processLearnQueue();
void handleRotaryEncoderCommand(ControlCode *toSend, bool repeating);
LEDStatus *processControlMessages(Timers *timers, LEDStatus *status);
void processCommandBuffer(CircularBuffer<void *, 4U> *commandBuffer);
void updateLEDS(Adafruit_SSD1306 *display, LEDStatus *leds);
enum bank getBank(byte *b);
void sendEncoder(uint8_t id, int tick);
void processEncoderQueue(CircularBuffer<ControlCode, 4U> *buffer, uint8_t selectedEncoder);
void updateDisplay(Adafruit_SSD1306 *display, int learnIndex, int screenWidth, int screenHeight);

#endif
