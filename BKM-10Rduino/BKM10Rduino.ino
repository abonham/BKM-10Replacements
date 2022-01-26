#define LEARN_TIMEOUT 3000
#define SLEEP_TIMER 300000
#define CIRCULAR_BUFFER_INT_SAFE

//Uncomment to enable output to serial monitor
//Not recommended to enable while connected to BVM
#define SERIAL_LOGGING

#define __ASSERT_USE_STDERR
#define POLL_RATE 16

//Uncomment one of these if using display on HW pins
//#define USE_HW_I2C
//#define USE_HW_SPI

//Uncomment if using I2C display instead of SPI
#define I2C_DISPLAY

#define LARGE_FONT u8x8_font_px437wyse700b_2x2_f
#define MEDIUM_FONT  u8x8_font_7x14_1x2_f
#define SMALL_FONT u8x8_font_chroma48medium8_r

#include "BKM10Rduino.h"
#include <CircularBuffer.h>

#include <assert.h>
#include "logging.h"

CircularBuffer<void*, 4> commandBuffer;

#ifdef USE_PHYSICAL_BUTTONS
volatile int buttonState = 0;
#endif

volatile bool learning = false;
volatile bool isHoldingLearnButton = false;
volatile uint8_t learnIndex = 0;
struct RemoteKey *learningInput = (RemoteKey*)malloc(sizeof(RemoteKey));
struct RemoteKey *lastLearnedInput = (RemoteKey*)malloc(sizeof(RemoteKey));

struct Timers * timers = (Timers*)malloc(sizeof(Timers));
volatile bool displaySleep = false;
volatile bool rs485sleep = false;
volatile uint16_t leds = 0;
volatile enum selectedBank bank = none;
volatile uint8_t selectedEncoder = 0x00;

volatile bool needsStatusUpdate = false;

volatile int group2LEDMask = 0;
volatile int group3LEDMask = 0;
volatile int group4LEDMask = 0;

//subscript accessor for RemoteKeys in EEPROM
StoredKey eeprom;

#ifdef I2C_DISPLAY
U8X8_SSD1306_128X64_NONAME_SW_I2C  u8x8(/*scl=*/10, /*sda=*/9, U8X8_PIN_NONE);
#else
//U8X8_SH1106_128X64_NONAME_4W_HW_SPI u8x8(/* cs=*/ 9, /* dc=*/ 10, /* reset=*/ 12);
U8X8_SH1106_128X64_NONAME_4W_SW_SPI u8x8(13, 11, /* cs=*/ 9, /* dc=*/ 10, /* reset=*/ 8);
#endif

void setup() {
  pinMode(LEARN_ENABLE_PIN, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFlipMode(digitalRead(LEARN_ENABLE_PIN));
  u8x8.setFont(SMALL_FONT);
  showName();
  u8x8.setCursor(0, 7);
  u8x8.print(F(" starting up..."));
  Serial.begin(38400);

  *learningInput = { 0, 0 };
  *lastLearnedInput = { 0, 0 };

  if (!checkROMInit()) {
    erase();
    for (int i = 0; i < COMMANDS_SIZE; i++) {
      RemoteKey k = {(uint16_t)i, (uint8_t)i, (uint8_t)i};
      EEPROM.put(i * sizeof(RemoteKey), k);
    }
    setROMInitFlag(true);
    learning = true;
  } else if (eeprom[0].address && eeprom[0].code && eeprom[0].id) {
    learning = true;
  }

  initPCIInterruptForTinyReceiver();

#ifdef USE_PHYSICAL_BUTTONS
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_DOWN_PIN, INPUT);
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_MENU_PIN, INPUT);
  pinMode(BUTTON_ENTER_PIN, INPUT);
  pinMode(BUTTON_POWER_PIN, INPUT);
#endif

  pinMode(TX_ENABLE_PIN, OUTPUT);
  pinMode(RX_ENABLE_LOW_PIN, OUTPUT);
  digitalWrite(TX_ENABLE_PIN, HIGH);
  digitalWrite(RX_ENABLE_LOW_PIN, LOW);

  timers->lastPoll = millis();
  timers->learnHold = millis();
  timers->lastInput = millis();

#ifdef SERIAL_LOGGING
  //  TX_D
  //  dumpEEPROM();
  //  TX_EN
#endif
  u8x8.clear();
  updateLEDS();
}

void dumpEEPROM() {
  Serial.println("All keys saved in eeprom:\n");
  for (int i = 0; i < COMMANDS_SIZE; i++) {
    RemoteKey k = eeprom[i];
    assert(k.id == i);
    Serial.print("Address: ");
    Serial.print(k.address, HEX);
    Serial.print(", command: ");
    Serial.print(k.code, HEX);
    Serial.print(", id: ");
    Serial.println(k.id, DEC);
  }
}

