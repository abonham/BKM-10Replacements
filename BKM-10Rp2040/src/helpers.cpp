#include "BKM10Rduino.h"

// HELPERS

// MARK:- Helpers
bool equals(RemoteKey lhs, RemoteKey rhs)
{
    return lhs.address == rhs.address && lhs.code == rhs.code;
}

void dumpNames()
{
    int s = sizeof(commands) / sizeof(Command);
    for (int i = 0; i < s; i++)
    {
        Serial.println(names[i]);
    }
}

void powerSave(Timers *timers, bool *displaySleep)
{
    if ((millis() - timers->lastInput > SLEEP_TIMER) && !*displaySleep)
    {
        cancelLearning();
        // u8x8.setPowerSave(1);
        *displaySleep = true;
    }
    else if ((millis() - timers->lastInput < SLEEP_TIMER) && displaySleep)
    {
        digitalWrite(RX_ENABLE_LOW_PIN, LOW);
        // u8x8.setPowerSave(0);
        // showName();
        *displaySleep = false;
    }
}

// handle diagnostic informations given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp)
{
    // transmit diagnostic informations through serial link.
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}
