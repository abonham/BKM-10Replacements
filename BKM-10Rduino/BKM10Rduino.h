#ifndef __BKM_H
#define __BKM_H

#include <Arduino.h>

#define COMMANDS_SIZE 35
#define IR_INPUT_PIN        7
#define TX_ENABLE_PIN       3
#define RX_ENABLE_LOW_PIN   2
#define LEARN_ENABLE_PIN    4

#define TICK_RATE 100

#include "TinyIRReceiver.hpp"
#include <EEPROM.h>
#include <U8g2lib.h>

#ifdef USE_HW_SPI
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#endif

#ifdef USE_HW_I2C
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#endif

/*
   Holds any timestamps required for time based events
*/
struct Timers {
  unsigned long lastPoll;
  unsigned long learnHold;
  unsigned long lastInput;
};

const byte keydown = 0x44;
const char ISWBank[] = "ISW";
const char IENBank[] = "IEN";

struct RemoteKey {
  uint16_t address;
  uint8_t code;
  uint8_t id;
} remoteKey;

#define CONTRAST_ENCODER 0x00
#define BRIGHT_ENCODER 0x01
#define CHROMA_ENCODER 0x02
#define PHASE_ENCODER 0x03

/**
   Provides a convenience subscript accessor to EEPROM for stored key codes

   Treats EEPROM from address 0 as an array equivalent to `RemoteKey keys[]`
*/
struct StoredKey {
  RemoteKey operator [] (int i) {
    RemoteKey k;
    EEPROM.get(i * sizeof(RemoteKey), k);
    return k;
  };
};

/*
   Two byte BKM-10R control code

   Codes are sent over RS485 as `keydown, group, code`
*/
struct ControlCode {
  byte group;
  byte code;
} controlCode;

enum encoder { CONTRAST = CONTRAST_ENCODER, BRIGHTNESS = BRIGHT_ENCODER, CHROMA = CHROMA_ENCODER, PHASE = PHASE_ENCODER };

struct Command {
  struct ControlCode cmd;
  bool repeats;
  uint8_t id;
} command;

const char power[] PROGMEM = "Power";
const char menu[] PROGMEM = "Menu";
const char enter[] PROGMEM = "Enter";
const char up[] PROGMEM = "Up";
const char down[] PROGMEM = "Down";
const char zero[] PROGMEM = "0";
const char one[] PROGMEM = "1";
const char two[] PROGMEM = "2";
const char three[] PROGMEM = "3";
const char four[] PROGMEM = "4";
const char five[] PROGMEM = "5";
const char six[] PROGMEM = "6";
const char seven[] PROGMEM = "7";
const char eight[] PROGMEM = "8";
const char nine[] PROGMEM = "9";
const char del[] PROGMEM = "Del";
const char ent[] PROGMEM = "Ent";
const char shiftSel[] PROGMEM = "Shift";
const char overscan[] PROGMEM = "Overscan/16:9";
const char hsync[] PROGMEM = "HSync/Sync";
const char vsync[] PROGMEM = "VSync/Blue";
const char mono[] PROGMEM = "Mono/R";
const char apt[] PROGMEM = "Apt/G";
const char comb[] PROGMEM = "Comb/B";
const char f1[] PROGMEM = "F1/F2";
const char f3[] PROGMEM = "F3/F4";
const char safe_area[] PROGMEM = "Safe Area/Address";
const char phase[] PROGMEM = "Phase manual";
const char chroma[] PROGMEM = "Chroma manual";
const char bright[] PROGMEM = "Brigtness manual";
const char contrast[] PROGMEM = "Contrast manual";
const char degauss[] PROGMEM = "Degauss";
const char encoderSwitch[] PROGMEM = "Next rotary";
const char encoderUp[] PROGMEM = "Rotary up";
const char encoderDown[] PROGMEM = "Rotary down";

