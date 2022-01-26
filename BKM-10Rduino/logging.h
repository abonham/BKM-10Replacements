#ifndef __LOGGING_H
#define __LOGGING_H

#include "BKM10Rduino.h"

#ifdef SERIAL_LOGGING
#define TX_EN digitalWrite(TX_ENABLE_PIN, HIGH);
#define TX_D digitalWrite(TX_ENABLE_PIN, LOW);
#endif

void logCommand(ControlCode *cmd, int idx) {
#ifdef SERIAL_LOGGING
  TX_D
  char buffer[20];
  strcpy_P(buffer, (char*)pgm_read_word(&(names[idx])));
  Serial.println("\nWill send command: ");
  Serial.print("0x");
  Serial.print(cmd->code, HEX);
  Serial.print("(");
  Serial.print(buffer);
  Serial.println(")");
  TX_EN
#endif
}

void logCheckBank(byte* b) {
#ifdef SERIAL_LOGGING
  TX_D
  Serial.print(b[0], HEX);
  Serial.print(b[1], HEX);
  Serial.println(b[2], HEX);
  Serial.println((char*)b);
  TX_EN
#endif
}

void logGroupMask(byte* b) {
  #ifdef SERIAL_LOGGING
    TX_D
    Serial.print(F("Group: "));
    Serial.println((unsigned char)b[1], HEX);
    Serial.print(F("Mask: "));
    Serial.println((unsigned char) b[2], HEX);
    TX_EN
#endif
}

void logLEDStatus(int group2, int group3, int group4) {
#ifdef SERIAL_LOGGING
  Serial.println(F("In ILE Bank, process led status: "));
  Serial.println(group2, HEX);
  Serial.println(group3, HEX);
  Serial.println(group4, HEX);
#endif
}

void logSendEncoder(uint8_t id, int tick) {
#ifdef SERIAL_LOGGING
  TX_D
  Serial.println("");
  Serial.print(F("Rotary Encoder: "));
  Serial.print(id);
  Serial.print(" - ");
  Serial.println(tick);
  TX_EN
  delay(10);
#endif  
}

void logUnknownKey(uint16_t aAddress, uint8_t aCommand) {
#ifdef SERIAL_LOGGING
  TX_D
  Serial.print("\nunknown key: ");
  Serial.print(aAddress, HEX);
  Serial.print(" ");
  Serial.println(aCommand, HEX);
  TX_EN
#endif
}

// handle diagnostic informations given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
  // transmit diagnostic informations through serial link.
  Serial.println(__func);
  Serial.println(__file);
  Serial.println(__lineno, DEC);
  Serial.println(__sexp);
  Serial.flush();
  // abort program execution.
  abort();
}

#endif
