
//index
const byte BTN_MODE[4] = {3, 1, 11, 9};
const byte BTN_SET[2] = {19, 17};
const byte BTN_FN[6] = {2, 0, 10, 8, 18, 16};
const byte BTN_PART[8] = {4, 5, 6, 7, 12, 13, 14, 15};
const byte BTN_COMMON[2] = {22, 23};

const byte LED_B_MODE[4] = {16, 19, 21, 23};
const byte LED_B_STATUS[2] = {3, 7};
const byte LED_B_PART[8] = {9, 11, 13, 15, 25, 27, 29, 31};
const byte LED_B_COMMON[2] = {1, 5};

const byte LED_R_MODE[4] = {17, 18, 20, 22};
const byte LED_R_STATUS[2] = {2, 6};
const byte LED_R_PART[8] = {8, 10, 12, 14, 24, 26, 28, 30};
const byte LED_R_COMMON[2] = {0, 4};

//模式
const String MODE_STRING[4] = {"Ctrl", "Edit", "Shift", "Config"};
byte currentMode = 0;

//組合(Record)
const byte SET_LENGTH = 25;
byte currentSet = 0;

//頁數
const short MAX_PAGE = 999;
short setsPage[8][SET_LENGTH] = {}; //各set頁數 (預設值應該設為1)
short commonsPage[2] = {}; //共用頁數

//LED狀態 0關 1開 2閃爍 3一閃
byte ledsStatus[32] = {};

//LED閃爍
const int LED_BLINK_PERIOD = 400;
const int LED_BLINK_TIME = 350;
unsigned long lastBlankTime = 0;

//LED一閃
const int LED_ONE_BLINK_TIME = 80;
unsigned long oneBlinkTime[32] = {}; //一閃發光時間

//LCD顯示
const int LCD_UPDATE_PERIOD = 50;
unsigned long lastLcdUpdate = 0;

//LCD高亮
const int LCD_BLINK_TIME = 300;
unsigned long lcdHighLightTime = 0;
String lcdHighLightStatus1 = "";
String lcdHighLightStatus2 = "";


//ctrl
short inputPage = 0; //數字鍵盤輸入的控制頁數

//edit
byte editingPart = 0; //0~7個別 8,9共通

//shift
boolean shihtAble[8] = {true, true, true, true, true, true, true, true};

//Keyboard
#include "ctrlPPT.h"


//Keypad
#include "Keypad.h"
const byte KEYPAD_PIN = A0;
const byte KEYPAD_NUMBER[17] = {0, 1, 2, 3, 0 , 4 , 5 , 6 , 0 , 7 , 8 , 9, 0, 0, 0, 0, 0};

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

BYTES_VAL_T pinValues;
BYTES_VAL_T oldPinValues;

//Lcd 74LS164
#include <Wire.h>
#include <LiquidCrystal_SR.h>
LiquidCrystal_SR lcd(2, 3, TWO_WIRE);

//Led 74HC595
const byte dataPin = 10;
const byte latchPin = 9;
const byte clockPin = 8;

//按鈕: 讀取狀態
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

//按鈕:透過index來檢查該按鈕是否按下
boolean isClickDown(byte index) {
  return ((pinValues >> index) & 1) && !((oldPinValues >> index) & 1);
}

//EEPROM
#include <EEPROM.h>

//EEPROM 讀寫Short
void EEPROMWriteShort(int address, short value)
{
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);

  EEPROM.write(address * 2, two);
  EEPROM.write(address * 2 + 1, one);
}

short EEPROMReadShort(int address)
{
  long two = EEPROM.read(address * 2);
  long one = EEPROM.read(address * 2 + 1);

  return ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
}

//寫入指定的Set資料
void saveSetData(byte setIndex)
{
  int addr = setIndex * 8;
  for (byte i = 0; i < 8; i++) {
    EEPROMWriteShort(addr++, setsPage[i][setIndex]);
  }
}

void saveCommonData()
{
  int addr = SET_LENGTH * 8;
  EEPROMWriteShort(addr++, commonsPage[0]);
  EEPROMWriteShort(addr++, commonsPage[1]);
}

//存檔組合
void saveCurrentData()
{
  saveSetData(currentSet);
  saveCommonData();
}

void saveData()
{
  int addr = 0;

  //寫入Part資料
  for (byte i = 0; i < SET_LENGTH; i++) {
    for (byte j = 0; j < 8; j++) {
      EEPROMWriteShort(addr++, setsPage[j][i]);
    }
  }

  //寫入Common資料
  EEPROMWriteShort(addr++, commonsPage[0]);
  EEPROMWriteShort(addr++, commonsPage[1]);
}