void showName() {
  u8x8.clear();
  u8x8.home();
  u8x8.setCursor(1, 4);
  u8x8.print(F("BKM-10iRduino"));
  Serial.write("ISW"); //request led status if BVM is already on
}

void loop() {
  updateState();
  processControlMessages();
  processCommandBuffer();
}

void updateState() {
  if (millis() - timers->lastPoll > POLL_RATE) {
    updateIsLearning();

    if (learning && (learnIndex < COMMANDS_SIZE)) {
      displayLearningMessage();
    }

    timers->lastPoll = millis();
    powerSave();
  }
}

void displayLearningMessage() {
  u8x8.drawString(0, 3, "press key:");
  char buffer[18] = {};
  strcpy_P(buffer, (char*)pgm_read_word(&(names[learnIndex])));
  int l = strlen(buffer);

  u8x8.setInverseFont(1);
  u8x8.setCursor(0, 5);
  if (l <= 8) {
    u8x8.setFont(LARGE_FONT);
    for (int i = 0; i < (8 - strlen(buffer)) / 2; i++) {
      u8x8.print(" ");
    }
    u8x8.print(buffer);
    for (int i = 0; i <= (8 - strlen(buffer)) / 2; i++) {
      u8x8.print(" ");
    }
  } else {
    u8x8.setFont(MEDIUM_FONT);
    u8x8.setCursor(0, 5);
    u8x8.print(buffer);
  }
  u8x8.setFont(SMALL_FONT);
  u8x8.setInverseFont(0);
}

enum selectedBank checkBank(byte* b) {
  //  logCheckBank(b);

  if (strcmp((char*)b, "ILE") == 0) {
    return ILE;
  }

  if (strcmp((char*)b, "ISW") == 0) {
    return ISW;
  }

  if (strcmp((char*)b, "IMT") == 0) {
    return IMT;
  }

  if (strcmp((char*)b, "IEN") == 0) {
    return IEN;
  }

  if (strcmp((char*)b, "ICC") == 0) {
    return ICC;
  }

  if ((unsigned char)b[0] == 0x44) {
    //    logGroupMask(b);
    return DATA;
  }

  return none;
}

//MARK:- BKM-10R TX/RX methods
void processControlMessages() {
  if (Serial.available() >= 3) {
    timers->lastInput = millis();
    byte incoming[4];                  //extra char for null termination
    Serial.readBytes(incoming, 3);
    incoming[3] = '\0';
    enum selectedBank packet = checkBank(incoming);
    if ((bank == ILE) && (packet == DATA)) {
      switch (incoming[1]) {
        case 0x02:
          if (group2LEDMask != incoming[2]) {
            needsStatusUpdate = true;
          }
          group2LEDMask = incoming[2];
          break;
        case 0x03:
          if (group3LEDMask != incoming[2]) {
            needsStatusUpdate = true;
          }
          group3LEDMask = incoming[2];
          break;
        case 0x04:
          if (group4LEDMask != incoming[2]) {
            needsStatusUpdate = true;
          }
          group4LEDMask = incoming[2];
          break;
        default:
          break;
      }
      logLEDStatus(group2LEDMask, group3LEDMask, group4LEDMask);
      updateLEDS();
    } else {
      bank = packet;
    }
  }
}

