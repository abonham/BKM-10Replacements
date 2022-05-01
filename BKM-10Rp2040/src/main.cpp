// #define SSD1306
#define NO_LED_FEEDBACK_CODE

#include "BKM10Rduino.h"
#include <TinyIRReceiver.h>
#include "IR.h"
#include "store.hpp"
#include "display.h"

extern "C" {
#include "pico.h"
#include "pico/time.h"
#include "pico/bootrom.h"
#include <pico-sdk/rp2_common/hardware_watchdog/include/hardware/watchdog.h>
}
// UART Serial2(0, 1, NC, NC);
// #define CLEAR_STORED_KEYS

#define MENU_BUTTON_PIN 16
#define ENTER_BUTTON_PIN 18
#define UP_BUTTON_PIN 17
#define POWER_BUTTON_PIN 15
#define DOWN_BUTTON_PIN 14
#define RIGHT_BUTTON_PIN 13

#define REBOOT_BUTTON_PIN 22
#define DUMP_JSON_BUTTON_PIN 20

CircularBuffer<void *, 4> commandBuffer;
CircularBuffer<ControlCode, 4> encoderBuffer;

volatile bool learning = false;
volatile bool isHoldingLearnButton = false;
uint8_t learnIndex = 0;

bool displaySleep = false;

enum bank selectedBank = none;
volatile uint8_t selectedEncoder = 0x00;

LEDStatus ledStatus = LEDStatus();
struct RemoteKey *learningInput = (RemoteKey *)malloc(sizeof(RemoteKey));
struct RemoteKey *lastLearnedInput = (RemoteKey *)malloc(sizeof(RemoteKey));
struct ControlCode *lastRemoteInput = (ControlCode *)malloc(sizeof(ControlCode));
struct Timers *timers = (Timers *)malloc(sizeof(Timers));

volatile int lastDebounce = 0;

// #define SH1106
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
TwoWire i2c(4, 5);
#ifdef SSD1306
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c, -1);
#else
#include "Adafruit_SH110X.h"
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c, -1);
#endif
StoreClass store = StoreClass();
const char *keysFileName = MBED_LITTLEFS_FILE_PREFIX "/keys.json";

#define BAUD_RATE 38400

char requestLEDS[] = {0x44, 0x33, 0x31};

String debugMessage = String("");

void setText(const char *msg)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(msg);
  display.display();
}

void setText(String msg) {
  setText(msg.c_str());
}