void readData()
{
  int addr = 0;

  //讀取Part資料
  for (byte i = 0; i < SET_LENGTH; i++) {
    for (byte j = 0; j < 8; j++) {
      setsPage[j][i] = EEPROMReadShort(addr++);
    }
  }

  //讀取Common資料
  commonsPage[0] = EEPROMReadShort(addr++);
  commonsPage[1] = EEPROMReadShort(addr++);
}

void clearDada() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

//高亮LCD
void highLightLcd(String status1, String status2, int highLightTime) {
  lcdHighLightStatus1 = status1;
  lcdHighLightStatus2 = status2;
  lcdHighLightTime = millis() + highLightTime;
}

//閃爍LED
void oneBlinkLed(byte index, int blinkTime) {
  ledsStatus[index] = 3;
  oneBlinkTime[index] = millis() + blinkTime;
}

//處理按鈕事件
void btnEvent(unsigned long nowTime) {
  //Mode 按鈕
  for (byte i = 0; i < 4; i++) {
    if (isClickDown(BTN_MODE[i])) {
      //更新Mode Led
      //ledsStatus[LED_B_MODE[currentMode]] = 0;
      //ledsStatus[LED_B_MODE[i]] = 1;
      //修改當前Mode
      currentMode = i;
      Serial.println("Mode: " + String(i));

      //重置閃爍
      lastBlankTime = nowTime;
    }
  }

  //Set 按鈕, 增減currentSet
  if (isClickDown(BTN_SET[0]) && currentSet > 0) {
    currentSet--;
    Serial.print("currentSet: ");
    Serial.println(currentSet);
  }
  if (isClickDown(BTN_SET[1]) && currentSet < SET_LENGTH - 1) {
    currentSet++;
    Serial.print("currentSet: ");
    Serial.println(currentSet);
  }

  //FN按鈕
  if (isClickDown(BTN_FN[0])) {
    Serial.println("FN-0");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-3);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:PageUp", LCD_BLINK_TIME);
        break;
      case 1:
        //單獨減1
        if (editingPart < 8) {
          if (setsPage[editingPart][currentSet] > 0) setsPage[editingPart][currentSet] --;
        } else {
          if (commonsPage[editingPart - 8] > 0) commonsPage[editingPart - 8] --;
        }
        //儲存
        saveCurrentData();
        break;
      case 2:
        //群體減1
        for (byte i = 0; i < 8; i++) {
          int page = setsPage[i][currentSet];
          if (shihtAble[i] && page > 0) setsPage[i][currentSet] = page - 1;
        }
        //儲存
        saveCurrentData();
        break;
    }
  }
  if (isClickDown(BTN_FN[1])) {
    Serial.println("FN-1");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-4);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:PageDown", LCD_BLINK_TIME);
        break;
      case 1:
        //單獨加1
        if (editingPart < 8) {
          if (setsPage[editingPart][currentSet] < MAX_PAGE) setsPage[editingPart][currentSet] ++;
        } else {
          if (commonsPage[editingPart - 8] < MAX_PAGE) commonsPage[editingPart - 8] ++;
        }
        //儲存
        saveCurrentData();
        break;
      case 2:
        //群體加1
        for (byte i = 0; i < 8; i++) {
          int page = setsPage[i][currentSet];
          if (shihtAble[i] && page > 0 && page < MAX_PAGE) setsPage[i][currentSet] = page + 1;
        }
        //儲存
        saveCurrentData();
        break;
    }
  }
  if (isClickDown(BTN_FN[2])) {
    Serial.println("FN-2");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-5);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Black", LCD_BLINK_TIME);
        break;
      case 1:
      case 2:
        highLightLcd("", "Save...", LCD_BLINK_TIME);
        saveData();
        break;
    }
  }
  if (isClickDown(BTN_FN[3])) {
    Serial.println("FN-3");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-6);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Write", LCD_BLINK_TIME);
        break;
    }
  }
  if (isClickDown(BTN_FN[4])) {
    Serial.println("FN-4");
    switch (currentMode) {
      case 0:
        keyControl(-1);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Start", LCD_BLINK_TIME);
        break;
    }
  }
  if (isClickDown(BTN_FN[5])) {
    Serial.println("FN-2");
    switch (currentMode) {
      case 0:
        keyControl(-2);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Stop", LCD_BLINK_TIME);
        break;
    }
  }

  //part按鈕
  for (byte i = 0; i < 8; i++) {
    if (isClickDown(BTN_PART[i])) {
      short page = setsPage[i][currentSet];
      switch (currentMode) {
        case 0:
          //送出控制
          Serial.println("CtrlPart: " + String(page));

          //頁數大於0檢查
          if (page > 0) {
            keyPage(page);
            oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
            highLightLcd("", "GoTo:" + String(page), LCD_BLINK_TIME);
          } else {
            highLightLcd("", "Enpty", LCD_BLINK_TIME);
          }
          break;
        case 1:
          //切換editingPart
          editingPart = i;
          Serial.print("editingPart: ");
          Serial.println(editingPart);
          break;
        case 2:
          //平移開關
          shihtAble[i] = !shihtAble[i];
          break;
      }
    }
  }

  //Common按鈕
  for (byte i = 0; i < 2; i++) {
    if (isClickDown(BTN_COMMON[i])) {
      short page = commonsPage[i];
      switch (currentMode) {
        case 0:
          //送出控制
          Serial.println("CtrlCommonPart: " + String(page));

          //頁數大於0檢查
          if (page > 0) {
            keyPage(page);
            oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
            highLightLcd("", "GoTo:" + String(page), LCD_BLINK_TIME);
          } else {
            highLightLcd("", "Enpty", LCD_BLINK_TIME);
          }
          break;
        case 1:
          //切換編輯中按鈕
          editingPart = 8 + i;
          Serial.print("editingPart: ");
          Serial.println(editingPart);
          break;
        case 2:
          //切換平移開關
          break;
      }
    }
  }

}