void updateLEDS() {
  if (!needsStatusUpdate || learning || isHoldingLearnButton) {
    return;
  }

  bool shifted = group3LEDMask & LED_SHIFT;

  u8x8.setFont(MEDIUM_FONT);
  u8x8.setCursor(0, 6);
  u8x8.setInverseFont(group2LEDMask & LED_PHASE);
  u8x8.print("Ph");
  u8x8.setCursor(3, 6);
  u8x8.setInverseFont(group2LEDMask & LED_CHROMA);
  u8x8.print("Ch");
  u8x8.setCursor(6, 6);
  u8x8.setInverseFont(group2LEDMask & LED_BRIGHT);
  u8x8.print("Br");
  u8x8.setCursor(9, 6);
  u8x8.setInverseFont(group2LEDMask & LED_CONTRAST);
  u8x8.print("Co");

  u8x8.setCursor(0, 0);
  u8x8.setInverseFont(group3LEDMask & LED_SHIFT);
  u8x8.print("Sh");
  u8x8.setCursor(3, 0);
  u8x8.setInverseFont(group3LEDMask & LED_OVERSCAN);
  u8x8.print(shifted ? "16" : "Ov");
  u8x8.setCursor(6, 0);
  u8x8.setInverseFont(group3LEDMask & LED_H_SYNC);
  u8x8.print(shifted ? "Sy" : "Hs");
  u8x8.setCursor(9, 0);
  u8x8.setInverseFont(group3LEDMask & LED_V_SYNC);
  u8x8.print(shifted ? "B*" : "Vs");
  u8x8.setCursor(12, 0);
  u8x8.setInverseFont(group3LEDMask & LED_MONO);
  u8x8.print(shifted ? "R" : "Mo");

  u8x8.setCursor(0, 3);
  u8x8.setInverseFont(group4LEDMask & LED_APT);
  u8x8.print(shifted ? "G" : "Ap");
  u8x8.setCursor(3, 3);
  u8x8.setInverseFont(group4LEDMask & LED_COMB);
  u8x8.print(shifted ? "B" : "Cb");
  u8x8.setCursor(6, 3);
  u8x8.setInverseFont(group4LEDMask & LED_F1);
  u8x8.print(shifted ? "F2" : "F1");
  u8x8.setCursor(9, 3);
  u8x8.setInverseFont(group4LEDMask & LED_F3);
  u8x8.print(shifted ? "F4" : "F3");
  u8x8.setCursor(12, 3);
  u8x8.setInverseFont(group4LEDMask & LED_SAFE_AREA);
  u8x8.print(shifted ? "Ad" : "Sa");

  u8x8.setCursor(12, 6);
  u8x8.setInverseFont(0);
  switch (selectedEncoder) {
    case 0:
      u8x8.print("EnC");
      break;
    case 1:
      u8x8.print("EnB");
      break;
    case 2:
      u8x8.print("EnY");
      break;
    case 3:
      u8x8.print("EnP");
      break;
    default:
      u8x8.print("En?");
  }

  needsStatusUpdate = false;
}

void processCommandBuffer() {
  while (!commandBuffer.isEmpty()) {
    ControlCode *code = (ControlCode*)commandBuffer.shift();
    sendCode(code);
  }
}

void sendCode(ControlCode *code) {
  Serial.write(ISWBank);
  Serial.write(keydown);
  Serial.write(code->group);
  Serial.write(code->code);
}

void sendEncoder(uint8_t id, int tick) {
  Serial.write(IENBank);
  Serial.write(keydown);
  Serial.write((byte)id);
  Serial.write((byte)tick);
  logSendEncoder(id, tick);
}

//MARK:- IR receiver interupt handlers
void handleReceivedTinyIRData(uint16_t aAddress, uint8_t aCommand, bool isRepeat) {
  timers->lastInput = millis();
  if (!learning) {
    handleRemoteCommand(aAddress, aCommand, isRepeat);
  } else {
    learnRemoteCommand(aAddress, aCommand, isRepeat);
  }
}

void handleRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat) {
  RemoteKey input = { aAddress, aCommand, 99 };

  int x = sizeof(commands) / sizeof(Command);
  for (int i = 0; i < x; i++) {
    RemoteKey k = eeprom[i];
    if (equals(k, input)) {
      ControlCode *toSend = (ControlCode*)&commands[i].cmd;
      if (toSend->group == ENCODER_GROUP) {
        handleRotaryEncoderCommand(toSend, isRepeat);
      } else if (!isRepeat || commands[i].repeats) { //remotes set isRepeat when holding down a button
        commandBuffer.push(toSend);
        logCommand(toSend, i);
      }
      return;
    }
  }
  logUnknownKey(aAddress, aCommand);
}

void handleRotaryEncoderCommand(ControlCode *toSend, bool repeating) {
  logSendEncoder(toSend->group, toSend->code);
  if (toSend->code == 0) {
    //Change selected rotary encoder
    if (repeating) {
      return;
    }
    needsStatusUpdate = true;
    int next = selectedEncoder + 1;
    selectedEncoder = next <= 3 ? next : 0;
    updateLEDS();
    Serial.println("switch enc");
  } else if (toSend->code == 1) {
    //Send positive tick for selected encoder
    Serial.println("enc 1");
    sendEncoder(selectedEncoder, TICK_RATE);
  } else if (toSend->code == 2) {
    //Send negative tick for selected encoder
    Serial.println("enc 2");
    sendEncoder(selectedEncoder, -TICK_RATE);
  }
}