void setupPins()
{
  pinMode(LEARN_ENABLE_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(IR_INPUT_PIN, INPUT);

  pinMode(TX_ENABLE_PIN, OUTPUT);
  pinMode(RX_ENABLE_LOW_PIN, OUTPUT);
  digitalWrite(TX_ENABLE_PIN, HIGH);
  digitalWrite(RX_ENABLE_LOW_PIN, LOW);

  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  
  pinMode(REBOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DUMP_JSON_BUTTON_PIN, INPUT_PULLUP);
}

void setup()
{
  setupPins();
  if (!gpio_get(LEARN_ENABLE_PIN))
  {
    reset_usb_boot(0, 0);
  }

  Serial.begin(BAUD_RATE);
  Serial1.begin(BAUD_RATE);

  while (gpio_get(LEARN_ENABLE_PIN)) {
    // connect serial then press button to continue boot
  }

#ifdef SSD1306
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
#else
  display.begin();
#endif
  display.clearDisplay();
  display.display();
  display.print("test");
  delay(1000);

  setText("Init");
  store.initFS();
#ifdef CLEAR_STORED_KEYS
  remove(keysFileName);
#endif
  setText("Loading");
  delay(500);
  int err = store.loadKeys(keysFileName);
  delay(100);
  if (err != Ok)
  {
    setText((char *)store.errorMsg(err));
    learning = true;
  }
  else
  {
    updateLEDS(&display, &ledStatus, false);
  }
  delay(500);

  *learningInput = {0, 0};
  *lastLearnedInput = {0, 0};
  *lastRemoteInput = {0, 0};

  attachInterrupt(digitalPinToInterrupt(3), handleIR, CHANGE);

  unsigned long mark = millis();
  timers->lastPoll = mark;
  timers->learnHold = mark;
  timers->lastInput = mark;
  timers->lastRemoteInput = mark;
  Serial1.write("ICC");
  Serial1.write("IMT");
  Serial1.write(requestLEDS);
}

void loop()
{
  // LEDStatus newStatus = processControlMessages(timers, &ledStatus);
  processControlMessages(timers, &ledStatus);
  // logStatus(&newStatus);
  if (ledStatus.needsUpdate)
  {
    logLEDs(&ledStatus);
  }
  processCommandBuffer(&commandBuffer);
  processEncoderQueue(&encoderBuffer, selectedBank);
  updateState();
}

// MARK:- IR receiver interupt handlers
// void handleReceivedTinyIRData(unsigned short address, unsigned char data, bool repeat)
void handleReceivedTinyIRData(uint16_t aAddress, uint8_t aCommand, bool isRepeat)
{
  timers->lastInput = millis();
  if (!learning)
  {
    handleRemoteCommand(aAddress, aCommand, isRepeat);
  }
  else
  {
    learnRemoteCommand(aAddress, aCommand, isRepeat);
  }
}

int commandIndex(RemoteKey key) {
  int x = sizeof(commands) / sizeof(Command);
  for (int i = 0; i < x; i++)
  {
    RemoteKey k = store.getKey(i);
    if (equals(k, key))
    {
      return i;
    }
  }
  return -1;
}

int commandIndex(ControlCode code) {
  int x = sizeof(commands) / sizeof(Command);
  for (int i = 0; i < x; i++)
  {
    Command command = commands[i];
    if (command.cmd.group == code.group && command.cmd.code == code.code)
    {
      return i;
    }
  }
  return -1;
}

void handleRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat)
{
  // debugMessage = debugMessage + "\nNew IR =>" + String(aAddress, HEX) + " " + String(aCommand, HEX);
  
  RemoteKey input = {aAddress, aCommand, 99};

  unsigned long mark = millis();

  int i = commandIndex(input);
  if (i == -1) {
    return;
  }
  ControlCode *toSend = (ControlCode *)&commands[i].cmd;
  if (toSend->group == ENCODER_GROUP)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    handleRotaryEncoderCommand(toSend, isRepeat);
  }
  else if ((!isRepeat || commands[i].repeats) && mark - timers->lastRemoteInput > 200)
  { // remote will set isRepeat when holding down buttons that aren't supposed to "pulse"
    digitalWrite(LED_BUILTIN, HIGH);
    commandBuffer.push(toSend);
    lastRemoteInput = toSend;
    timers->lastRemoteInput = mark;
  }
  return;
}

void handleRotaryEncoderCommand(ControlCode *toSend, bool repeating)
{
  if (toSend->code == 0 && !repeating)
  {
    // Change selected rotary encoder
    int next = selectedEncoder + 1;
    selectedEncoder = next <= 3 ? next : 0;
    ledStatus.selectedEncoder = selectedEncoder;
  }
  else
  {
    ControlCode newCode = {};
    newCode.group = selectedEncoder;
    newCode.code = toSend->code;
    encoderBuffer.push(newCode);
  }
}

// MARK:- Learn remote control
void updateIsLearning()
{
  int learnButtonState = digitalRead(LEARN_ENABLE_PIN);
  if (learnButtonState == HIGH)
  {
    isHoldingLearnButton = false;
  }
  else
  {
    timers->lastInput = millis();
  }

  if (!learning)
  {
    if (learnButtonState == LOW)
    {
      if (!isHoldingLearnButton)
      {
        isHoldingLearnButton = true;
        timers->learnHold = millis();
      }
      else if (millis() - timers->learnHold > LEARN_TIMEOUT)
      {
        learnIndex = 0;
        learning = true;
      }
    }
  }
  else if (learnButtonState == LOW && !isHoldingLearnButton && millis() - timers->learnHold > LEARN_TIMEOUT)
  {
    cancelLearning();
  }
  else
  {
    processLearnQueue();
  }
}

void cancelLearning()
{
#ifdef SERIAL_LOGGING
  Serial.println("cancel learning");
#endif
  learning = false;
  learningInput->address = 0;
  learningInput->code = 0;
  lastLearnedInput->address = 0;
  lastLearnedInput->code = 0;
  learnIndex = 0;
}

void reboot() {
  reset_usb_boot(1<<PICO_DEFAULT_LED_PIN,0);
}