void keypadEvent()
{
  //讀取Keypad
  byte key = readPush(KEYPAD_PIN);
  if (key == 0) return;

  //根據Mode和 按下的按鈕決定動作
  switch (currentMode) {
    case 0:
      //控制inputPage
      switch (key) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 14:
          //數字鍵
          if (inputPage == MAX_PAGE) {
            inputPage = KEYPAD_NUMBER[key];
          } else {
            inputPage = inputPage * 10 + KEYPAD_NUMBER[key];
            if (inputPage > MAX_PAGE) inputPage = MAX_PAGE;
          }
          break;
        case 13:
          //減
          if (--inputPage < 0) inputPage = 0;
          break;
        case 15:
          //加
          if (++inputPage > MAX_PAGE) inputPage = MAX_PAGE;
          break;
        case 4:
          break;
        case 8:
          //退後
          inputPage /= 10;
          break;
        case 12:
          //清除
          inputPage = 0;
          break;
        case 16:
          //Enter
          if (inputPage > 0) {
            Serial.println("Input Ctrl " + String(inputPage));
            keyPage(inputPage);
            oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
            inputPage = 0;
          }
          break;
      }
      break;
    case 1:
      //編輯頁數
      short newPage = editingPart < 8 ? setsPage[editingPart][currentSet] : commonsPage[editingPart - 8];
      switch (key) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 14:
          //數字
          newPage = newPage * 10 + KEYPAD_NUMBER[key];
          if (newPage > MAX_PAGE) newPage = MAX_PAGE;
          break;
        case 13:
          //減
          if (--newPage < 0) newPage = 0;
          break;
        case 15:
          //加
          if (++newPage > MAX_PAGE) newPage = MAX_PAGE;
          break;
        case 4:
          //倒退
          newPage /= 10;
          break;
        case 8:
          //清除
          newPage = 0;
          break;
        case 12:
          break;
        case 16:
          //Enter 切到下一個part
          if (++editingPart > 9) editingPart = 0;
          return; //直接return掉
      }

      if (editingPart < 8)
        setsPage[editingPart][currentSet] = newPage;
      else
        commonsPage[editingPart - 8] = newPage;
      //儲存
      saveCurrentData();
      break;
  }
}

//更新Led狀態
void updateLed(unsigned long nowTime)
{
  //閃爍判定
  unsigned long diffTime = nowTime - lastBlankTime;
  boolean isLight = diffTime < LED_BLINK_TIME;
  boolean isBlink = diffTime > LED_BLINK_TIME + LED_BLINK_PERIOD;
  if (isBlink) lastBlankTime = nowTime;

  //將狀態轉為byte格式
  byte byteLedsStatus[4] = {};
  for (byte i = 0; i < 32; i++) {
    byte index = i / 8;
    switch (ledsStatus[i]) {
      case 0:
        //關
        byteLedsStatus[index] = (byteLedsStatus[index] << 1);
        break;
      case 1:
        //亮
        byteLedsStatus[index] = (byteLedsStatus[index] << 1) + 1;
        break;
      case 2:
        //閃
        byteLedsStatus[index] = isLight ? (byteLedsStatus[index] << 1) + 1 : (byteLedsStatus[index] << 1);
        break;
      case 3:
        //一閃
        if (nowTime < oneBlinkTime[i]) {
          byteLedsStatus[index] = (byteLedsStatus[index] << 1) + 1;
        } else {
          byteLedsStatus[index] = (byteLedsStatus[index] << 1);
          ledsStatus[i] = 0;
        }
        break;
    }

  }
  //這個之後可以獨立成一個函式
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[0]); //0~7
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[1]); //8~15
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[2]); //16~23
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[3]); //24~31
  digitalWrite(latchPin, HIGH);
}