//MARK:- Learn remote control
void updateIsLearning() {
  int learnButtonState = digitalRead(LEARN_ENABLE_PIN);
  if (learnButtonState == HIGH) {
    isHoldingLearnButton = false;
  } else {
    timers->lastInput = millis();
  }

  if (!learning) {
    if (learnButtonState == LOW) {
      if (!isHoldingLearnButton) {
        u8x8.clear();
        u8x8.setCursor(0, 2);
        u8x8.print(F("Hold to start"));
        u8x8.setCursor(0, 3);
        u8x8.print(F("learning remote"));
        isHoldingLearnButton = true;
        timers->learnHold = millis();
      } else if (millis() - timers->learnHold > LEARN_TIMEOUT) {

#ifdef SERIAL_LOGGING
        dumpNames();
#endif
        u8x8.clear();
        learnIndex = 0;
        learning = true;
      }
    }
  } else if (learnButtonState == LOW && !isHoldingLearnButton && millis() - timers->learnHold > LEARN_TIMEOUT) {
    cancelLearning();
  } else {
    processLearnQueue();
  }
}

void cancelLearning() {
#ifdef SERIAL_LOGGING
  TX_D
  Serial.println("cancel learning");
  TX_EN
#endif
  learning = false;
  learningInput->address = 0;
  learningInput->code = 0;
  lastLearnedInput->address = 0;
  lastLearnedInput->code = 0;
  learnIndex = 0;
  u8x8.clear();
  needsStatusUpdate = true;
}

void processLearnQueue() {
  u8x8.setCursor(0, 2);
  u8x8.print(learnIndex + 1, DEC);
  u8x8.print(" of ");
  u8x8.print(COMMANDS_SIZE);
  if (learnIndex >= COMMANDS_SIZE) {
    cancelLearning();
    return;
  }

  if (learningInput->address == 0) {
    return;
  }

#ifdef SERIAL_LOGGING
  TX_D
  RemoteKey old = eeprom[learnIndex];
  Serial.print("old: ");
  Serial.print(old.address, HEX);
  Serial.println(old.code, HEX);
  TX_EN
#endif

  RemoteKey newKey = { learningInput->address, learningInput->code, learnIndex };

#ifdef SERIAL_LOGGING
  TX_D
  Serial.print("new: ");
  Serial.print(newKey.address, HEX);
  Serial.println(newKey.code, HEX);
  TX_EN
#endif

  EEPROM.put(learnIndex * sizeof(RemoteKey), newKey);

  lastLearnedInput->address = newKey.address;
  lastLearnedInput->code = newKey.code;
  learningInput->address = 0;
  learningInput->code = 0;
  learnIndex++;
  u8x8.clear();
}

void learnRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat) {
  //Easy ignore for holding ir button too long
  if (isRepeat) return;

  //Don't double process keys that don't send isRepeat, like vol +/-
  if (aAddress == lastLearnedInput->address && aCommand == lastLearnedInput->code) return;

  //Since we are coming from an interupt, don't process if main thread hasn't finished processing last command
  if (learningInput->address) return;

  RemoteKey incoming = { aAddress, aCommand, 0 };

  if (!equals(incoming, *lastLearnedInput)) {
    learningInput->address = aAddress;
    learningInput->code = aCommand;
  }
}

//MARK:- Helpers
enum encoder nextEncoder() {
  switch (selectedEncoder) {
    case CONTRAST:
          return BRIGHTNESS;
      case BRIGHTNESS:
        return CHROMA;
      case CHROMA:
        return PHASE;
      case PHASE:
        return CONTRAST;
      default:
        return PHASE;
    }
  }

  bool equals(RemoteKey lhs, RemoteKey rhs) {
  return lhs.address == rhs.address && lhs.code == rhs.code;
}

void dumpNames() {
  int s = sizeof(commands) / sizeof(Command);
  for (int i = 0; i < s; i++) {
    char buffer[20];
    #pragma warning disable
    strcpy_P(buffer, (char*)pgm_read_word(&(names[i])));
    #pragma warning restore
    Serial.println(buffer);
  }
}

void powerSave() {
  if ((millis() - timers->lastInput > SLEEP_TIMER) && !displaySleep) {
    setLowPowerMode(true);
  } else if ((millis() - timers->lastInput < SLEEP_TIMER) && displaySleep) {
    setLowPowerMode(false);
  }
}

void setLowPowerMode(bool enable) {
  if (enable) {
    cancelLearning();
    u8x8.setPowerSave(1);
    displaySleep = true;
  } else {
    u8x8.setPowerSave(0);
    displaySleep = false;
    needsStatusUpdate = true;
    updateLEDS();
  }
}

//MARK:- EEPROM
bool checkROMInit() {
  return EEPROM.read(EEPROM.length() - 1) == 0xAA;
}

void setROMInitFlag(bool value) {
  int toWrite = value ? 0xAA : 0x00;
  EEPROM.write(EEPROM.length() - 1, toWrite);
}

void erase(void)
{
  for (int i = 0 ; i < EEPROM.length() ; i++)
    EEPROM.write(i, 0);
}
