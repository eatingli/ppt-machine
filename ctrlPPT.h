#include <Keyboard.h>

static void key(uint8_t key) {
  Keyboard.press(key);
  //delay(100);
  Keyboard.release(key);
}

//控制頁數跳頁
static boolean keyPage(int page) {
  if (page < 10) {
    Keyboard.print(page);
  } else if (page < 100) {
    Keyboard.print(page / 10);
    Keyboard.print(page % 10);
  } else {
    key(Keyboard.print(page / 100));
    key(Keyboard.print((page % 100) / 10));
    key(Keyboard.print(page % 10));
  }
  //delay(100);
  key(KEY_RETURN);
  return true;
}

//控制特定功能
static boolean keyControl(int ctrlCode) {
  //Serial.println(ctrlCode);
  switch (ctrlCode) {
    case -1: key(KEY_F5); break;
    case -2: key(KEY_ESC); break;
    case -3: key(KEY_PAGE_UP);  break;
    case -4: key(KEY_PAGE_DOWN); break;
    case -5: Keyboard.print("B"); break; 
    case -6: Keyboard.print("W"); break; 
    default: return false;
  }
  return true;
}

//控制PPT, 回應錯誤訊息(空白代表成功)
static char* ctrlPPT(int ctrlCode) {
  if (ctrlCode > 0 && ctrlCode < 1000 && keyPage(ctrlCode)) {
    return "";

  } else if (ctrlCode < 0) {
    if (keyControl(ctrlCode)) return "";
    else return "This ctrl-code fail to be parsed.";
    
  } else {
    return "This ctrl-code over the range.(-1~-4 and 1~999)";
  }
}