//更新Lcd狀態
void updateLcd(unsigned long nowTime)
{
  //判斷時間是否達到更新週期
  if (nowTime - lastLcdUpdate < LCD_UPDATE_PERIOD) return;
  lastLcdUpdate = nowTime;

  //Clear
  lcd.clear();

  //左上角Set(Record)
  lcd.home();
  lcd.print("Record-");
  lcd.print(currentSet + 1);

  //右上角Mode
  //lcd.setCursor(16 - String(currentMode).length(), 0);
  lcd.setCursor(16 - MODE_STRING[currentMode].length(), 0);
  lcd.print(MODE_STRING[currentMode]);

  //下排
  switch (currentMode) {
    case 0:
      //inputPage
      if (inputPage > 0) {
        lcd.setCursor(0, 1);
        lcd.print("Input:" + String(inputPage));
      }
      break;
    case 1:
      //editingPart頁數
      lcd.setCursor(0, 1);
      if (editingPart < 8)
        lcd.print("Part_" + String(editingPart + 1) + ":" + (setsPage[editingPart][currentSet] > 0 ? String(setsPage[editingPart][currentSet]) : "_"));
      else
        lcd.print("Common_" + String(editingPart - 7) + ":" + (commonsPage[editingPart - 8] > 0 ? String(commonsPage[editingPart - 8]) : "_"));
      break;
    case 2:
      //計算最小頁數
      unsigned int minValue = MAX_PAGE;
      for (byte i = 0; i < 8; i++) {
        if (setsPage[i][currentSet] > 0)  minValue = min(minValue, setsPage[i][currentSet]);
      }
      //顯示最小頁數
      lcd.setCursor(0, 1);
      lcd.print("First:");
      lcd.print(minValue > 0 && minValue < MAX_PAGE ? String(minValue) : "_");
      break;
  }

  //Lcd High Light
  if (lcdHighLightTime > nowTime) {
    lcd.setCursor(0, 0);
    lcd.print(lcdHighLightStatus1);
    lcd.setCursor(0, 1);
    lcd.print(lcdHighLightStatus2);
  }
}

void setup()
{
  Serial.begin(9600);
  //clearDada();
  readData();

  //BUTTON
  pinMode(btnPloadPin, OUTPUT);
  pinMode(btnClockEnablePin, OUTPUT);
  pinMode(btnClockPin, OUTPUT);
  pinMode(btnDataPin, INPUT);

  digitalWrite(btnClockPin, LOW);
  digitalWrite(btnPloadPin, HIGH);
  pinValues = read_shift_regs();
  oldPinValues = pinValues;

  //LCD
  lcd.begin(16, 2);

  //LED
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

}

void loop()
{
  unsigned long nowTime = millis();

  //讀取按鈕，更新狀態 or 執行控制 (根據Mode，按鈕會有不同功能)
  oldPinValues = pinValues;
  pinValues = read_shift_regs();
  btnEvent(nowTime);
  delay(POLL_DELAY_MSEC);

  keypadEvent();


  //更新Led顯示(在這裡更新效能差了點，不過可以節省很多程式碼)
  //ModeLed
  for (byte j = 0; j < 4; j++) {
    ledsStatus[LED_B_MODE[j]] = (j == currentMode ? 1 : 0);
  }
  //PartLed、CommonLed
  switch (currentMode) {
    case 0:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = (setsPage[j][currentSet] > 0 ?  1 : 0);
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = (commonsPage[j] > 0 ?  1 : 0);
      }
      break;
    case 1:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = (setsPage[j][currentSet] > 0 ?  1 : 0);
        if (j == editingPart) ledsStatus[LED_B_PART[j]] = 2;
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = (commonsPage[j] > 0 ?  1 : 0);
        if (j == editingPart - 8) ledsStatus[LED_B_COMMON[j]] = 2;
      }
      break;
    case 2:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = (shihtAble[j] ?  2 : 0);
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = 0;
      }
      break;
    case 3:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = 0;
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = 0;
      }
      break;
  }
  updateLed(nowTime);

  //控制Lcd顯示
  updateLcd(nowTime);
}
//index
const byte BTN_MODE[4] = {3, 1, 11, 9};
const byte BTN_SET[2] = {19, 17};
const byte BTN_FN[6] = {2, 0, 10, 8, 18, 16};
const byte BTN_PART[8] = {4, 5, 6, 7, 12, 13, 14, 15};
const byte BTN_COMMON[2] = {22, 23};

