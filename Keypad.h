
//按鍵類比數值
const int keyValueMap[16] = {928, 851, 785, 729, 910, 836, 772,718, 892, 820, 759, 707, 875, 807, 747, 695};

//類比數值誤差範圍
const byte VALUE_RANGE = 3;

//過濾
#define FILTER_WIDTH 10
const int OK_RANGE = 3 * FILTER_WIDTH;
int filterArray[FILTER_WIDTH];
byte filterIndex = 0;

//彈跳
const int DEBOUNCE_TIME = 100; 
int lastKey = 0;
long debounceColldown = 0;

//能過濾雜訊的讀取
int filterRead(byte pin) {
  //紀錄N次的值
  int value = analogRead(pin);
  filterArray[filterIndex++] = value;
  if (filterIndex == FILTER_WIDTH) filterIndex = 0;

  //計算平均值
  int averageValue = 0;
  for (byte i = 0; i < FILTER_WIDTH; i++) {
    averageValue += filterArray[i];
  }
  averageValue = averageValue / FILTER_WIDTH;

  //計算總平均差
  int deviation = 0;
  for (byte i = 0; i < FILTER_WIDTH; i++) {
    deviation += fabs(filterArray[i] - averageValue);
  }

  //判定是否合乎標準
  if (deviation < OK_RANGE) return averageValue;
  else return 0;
}

//讀取按鍵: 0無、1~16按鍵、17未知
byte readKey(byte pin) {

  int aValue = filterRead(pin);

  if (aValue < 600) return 0;

  for (byte i = 0; i < 16; i++) {
    if ((aValue > (keyValueMap[i] - VALUE_RANGE)) && (aValue < (keyValueMap[i] + VALUE_RANGE))) return i + 1;
  }
  return 17;
}

//讀取按下按鍵(消除彈跳): 0無 1~16按鍵
byte readPush(byte pin) {

  long nowTime = millis();
  byte nowKey = readKey(pin);

  if (nowKey != 17) {
    if (lastKey == 0 && nowKey > 0 && nowTime > debounceColldown) {
      debounceColldown = nowTime + DEBOUNCE_TIME;
      return nowKey;
    }
    lastKey = nowKey;
  }
  return 0;
}