void processLearnQueue()
{
  if (learnIndex >= COMMANDS_SIZE)
  {
    if (store.saveStoredKeys(keysFileName) == 0)
    {
      setText("save ok");
      delay(1000);
    }
    cancelLearning();
    return;
  }

  if (learningInput->address == 0)
  {
    return;
  }

  RemoteKey old = store.getKey(learnIndex);
  Serial.print(learnIndex);

  Serial.print(" - old: (");
  Serial.print(old.address);
  Serial.print(",");
  Serial.print(old.code);
  Serial.print("), ");

  RemoteKey newKey = {learningInput->address, learningInput->code, learnIndex};

  Serial.print("new: (");
  Serial.print(newKey.address);
  Serial.print(",");
  Serial.print(newKey.code);
  Serial.print("), ");

  Serial.print("id: ");
  Serial.println(newKey.id);

  // for (int i = 0; i++; i < learnIndex) {
  //   RemoteKey k = store.getKey(i);
  //   if (equals(newKey, k)) {
  //     Serial.println("existing key");
  //     learningInput->address = 0;
  //     learningInput->code = 0;
  //     return;
  //   }
  // }

  int err = store.putKey(learnIndex, newKey, false);
  if (err != StorageError::Ok)
  {
    Serial.println("failed to save new key");
  }

  lastLearnedInput->address = newKey.address;
  lastLearnedInput->code = newKey.code;
  learningInput->address = 0;
  learningInput->code = 0;
  learnIndex++;
}

void learnRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat)
{
  // Easy ignore for holding ir button too long
  // Don't double process keys that don't send isRepeat, like vol +/-
  if (isRepeat || (aAddress == lastLearnedInput->address && aCommand == lastLearnedInput->code))
  {
    return;
  }
  // Since we are coming from an interupt, don't process if main thread hasn't finished processing last command
  if (learningInput->address)
  {
    return;
  }

  RemoteKey incoming = {aAddress, aCommand, 0};

  if (!equals(incoming, *lastLearnedInput))
  {
    learningInput->address = aAddress;
    learningInput->code = aCommand;
  }
}

void handleButtonState(byte state) {
  digitalWrite(LED_BUILTIN, HIGH);
  if (bitRead(state,0)) {
      commandBuffer.push((ControlCode *)&commands[0].cmd); 
  }
  else if (bitRead(state,1)) {
      commandBuffer.push((ControlCode *)&commands[1].cmd); 
  }
  else if (bitRead(state,2)) {
      commandBuffer.push((ControlCode *)&commands[2].cmd); 
  }
  else if (bitRead(state,3)) {
      commandBuffer.push((ControlCode *)&commands[3].cmd); 
  }
  else if (bitRead(state,4)) {
      commandBuffer.push((ControlCode *)&commands[4].cmd); 
  }
}

void checkPhysicalButtons() {
  if (digitalRead(REBOOT_BUTTON_PIN) == LOW) {
    reset_usb_boot(1<<PICO_DEFAULT_LED_PIN,0);
  }

  if (digitalRead(DUMP_JSON_BUTTON_PIN) == LOW) {
    watchdog_enable(1, 1);
    while(1); 
    // store.loadKeys(keysFileName);
    return;
  }

  ButtonState btnState = {0};

  if (!(millis() - lastDebounce > 100)) {
    return;
  }

  if (digitalRead(POWER_BUTTON_PIN) == LOW) {
    bitSet(btnState.state, 0);
    Serial.println("power on from button");
  }
  if (digitalRead(MENU_BUTTON_PIN) == LOW) {
    bitSet(btnState.state, 1);
    Serial.println("menu from button");
  }
  if (digitalRead(ENTER_BUTTON_PIN) == LOW) {
    bitSet(btnState.state, 2);
    Serial.println("enter from button");
  }
  if (digitalRead(UP_BUTTON_PIN) == LOW) {
    bitSet(btnState.state, 3);
    Serial.println("up from button");
  }
  if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
    bitSet(btnState.state, 4);
    Serial.println("down from button");
  }

  lastDebounce = millis();
  
  if (btnState.state) {
    handleButtonState(btnState.state);
  }
}

void updateState()
{
  unsigned long mark = millis();
  if (mark - timers->lastPoll > POLL_RATE)
  {
    digitalWrite(LED_BUILTIN, LOW);
    checkPhysicalButtons();
    powerSave(timers, &displaySleep);
    updateIsLearning();

    // Do serial console output here as it's too slow to do during IR interupt
    if (debugMessage != "")
    {
      Serial.println(debugMessage);
      debugMessage = "";
    }

    int i = commandIndex(*lastRemoteInput);
    if (learning && (learnIndex < COMMANDS_SIZE))
    {
      updateDisplay(&display, learnIndex, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    else if (i != -1 && mark - timers->lastRemoteInput < 500) {
      setText(String(names[i]));
    }
    else
    {
      updateLEDS(&display, &ledStatus, displaySleep);
    }
    timers->lastPoll = mark;
  }
}
