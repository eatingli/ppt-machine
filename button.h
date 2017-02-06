//Button 74HC165
#define NUMBER_OF_SHIFT_CHIPS   3
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8
#define PULSE_WIDTH_USEC   20
#define POLL_DELAY_MSEC   1
#define BYTES_VAL_T unsigned long

const byte btnClockEnablePin  = 4;  // Connects to Clock Enable pin the 165
const byte btnDataPin         = 5; // Connects to the Q7 pin the 165
const byte btnClockPin        = 6; // Connects to the Clock pin the 165
const byte btnPloadPin        = 7;  // Connects to Parallel load pin the 165

//讀取按鈕狀態
BYTES_VAL_T read_shift_regs()
{
  long bitVal;
  BYTES_VAL_T bytesVal = 0;

  digitalWrite(btnClockEnablePin, HIGH);
  digitalWrite(btnPloadPin, LOW);
  delayMicroseconds(PULSE_WIDTH_USEC);
  digitalWrite(btnPloadPin, HIGH);
  digitalWrite(btnClockEnablePin, LOW);

  for (int i = 0; i < DATA_WIDTH; i++)
  {
    bitVal = digitalRead(btnDataPin);

    bytesVal |= (bitVal << ((DATA_WIDTH - 1) - i));

    digitalWrite(btnClockPin, HIGH);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(btnClockPin, LOW);
  }

  return (bytesVal);
}