const byte LED_B_MODE[4] = {16, 19, 21, 23};
const byte LED_B_STATUS[2] = {3, 7};
const byte LED_B_PART[8] = {9, 11, 13, 15, 25, 27, 29, 31};
const byte LED_B_COMMON[2] = {1, 5};

const byte LED_R_MODE[4] = {17, 18, 20, 22};
const byte LED_R_STATUS[2] = {2, 6};
const byte LED_R_PART[8] = {8, 10, 12, 14, 24, 26, 28, 30};
const byte LED_R_COMMON[2] = {0, 4};

//模式
const String MODE_STRING[4] = {"Ctrl", "Edit", "Shift", "Config"};
byte currentMode = 0;

//組合(Record)
const byte SET_LENGTH = 25;
byte currentSet = 0;

//頁數
const short MAX_PAGE = 999;
short setsPage[8][SET_LENGTH] = {}; //各set頁數 (預設值應該設為1)
short commonsPage[2] = {}; //共用頁數

//LED狀態 0關 1開 2閃爍 3一閃
byte ledsStatus[32] = {};

//LED閃爍
const int LED_BLINK_PERIOD = 400;
const int LED_BLINK_TIME = 350;
unsigned long lastBlankTime = 0;

//LED一閃
const int LED_ONE_BLINK_TIME = 80;
unsigned long oneBlinkTime[32] = {}; //一閃發光時間

//LCD顯示
const int LCD_UPDATE_PERIOD = 50;
unsigned long lastLcdUpdate = 0;

//LCD高亮
const int LCD_BLINK_TIME = 300;
unsigned long lcdHighLightTime = 0;
String lcdHighLightStatus1 = "";
String lcdHighLightStatus2 = "";


//ctrl
short inputPage = 0; //數字鍵盤輸入的控制頁數

//edit
byte editingPart = 0; //0~7個別 8,9共通

//shift
boolean shihtAble[8] = {true, true, true, true, true, true, true, true};

//Keyboard
#include "ctrlPPT.h"


//Keypad
#include "Keypad.h"
const byte KEYPAD_PIN = A0;
const byte KEYPAD_NUMBER[17] = {0, 1, 2, 3, 0 , 4 , 5 , 6 , 0 , 7 , 8 , 9, 0, 0, 0, 0, 0};

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

BYTES_VAL_T pinValues;
BYTES_VAL_T oldPinValues;

//Lcd 74LS164
#include <Wire.h>
#include <LiquidCrystal_SR.h>
LiquidCrystal_SR lcd(2, 3, TWO_WIRE);

//Led 74HC595
const byte dataPin = 10;
const byte latchPin = 9;
const byte clockPin = 8;

//按鈕: 讀取狀態
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

//按鈕:透過index來檢查該按鈕是否按下
boolean isClickDown(byte index) {
  return ((pinValues >> index) & 1) && !((oldPinValues >> index) & 1);
}

//EEPROM
#include <EEPROM.h>

//EEPROM 讀寫Short
void EEPROMWriteShort(int address, short value)
{
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);

  EEPROM.write(address * 2, two);
  EEPROM.write(address * 2 + 1, one);
}

short EEPROMReadShort(int address)
{
  long two = EEPROM.read(address * 2);
  long one = EEPROM.read(address * 2 + 1);

  return ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
}

//寫入指定的Set資料
void saveSetData(byte setIndex)
{
  int addr = setIndex * 8;
  for (byte i = 0; i < 8; i++) {
    EEPROMWriteShort(addr++, setsPage[i][setIndex]);
  }
}

void saveCommonData()
{
  int addr = SET_LENGTH * 8;
  EEPROMWriteShort(addr++, commonsPage[0]);
  EEPROMWriteShort(addr++, commonsPage[1]);
}

//存檔組合
void saveCurrentData()
{
  saveSetData(currentSet);
  saveCommonData();
}

void saveData()
{
  int addr = 0;

  //寫入Part資料
  for (byte i = 0; i < SET_LENGTH; i++) {
    for (byte j = 0; j < 8; j++) {
      EEPROMWriteShort(addr++, setsPage[j][i]);
    }
  }

  //寫入Common資料
  EEPROMWriteShort(addr++, commonsPage[0]);
  EEPROMWriteShort(addr++, commonsPage[1]);
}

void readData()
{
  int addr = 0;

  //讀取Part資料
  for (byte i = 0; i < SET_LENGTH; i++) {
    for (byte j = 0; j < 8; j++) {
      setsPage[j][i] = EEPROMReadShort(addr++);
    }
  }

  //讀取Common資料
  commonsPage[0] = EEPROMReadShort(addr++);
  commonsPage[1] = EEPROMReadShort(addr++);
}

