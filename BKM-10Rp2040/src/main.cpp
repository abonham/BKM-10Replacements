#include "BKM10Rduino.h"
#include <TinyIRReceiver.h>
#include "IR.h"
#include "store.hpp"
#include "display.h"
#include <pico/bootrom.h>

// UART Serial2(0, 1, NC, NC);
// #define CLEAR_STORED_KEYS

CircularBuffer<void *, 4> commandBuffer;
CircularBuffer<ControlCode, 4> encoderBuffer;

volatile bool learning = false;
volatile bool isHoldingLearnButton = false;
volatile int learnIndex = 0;

bool displaySleep = false;

enum bank selectedBank = none;
volatile uint8_t selectedEncoder = 0x00;

LEDStatus ledStatus = LEDStatus();
struct RemoteKey *learningInput = (RemoteKey *)malloc(sizeof(RemoteKey));
struct RemoteKey *lastLearnedInput = (RemoteKey *)malloc(sizeof(RemoteKey));
struct Timers *timers = (Timers *)malloc(sizeof(Timers));

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
TwoWire i2c(4, 5);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c, -1);
StoreClass store = StoreClass();
const char *keysFileName = MBED_LITTLEFS_FILE_PREFIX "/keys.json";

#define BAUD_RATE 38400

char requestLEDS[] = {0x44, 0x33, 0x31};

void setText(const char *msg)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(msg);
  display.display(); // Show initial text
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

  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.display();
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
    setText("ok");
  }
  delay(500);

  *learningInput = {0, 0};
  *lastLearnedInput = {0, 0};

  attachInterrupt(digitalPinToInterrupt(3), handleIR, CHANGE);

  timers->lastPoll = millis();
  timers->learnHold = millis();
  timers->lastInput = millis();
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

void tell()
{
  Serial.println("TELL");
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

void handleRemoteCommand(uint16_t aAddress, uint8_t aCommand, bool isRepeat)
{
  digitalWrite(LED_BUILTIN, HIGH);
  // String log = String("New IR =>") + String(aAddress, HEX) + String(" ") + String(aCommand, HEX);
  // Serial.println(log);
  RemoteKey input = {aAddress, aCommand, 99};

  int x = sizeof(commands) / sizeof(Command);
  for (int i = 0; i < x; i++)
  {
    RemoteKey k = store.getKey(i);
    if (equals(k, input))
    {
      ControlCode *toSend = (ControlCode *)&commands[i].cmd;
      if (toSend->group == ENCODER_GROUP)
      {
        handleRotaryEncoderCommand(toSend, isRepeat);
      }
      else if (!isRepeat || commands[i].repeats)
      { // remote will set isRepeat when holding down buttons that aren't supposed to "pulse"
        commandBuffer.push(toSend);
      }
      return;
    }
  }
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

void processLearnQueue()
{
  if (learnIndex >= COMMANDS_SIZE)
  {
    if (store.saveStoredKeys(keysFileName) == 0)
    {
      setText("save ok");
    }
    cancelLearning();
    return;
  }

  if (learningInput->address == 0)
  {
    return;
  }

  RemoteKey old = store.getKey(learnIndex);
  Serial.print("old: ");
  Serial.print(old.address, HEX);
  Serial.println(old.code, HEX);

  RemoteKey newKey = {learningInput->address, learningInput->code, (unsigned char)learnIndex};

  Serial.print("new: ");
  Serial.print(newKey.address, HEX);
  Serial.println(newKey.code, HEX);

  int err = store.putKey(newKey, false);
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
  if (isRepeat)
    return;

  // Don't double process keys that don't send isRepeat, like vol +/-
  if (aAddress == lastLearnedInput->address && aCommand == lastLearnedInput->code)
    return;

  // Since we are coming from an interupt, don't process if main thread hasn't finished processing last command
  if (learningInput->address)
    return;

  RemoteKey incoming = {aAddress, aCommand, 0};

  if (!equals(incoming, *lastLearnedInput))
  {
    learningInput->address = aAddress;
    learningInput->code = aCommand;
  }
}

void updateState()
{
  if (millis() - timers->lastPoll > POLL_RATE)
  {
    updateIsLearning();

    if (learning && (learnIndex < COMMANDS_SIZE))
    {
      updateDisplay(&display, learnIndex, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    else
    {
      updateLEDS(&display, &ledStatus);
    }

    timers->lastPoll = millis();
    powerSave(timers, &displaySleep);
  }
}