const char * const names[] PROGMEM = {
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

#define BKM_POWER     { 0x01, 0x10 }
#define BKM_MENU      { 0x02, 0x10 }
#define BKM_ENTER     { 0x02, 0x20 }
#define BKM_UP        { 0x02, 0x40 }
#define BKM_DOWN      { 0x02, 0x80 }
#define BKM_ZERO      { 0x00, 0x01 }
#define BKM_ONE       { 0x00, 0x02 }
#define BKM_TWO       { 0x00, 0x04 }
#define BKM_THREE     { 0x00, 0x08 }
#define BKM_FOUR      { 0x00, 0x10 }
#define BKM_FIVE      { 0x00, 0x20 }
#define BKM_SIX       { 0x00, 0x40 }
#define BKM_SEVEN     { 0x00, 0x80 }
#define BKM_EIGHT     { 0x01, 0x01 }
#define BKM_NINE      { 0x01, 0x02 }
#define BKM_DEL       { 0x01, 0x04 }
#define BKM_ENT       { 0x01, 0x08 }

#define BKM_SHIFT     { 0x03, 0x01 }
#define BKM_OVERSCAN  { 0x03, 0x02 } //OVERSCAN / 16:9
#define BKM_H_SYNC    { 0x03, 0x04 } //HORIZ SYNC VIEW / SYNC
#define BKM_V_SYNC    { 0x03, 0x08 } //VERT SYNC VIEW / BLUE ONLY
#define BKM_MONO      { 0x03, 0x10 } //MONO / R

#define BKM_APT       { 0x04, 0x01 } //APT / G
#define BKM_COMB      { 0x04, 0x02 } //COMB / B
#define BKM_F1        { 0x04, 0x04 } //F1 / F3
#define BKM_F3        { 0x04, 0x08 } //F2 / F4
#define BKM_SAFE_AREA { 0x04, 0x10 } //SAFE AREA / ADDRESS

#define BKM_PHASE     { 0x02, 0x08 } //PHASE MANUAL
#define BKM_CHROMA    { 0x02, 0x04 } //CHROMA MANUAL
#define BKM_BRIGHT    { 0x02, 0x02 } //BRIGHT MANUAL
#define BKM_CONTRAST  { 0x02, 0x01 } //CONTRAST MANUAL
#define BKM_DEGAUSS   { 0x01, 0x20 } //DEGAUSS

//LED Group 2
#define LED_CONTRAST 0x01
#define LED_BRIGHT 0x02
#define LED_CHROMA 0x04
#define LED_PHASE 0x08

//LED Group 3
#define LED_SHIFT 0x01
#define LED_OVERSCAN 0x02
#define LED_H_SYNC 0x04
#define LED_V_SYNC 0x08
#define LED_MONO 0x10

//LED Group 4
#define LED_APT 0x01
#define LED_COMB 0x02
#define LED_F1 0x04
#define LED_F3 0x08
#define LED_SAFE_AREA 0x10

#define ENCODER_GROUP 0x05 //arbitrary new group for rotary encoders

#define IEN_SWITCH { ENCODER_GROUP, 0x00 }
#define IEN_UP { ENCODER_GROUP, 0x01 }
#define IEN_DOWN { ENCODER_GROUP, 0x02 }

enum selectedBank {ISW, ILE, IEN, IMT, ICC, DATA, none};
enum serialState {BANK, KEYDOWN, GROUP, MASK};

const struct Command commands[] = {
  { BKM_POWER, false, 0 },
  { BKM_MENU, false, 1},
  { BKM_ENTER, false, 2},
  { BKM_UP, true, 3},
  { BKM_DOWN, true, 4 },
  { BKM_ZERO, false, 5 },
  { BKM_ONE, false, 6 },
  { BKM_TWO, false, 7 },
  { BKM_THREE, false, 8 },
  { BKM_FOUR, false, 9 },
  { BKM_FIVE, false, 10 },
  { BKM_SIX, false, 11 },
  { BKM_SEVEN, false, 12 },
  { BKM_EIGHT, false, 13 },
  { BKM_NINE, false, 14 },
  { BKM_DEL, true, 15 },
  { BKM_ENT, false, 16 },
  { BKM_SHIFT, false, 17 },
  { BKM_OVERSCAN, false, 18 },
  { BKM_H_SYNC, false, 19 },
  { BKM_V_SYNC, false, 20 },
  { BKM_MONO, false, 21 },
  { BKM_APT, false, 22 },
  { BKM_COMB, false, 23 },
  { BKM_F1, false, 24 },
  { BKM_F3, false, 25 },
  { BKM_SAFE_AREA, false, 26 },
  { BKM_PHASE, false, 27 },
  { BKM_CHROMA, false, 28 },
  { BKM_BRIGHT, false, 29 },
  { BKM_CONTRAST, false, 30 },
  { BKM_DEGAUSS, false, 31 },
  { IEN_SWITCH, true, 32 },
  { IEN_UP, true, 33 },
  { IEN_DOWN, true, 34 },
};

/*** Thank you to the anonymous pastebin hero responsible for providing the control codes ***/

/*
  https://pastebin.com/aTUWf33J
  BKM-10R Protocol
  Version 1 - May 16, 2016
  License: CC0

  The BKM-10R is a remote control used for Sony BVM CRT televisions. It is
  required to control some CRTs without built in control, such as the BVM-20F1U.

  The protocol is vaguely similar to the one used to control Sony VCRs:
  ftp://ftp.sgi.com/sgi/video/rld/vidpage/s9pin.html

  This document currently only applies to the CONTROL connection, not to the
  REMOTE 1 input/output, though it's expected that they are similar.

  == Pinout ==

  The remote control and CRT are connected with straight through male-male DE9
  cable.

  Female DB-9, BKM-10R side:
  1. GND
  2. /TXD
  3. RXD
  4. GND
  5. +5V
  6. GND
  7. TXD
  8. /RXD
  9. GND

  Since a straight through cable is used, TXD on the remote side is RXD on the
  CRT side. +5V is supplied from the CRT to power the remote.

  == Protocol ==

  Full duplex RS-422, 38400 baud, 8N1 (the Sony VCR protocol uses odd parity, but
  that seems not to be the case here). If you only want to control the CRT,
  half duplex RS-485 is electrically compatible.

  == Bank ==

  There are multiple "banks" that can be switched via 3 byte commands:
  0x49 0x45 0x4E - "IEN" - encoders
  0x49 0x53 0x57 - "ISW" - switches
  0x49 0x4C 0x45 - "ILE" - leds
  0x49 0x43 0x43 - "ICC" - ??
  0x49 0x4d 0x54 - "IMT" - ??

  The remote lazily switches banks when needed.

  == Keypresses ==

  Each keydown sends 0x44, then group, then mask.
  0x44 <group> <mask>

  The SHIFT key is "just another key" - state is kept on the CRT. Shifted labels
  are listed after non-shifted labels.

  You must be in the "ISW" bank first.

  <group> <mask> - label
  0x03 0x01 - SHIFT
  0x03 0x02 - OVERSCAN / 16:9
  0x03 0x04 - HORIZ SYNC VIEW / SYNC
  0x03 0x08 - VERT SYNC VIEW / BLUE ONLY
  0x03 0x10 - MONO / R
  0x04 0x01 - APT / G
  0x04 0x02 - COMB / B
  0x04 0x04 - F1 / F3
  0x04 0x08 - F2 / F4
  0x04 0x10 - SAFE AREA / ADDRESS
  0x02 0x40 - UP
  0x02 0x80 - DOWN
  0x02 0x10 - MENU
  0x02 0x20 - ENTER
  0x02 0x08 - PHASE MANUAL
  0x02 0x04 - CHROMA MANUAL
  0x02 0x02 - BRIGHT MANUAL
  0x02 0x01 - CONTRAST MANUAL
  0x00 0x01 - 0
  0x00 0x02 - 1
  0x00 0x04 - 2
  0x00 0x08 - 3
  0x00 0x10 - 4
  0x00 0x20 - 5
  0x00 0x40 - 6
  0x00 0x80 - 7
  0x01 0x01 - 8
  0x01 0x02 - 9
  0x01 0x04 - Del
  0x01 0x08 - Ent
  0x01 0x10 - POWER (but see below)
  0x01 0x20 - DEGAUSS

  == POWER ==

  You only need to send a POWER keypress to turn on the monitor, but for some
  reason the remote does more.

  Power sends five 3-byte sequences:
  0x49 0x53 0x57 - switch bank
  0x44 0x01 0x10 - POWER switch
  (1s pause)
  0x49 0x43 0x43 - CC bank?
  0x49 0x4d 0x54 - MT bank?
  0x44 0x33 0x31 - switch?

  CRT responds with:
  0x49 0x43 0x43 - CC bank?
  <full led state report>
  0x49 0x4d 0x54 - MT bank?
  <full led state report>

  == Encoders ==

  Dials are rotary encoders that send 3 bytes:
  0x44 <encoder id id> <ticks>

  Ticks is a two's complement byte indicating how far the dial has turned since
  the last message. The remote sends messages about every 100ms when the dial
  is turning.

  You must be in the "IEN" bank first.

  0x03 - PHASE
  0x02 - CHROMA
  0x01 - BRIGHT
  0x00 - CONTRAST

  == LEDs ==

  LEDs use the same groups and masks as keypresses. Every time the CRT needs to
  change the lights, it changes to the LED bank:

  0x49 0x4c 0x45

  then sends changed switch groups:

  0x44 <group> <mask>

  where <mask> is all of the active LED masks OR'd together.

  == Memory Card ==

  Nobody cares.
*/
#endif