void clearDada() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

//高亮LCD
void highLightLcd(String status1, String status2, int highLightTime) {
  lcdHighLightStatus1 = status1;
  lcdHighLightStatus2 = status2;
  lcdHighLightTime = millis() + highLightTime;
}

//閃爍LED
void oneBlinkLed(byte index, int blinkTime) {
  ledsStatus[index] = 3;
  oneBlinkTime[index] = millis() + blinkTime;
}

//處理按鈕事件
void btnEvent(unsigned long nowTime) {
  //Mode 按鈕
  for (byte i = 0; i < 4; i++) {
    if (isClickDown(BTN_MODE[i])) {
      //更新Mode Led
      //ledsStatus[LED_B_MODE[currentMode]] = 0;
      //ledsStatus[LED_B_MODE[i]] = 1;
      //修改當前Mode
      currentMode = i;
      Serial.println("Mode: " + String(i));

      //重置閃爍
      lastBlankTime = nowTime;
    }
  }

  //Set 按鈕, 增減currentSet
  if (isClickDown(BTN_SET[0]) && currentSet > 0) {
    currentSet--;
    Serial.print("currentSet: ");
    Serial.println(currentSet);
  }
  if (isClickDown(BTN_SET[1]) && currentSet < SET_LENGTH - 1) {
    currentSet++;
    Serial.print("currentSet: ");
    Serial.println(currentSet);
  }

  //FN按鈕
  if (isClickDown(BTN_FN[0])) {
    Serial.println("FN-0");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-3);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:PageUp", LCD_BLINK_TIME);
        break;
      case 1:
        //單獨減1
        if (editingPart < 8) {
          if (setsPage[editingPart][currentSet] > 0) setsPage[editingPart][currentSet] --;
        } else {
          if (commonsPage[editingPart - 8] > 0) commonsPage[editingPart - 8] --;
        }
        //儲存
        saveCurrentData();
        break;
      case 2:
        //群體減1
        for (byte i = 0; i < 8; i++) {
          int page = setsPage[i][currentSet];
          if (shihtAble[i] && page > 0) setsPage[i][currentSet] = page - 1;
        }
        //儲存
        saveCurrentData();
        break;
    }
  }
  if (isClickDown(BTN_FN[1])) {
    Serial.println("FN-1");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-4);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:PageDown", LCD_BLINK_TIME);
        break;
      case 1:
        //單獨加1
        if (editingPart < 8) {
          if (setsPage[editingPart][currentSet] < MAX_PAGE) setsPage[editingPart][currentSet] ++;
        } else {
          if (commonsPage[editingPart - 8] < MAX_PAGE) commonsPage[editingPart - 8] ++;
        }
        //儲存
        saveCurrentData();
        break;
      case 2:
        //群體加1
        for (byte i = 0; i < 8; i++) {
          int page = setsPage[i][currentSet];
          if (shihtAble[i] && page > 0 && page < MAX_PAGE) setsPage[i][currentSet] = page + 1;
        }
        //儲存
        saveCurrentData();
        break;
    }
  }
  if (isClickDown(BTN_FN[2])) {
    Serial.println("FN-2");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-5);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Black", LCD_BLINK_TIME);
        break;
      case 1:
      case 2:
        highLightLcd("", "Save...", LCD_BLINK_TIME);
        saveData();
        break;
    }
  }
  if (isClickDown(BTN_FN[3])) {
    Serial.println("FN-3");
    switch (currentMode) {
      case 0:
        //送出控制
        keyControl(-6);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Write", LCD_BLINK_TIME);
        break;
    }
  }
  if (isClickDown(BTN_FN[4])) {
    Serial.println("FN-4");
    switch (currentMode) {
      case 0:
        keyControl(-1);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Start", LCD_BLINK_TIME);
        break;
    }
  }
  if (isClickDown(BTN_FN[5])) {
    Serial.println("FN-2");
    switch (currentMode) {
      case 0:
        keyControl(-2);
        oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
        highLightLcd("", "Fn:Stop", LCD_BLINK_TIME);
        break;
    }
  }

  //part按鈕
  for (byte i = 0; i < 8; i++) {
    if (isClickDown(BTN_PART[i])) {
      short page = setsPage[i][currentSet];
      switch (currentMode) {
        case 0:
          //送出控制
          Serial.println("CtrlPart: " + String(page));

          //頁數大於0檢查
          if (page > 0) {
            keyPage(page);
            oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
            highLightLcd("", "GoTo:" + String(page), LCD_BLINK_TIME);
          } else {
            highLightLcd("", "Enpty", LCD_BLINK_TIME);
          }
          break;
        case 1:
          //切換editingPart
          editingPart = i;
          Serial.print("editingPart: ");
          Serial.println(editingPart);
          break;
        case 2:
          //平移開關
          shihtAble[i] = !shihtAble[i];
          break;
      }
    }
  }

  //Common按鈕
  for (byte i = 0; i < 2; i++) {
    if (isClickDown(BTN_COMMON[i])) {
      short page = commonsPage[i];
      switch (currentMode) {
        case 0:
          //送出控制
          Serial.println("CtrlCommonPart: " + String(page));

          //頁數大於0檢查
          if (page > 0) {
            keyPage(page);
            oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
            highLightLcd("", "GoTo:" + String(page), LCD_BLINK_TIME);
          } else {
            highLightLcd("", "Enpty", LCD_BLINK_TIME);
          }
          break;
        case 1:
          //切換編輯中按鈕
          editingPart = 8 + i;
          Serial.print("editingPart: ");
          Serial.println(editingPart);
          break;
        case 2:
          //切換平移開關
          break;
      }
    }
  }

}

