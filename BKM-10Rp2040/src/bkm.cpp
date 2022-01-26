#include "BKM10Rduino.h"

void logLEDs(LEDStatus *leds)
{
    Serial.print("LEDS => ");
    Serial.println();
    Serial.print("BANK: ");
    Serial.print(leds->currentBank);
    Serial.println();
    Serial.println("Groups: ");
    Serial.print("Group 2: ");
    Serial.println(leds->group2LEDMask, HEX);
    Serial.print("Group 3: ");
    Serial.println(leds->group3LEDMask, HEX);
    Serial.print("Group 4: ");
    Serial.println(leds->group4LEDMask, HEX);
    Serial.print("Needs update? ");
    Serial.println(leds->needsUpdate);
}

// MARK:- BKM-10R TX/RX methods
LEDStatus *processControlMessages(Timers *timers, LEDStatus *status)
{
    // LEDStatus updated = {};
    // updated.group2LEDMask = status->group2LEDMask;
    // updated.group3LEDMask = status->group3LEDMask;
    // updated.group4LEDMask = status->group4LEDMask;
    if (Serial1.available() >= 3)
    {
        Serial.println("serial available");
        timers->lastInput = millis();
        byte incoming[4]; // extra char for null termination
        Serial1.readBytes(incoming, 3);
        incoming[3] = '\0';
        for (auto b : incoming)
        {
            Serial.print(b, HEX);
            Serial.print(" ");
        }
        Serial.println((char *)incoming);
        enum bank packet = getBank(incoming);
        if ((status->currentBank == ILE) && (packet == DATA))
        {
            switch (incoming[1])
            {
            case 2:
                status->group2LEDMask = incoming[2];
                break;
            case 3:
                status->group3LEDMask = incoming[2];
                break;
            case 4:
                status->group4LEDMask = incoming[2];
                break;
            default:
                break;
            }
        }
        else
        {
            status->currentBank = packet;
        }
        logLEDs(status);
    }
    return status;
}

void sendCode(ControlCode *code)
{
    Serial1.write(ISWBank);
    Serial1.write(keydown);
    Serial1.write(code->group);
    Serial1.write(code->code);
    // uart_write_blocking(uart0, (uint8_t *)ISWBank, 3);
    // Serial1.write(keydown);
    // Serial1.write(code->group);
    // Serial1.write(code->code);
}

void sendEncoder(uint8_t id, int tick)
{
    Serial1.write(IENBank);
    Serial1.write(keydown);
    Serial1.write((byte)id);
    Serial1.write((byte)tick);
    // logSendEncoder(id, tick);
}

void processCommandBuffer(CircularBuffer<void *, 4U> *commandBuffer)
{
    while (!commandBuffer->isEmpty())
    {
        ControlCode *code = (ControlCode *)commandBuffer->shift();
        sendCode(code);
    }
}

void processEncoderQueue(CircularBuffer<ControlCode, 4U> *buffer, uint8_t selectedEncoder)
{
    while (!buffer->isEmpty())
    {
        ControlCode code = buffer->shift();
        if (code.code == 1)
        {
            Serial.print("encoder: ");
            Serial.println(code.group);
            // Send positive tick for selected encoder
            sendEncoder(code.group, TICK_RATE);
        }
        else if (code.code == 2)
        {
            // Send negative tick for selected encoder
            sendEncoder(code.group, -TICK_RATE);
        }
    }
}
