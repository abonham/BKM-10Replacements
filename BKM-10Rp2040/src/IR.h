#include <TinyIRReceiver.h>
#ifndef IR_INPUT_PIN
#define IR_INPUT_PIN 3
#endif

TinyIRReceiverStruct TinyIRReceiverControl;

void handleReceivedTinyIRData(unsigned short address, unsigned char data, bool repeat);

void handleIR()
{
  uint_fast8_t tIRLevel = digitalRead(IR_INPUT_PIN);

#ifndef NO_LED_FEEDBACK_CODE
  digitalWrite(LED_BUILTIN, tIRLevel);
#endif

  uint32_t tCurrentMicros = micros();
  uint16_t tMicrosOfMarkOrSpace = tCurrentMicros - TinyIRReceiverControl.LastChangeMicros;
  TinyIRReceiverControl.LastChangeMicros = tCurrentMicros;

  uint8_t tState = TinyIRReceiverControl.IRReceiverState;

  if (tIRLevel == LOW)
  {
    /*
     * We have a mark here
     */
    if (tMicrosOfMarkOrSpace > 2 * NEC_HEADER_MARK)
    {
      // timeout -> must reset state machine
      tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
    }
    if (tState == IR_RECEIVER_STATE_WAITING_FOR_START_MARK)
    {
      // We are at the beginning of the header mark, check timing at the next transition
      tState = IR_RECEIVER_STATE_WAITING_FOR_START_SPACE;
    }

    else if (tState == IR_RECEIVER_STATE_WAITING_FOR_FIRST_DATA_MARK)
    {
      if (tMicrosOfMarkOrSpace >= lowerValue25Percent(NEC_HEADER_SPACE) && tMicrosOfMarkOrSpace <= upperValue25Percent(NEC_HEADER_SPACE))
      {
        /*
         * We have a valid data header space here -> initialize data
         */
        TinyIRReceiverControl.IRRawDataBitCounter = 0;
        TinyIRReceiverControl.IRRawData.ULong = 0;
        TinyIRReceiverControl.IRRawDataMask = 1;
        TinyIRReceiverControl.IRRepeatDetected = false;
        tState = IR_RECEIVER_STATE_WAITING_FOR_DATA_SPACE;
      }
      else if (tMicrosOfMarkOrSpace >= lowerValue25Percent(NEC_REPEAT_HEADER_SPACE) && tMicrosOfMarkOrSpace <= upperValue25Percent(NEC_REPEAT_HEADER_SPACE) && TinyIRReceiverControl.IRRawDataBitCounter >= NEC_BITS)
      {
        /*
         * We have a repeat header here and no broken receive before -> set repeat flag
         */
        TinyIRReceiverControl.IRRepeatDetected = true;
        tState = IR_RECEIVER_STATE_WAITING_FOR_DATA_SPACE;
      }
      else
      {
        // This parts are optimized by the compiler into jumps to one code :-)
        // Wrong length -> reset state
        tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
      }
    }

    else if (tState == IR_RECEIVER_STATE_WAITING_FOR_DATA_MARK)
    {
      // Check data space length
      if (tMicrosOfMarkOrSpace >= lowerValue(NEC_ZERO_SPACE) && tMicrosOfMarkOrSpace <= upperValue(NEC_ONE_SPACE))
      {
        // We have a valid bit here
        tState = IR_RECEIVER_STATE_WAITING_FOR_DATA_SPACE;
        if (tMicrosOfMarkOrSpace >= 2 * NEC_UNIT)
        {
          // we received a 1
          TinyIRReceiverControl.IRRawData.ULong |= TinyIRReceiverControl.IRRawDataMask;
        }
        else
        {
          // we received a 0 - empty code for documentation
        }
        // prepare for next bit
        TinyIRReceiverControl.IRRawDataMask = TinyIRReceiverControl.IRRawDataMask << 1;
        TinyIRReceiverControl.IRRawDataBitCounter++;
      }
      else
      {
        // Wrong length -> reset state
        tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
      }
    }
    else
    {
      // error wrong state for the received level, e.g. if we missed one change interrupt -> reset state
      tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
    }
  }
  else
  {
    /*
     * We have a space here
     */
    if (tState == IR_RECEIVER_STATE_WAITING_FOR_START_SPACE)
    {
      /*
       * Check length of header mark here
       */
      if (tMicrosOfMarkOrSpace >= lowerValue25Percent(NEC_HEADER_MARK) && tMicrosOfMarkOrSpace <= upperValue25Percent(NEC_HEADER_MARK))
      {
        tState = IR_RECEIVER_STATE_WAITING_FOR_FIRST_DATA_MARK;
      }
      else
      {
        // Wrong length of header mark -> reset state
        tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
      }
    }

    else if (tState == IR_RECEIVER_STATE_WAITING_FOR_DATA_SPACE)
    {
      // Check data mark length
      if (tMicrosOfMarkOrSpace >= lowerValue(NEC_BIT_MARK) && tMicrosOfMarkOrSpace <= upperValue(NEC_BIT_MARK))
      {
        /*
         * We have a valid mark here, check for transmission complete
         */
        if (TinyIRReceiverControl.IRRawDataBitCounter >= NEC_BITS || TinyIRReceiverControl.IRRepeatDetected)
        {
          /*
           * Code complete -> call callback, no parity check!
           */
          // Reset state for new start
          tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
#if !defined(ARDUINO_ARCH_MBED)
          // interrupts();
#endif
          /*
           * Address reduction to 8 bit
           */
          if (TinyIRReceiverControl.IRRawData.UByte.LowByte == (uint8_t)(~TinyIRReceiverControl.IRRawData.UByte.MidLowByte))
          {
            // standard 8 bit address NEC protocol
            TinyIRReceiverControl.IRRawData.UByte.MidLowByte = 0; // Address is the first 8 bit
          }

          /*
           * Call user provided callback here
           */
          handleReceivedTinyIRData(TinyIRReceiverControl.IRRawData.UWord.LowWord,
                                   TinyIRReceiverControl.IRRawData.UByte.MidHighByte, TinyIRReceiverControl.IRRepeatDetected);
        }
        else
        {
          // not finished yet
          tState = IR_RECEIVER_STATE_WAITING_FOR_DATA_MARK;
        }
      }
      else
      {
        // Wrong length -> reset state
        tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
      }
    }
    else
    {
      // error wrong state for the received level, e.g. if we missed one change interrupt -> reset state
      tState = IR_RECEIVER_STATE_WAITING_FOR_START_MARK;
    }
  }

  TinyIRReceiverControl.IRReceiverState = tState;
}