void keypadEvent()
{
  //讀取Keypad
  byte key = readPush(KEYPAD_PIN);
  if (key == 0) return;

  //根據Mode和 按下的按鈕決定動作
  switch (currentMode) {
    case 0:
      //控制inputPage
      switch (key) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 14:
          //數字鍵
          if (inputPage == MAX_PAGE) {
            inputPage = KEYPAD_NUMBER[key];
          } else {
            inputPage = inputPage * 10 + KEYPAD_NUMBER[key];
            if (inputPage > MAX_PAGE) inputPage = MAX_PAGE;
          }
          break;
        case 13:
          //減
          if (--inputPage < 0) inputPage = 0;
          break;
        case 15:
          //加
          if (++inputPage > MAX_PAGE) inputPage = MAX_PAGE;
          break;
        case 4:
          break;
        case 8:
          //退後
          inputPage /= 10;
          break;
        case 12:
          //清除
          inputPage = 0;
          break;
        case 16:
          //Enter
          if (inputPage > 0) {
            Serial.println("Input Ctrl " + String(inputPage));
            keyPage(inputPage);
            oneBlinkLed(LED_B_STATUS[0], LED_ONE_BLINK_TIME);
            inputPage = 0;
          }
          break;
      }
      break;
    case 1:
      //編輯頁數
      short newPage = editingPart < 8 ? setsPage[editingPart][currentSet] : commonsPage[editingPart - 8];
      switch (key) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 14:
          //數字
          newPage = newPage * 10 + KEYPAD_NUMBER[key];
          if (newPage > MAX_PAGE) newPage = MAX_PAGE;
          break;
        case 13:
          //減
          if (--newPage < 0) newPage = 0;
          break;
        case 15:
          //加
          if (++newPage > MAX_PAGE) newPage = MAX_PAGE;
          break;
        case 4:
          //倒退
          newPage /= 10;
          break;
        case 8:
          //清除
          newPage = 0;
          break;
        case 12:
          break;
        case 16:
          //Enter 切到下一個part
          if (++editingPart > 9) editingPart = 0;
          return; //直接return掉
      }

      if (editingPart < 8)
        setsPage[editingPart][currentSet] = newPage;
      else
        commonsPage[editingPart - 8] = newPage;
      //儲存
      saveCurrentData();
      break;
  }
}

//更新Led狀態
void updateLed(unsigned long nowTime)
{
  //閃爍判定
  unsigned long diffTime = nowTime - lastBlankTime;
  boolean isLight = diffTime < LED_BLINK_TIME;
  boolean isBlink = diffTime > LED_BLINK_TIME + LED_BLINK_PERIOD;
  if (isBlink) lastBlankTime = nowTime;

  //將狀態轉為byte格式
  byte byteLedsStatus[4] = {};
  for (byte i = 0; i < 32; i++) {
    byte index = i / 8;
    switch (ledsStatus[i]) {
      case 0:
        //關
        byteLedsStatus[index] = (byteLedsStatus[index] << 1);
        break;
      case 1:
        //亮
        byteLedsStatus[index] = (byteLedsStatus[index] << 1) + 1;
        break;
      case 2:
        //閃
        byteLedsStatus[index] = isLight ? (byteLedsStatus[index] << 1) + 1 : (byteLedsStatus[index] << 1);
        break;
      case 3:
        //一閃
        if (nowTime < oneBlinkTime[i]) {
          byteLedsStatus[index] = (byteLedsStatus[index] << 1) + 1;
        } else {
          byteLedsStatus[index] = (byteLedsStatus[index] << 1);
          ledsStatus[i] = 0;
        }
        break;
    }

  }
  //這個之後可以獨立成一個函式
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[0]); //0~7
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[1]); //8~15
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[2]); //16~23
  shiftOut(dataPin, clockPin, LSBFIRST, byteLedsStatus[3]); //24~31
  digitalWrite(latchPin, HIGH);
}

//更新Lcd狀態
void updateLcd(unsigned long nowTime)
{
  //判斷時間是否達到更新週期
  if (nowTime - lastLcdUpdate < LCD_UPDATE_PERIOD) return;
  lastLcdUpdate = nowTime;

  //Clear
  lcd.clear();

  //左上角Set(Record)
  lcd.home();
  lcd.print("Record-");
  lcd.print(currentSet + 1);

  //右上角Mode
  //lcd.setCursor(16 - String(currentMode).length(), 0);
  lcd.setCursor(16 - MODE_STRING[currentMode].length(), 0);
  lcd.print(MODE_STRING[currentMode]);

  //下排
  switch (currentMode) {
    case 0:
      //inputPage
      if (inputPage > 0) {
        lcd.setCursor(0, 1);
        lcd.print("Input:" + String(inputPage));
      }
      break;
    case 1:
      //editingPart頁數
      lcd.setCursor(0, 1);
      if (editingPart < 8)
        lcd.print("Part_" + String(editingPart + 1) + ":" + (setsPage[editingPart][currentSet] > 0 ? String(setsPage[editingPart][currentSet]) : "_"));
      else
        lcd.print("Common_" + String(editingPart - 7) + ":" + (commonsPage[editingPart - 8] > 0 ? String(commonsPage[editingPart - 8]) : "_"));
      break;
    case 2:
      //計算最小頁數
      unsigned int minValue = MAX_PAGE;
      for (byte i = 0; i < 8; i++) {
        if (setsPage[i][currentSet] > 0)  minValue = min(minValue, setsPage[i][currentSet]);
      }
      //顯示最小頁數
      lcd.setCursor(0, 1);
      lcd.print("First:");
      lcd.print(minValue > 0 && minValue < MAX_PAGE ? String(minValue) : "_");
      break;
  }

  //Lcd High Light
  if (lcdHighLightTime > nowTime) {
    lcd.setCursor(0, 0);
    lcd.print(lcdHighLightStatus1);
    lcd.setCursor(0, 1);
    lcd.print(lcdHighLightStatus2);
  }
}

void setup()
{
  Serial.begin(9600);
  //clearDada();
  readData();

  //BUTTON
  pinMode(btnPloadPin, OUTPUT);
  pinMode(btnClockEnablePin, OUTPUT);
  pinMode(btnClockPin, OUTPUT);
  pinMode(btnDataPin, INPUT);

  digitalWrite(btnClockPin, LOW);
  digitalWrite(btnPloadPin, HIGH);
  pinValues = read_shift_regs();
  oldPinValues = pinValues;

  //LCD
  lcd.begin(16, 2);

  //LED
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

}

void loop()
{
  unsigned long nowTime = millis();

  //讀取按鈕，更新狀態 or 執行控制 (根據Mode，按鈕會有不同功能)
  oldPinValues = pinValues;
  pinValues = read_shift_regs();
  btnEvent(nowTime);
  delay(POLL_DELAY_MSEC);

  keypadEvent();


  //更新Led顯示(在這裡更新效能差了點，不過可以節省很多程式碼)
  //ModeLed
  for (byte j = 0; j < 4; j++) {
    ledsStatus[LED_B_MODE[j]] = (j == currentMode ? 1 : 0);
  }
  //PartLed、CommonLed
  switch (currentMode) {
    case 0:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = (setsPage[j][currentSet] > 0 ?  1 : 0);
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = (commonsPage[j] > 0 ?  1 : 0);
      }
      break;
    case 1:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = (setsPage[j][currentSet] > 0 ?  1 : 0);
        if (j == editingPart) ledsStatus[LED_B_PART[j]] = 2;
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = (commonsPage[j] > 0 ?  1 : 0);
        if (j == editingPart - 8) ledsStatus[LED_B_COMMON[j]] = 2;
      }
      break;
    case 2:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = (shihtAble[j] ?  2 : 0);
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = 0;
      }
      break;
    case 3:
      for (byte j = 0; j < 8; j++) {
        ledsStatus[LED_B_PART[j]] = 0;
      }
      for (byte j = 0; j < 2; j++) {
        ledsStatus[LED_B_COMMON[j]] = 0;
      }
      break;
  }
  updateLed(nowTime);

  //控制Lcd顯示
  updateLcd(nowTime);
}
