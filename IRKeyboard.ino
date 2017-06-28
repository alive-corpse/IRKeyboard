/*
 * Made by Evgeniy Shumilov <evgeniy.shumilov@gmail.com>
 * 
 * Based on https://github.com/NicoHood/HID/wiki/Keyboard-API#improved-keyboard,
 * IRremote
 */

#include <IRremote.h>
#include "HID-Project.h"

int RECV_PIN = 4;
byte sleep = 150;

IRrecv irrecv(RECV_PIN);
decode_results results;

/*
 * Count of modes
 */
const byte mcount = 4; 

/*
 * Current mode
 * 0 - main mode
 * 1 - mouse mode
 * 2 - macro mode
 * 3 - kodi mode
 */
byte mode = 0;

/* Code types
 *  -1 - none
 *  0 - normal
 *  1 - send in not raw type
 *  2 - macro
 *  3 - set mode (code is modenum)
 *  10 - mouse click (code is number of button: 1 - left, 2 - middle, 3 - right)
 *  11 - mouse down/up (the same as click)
 *  12 - mouse shift x axe
 *  13 - mouse shift y axe
 *  14 - mouse set shift
 */
struct tcode {
  int code;
  int type;
};

struct tkey {
  unsigned long id;
  String desc;
  tcode codes[mcount]; 
};

tcode last;
byte mouseshift = 15;

const int repeat = -100;
const int kcount = 35;


tkey keys [kcount] = {//    Main Mode    Mouse Mode   Macro Mode   Kodi Mode
  {4294967295, "Repeat",   {{repeat, 0}, {repeat, 0}, {repeat, 0}, {repeat, 0}}},
  {284111055,"PowerOnOff", {{102,    0}, {102,    0}, {102,    0}, {102,    0}}},
  {284143695, "AV/TV",     {{3,      3}, {3,      3}, {3,      3}, {3,      3}}},
  {284127375, "Macro",     {{2,      3}, {2,      3}, {2,      3}, {2,      3}}},
  
  {284117175, "1",         {{30, 0}, {5,   14}, {260, 0}, {30,  0}}},
  {284149815, "2",         {{31, 0}, {10,  14}, {261, 0}, {31,  0}}},
  {284109015, "3",         {{32, 0}, {25,  14}, {262, 0}, {32,  0}}},
  {284141655, "4",         {{33, 0}, {50,  14}, {33,  0}, {33,  0}}},
  {284125335, "5",         {{34, 0}, {100, 14}, {34,  0}, {34,  0}}},
  {284157975, "6",         {{35, 0}, {125, 14}, {35,  0}, {35,  0}}},
  {284104935, "7",         {{36, 0}, {150, 14}, {36,  0}, {36,  0}}},
  {284137575, "8",         {{37, 0}, {200, 14}, {37,  0}, {37,  0}}},
  {284121255, "9",         {{38, 0}, {250, 14}, {38,  0}, {38,  0}}},
  {284145735, "0",         {{39, 0}, {2 ,  14}, {39,  0}, {39,  0}}},
  {284113095, "Num",       {{-1, 0}, {-1,   0}, {-1,  0}, {278, 0}}},
  
  {284129415, "Back",      {{42,      0}, {42,      0}, {42,      0}, {42,  0}}},
  {284162055, "VolUp",     {{128,     0}, {128,     0}, {128,     0}, {128, 0}}},
  {284148795, "VolDown",   {{129,     0}, {129,     0}, {129,     0}, {129, 0}}},
  {284099835, "Mute",      {{127,     0}, {127,     0}, {127,     0}, {127, 0}}},
  {284107995, "Mouse",     {{1,       3}, {1,       3}, {1,       3}, {1,   3}}},
  {284132475, "ChUp",      {{331,     0}, {331,     0}, {-1,      0}, {MEDIA_NEXT, 1}}}, //U - move item up for kodi
  {284140635, "ChDown",    {{334,     0}, {334,     0}, {-1,      0}, {MEDIA_PREVIOUS, 1}}}, //D - move item down for kodi
  {284124315, "Menu",      {{101,     0}, {101,     0}, {101,     0}, {101, 0}}}, //C context menu for kodi
  {284103915, "Exit",      {{KEY_ESC, 0}, {KEY_ESC, 0}, {KEY_ESC, 0}, {KEY_ESC, 0}}},
  {284135535, "Info",      {{233,     0}, {1,      11}, {-1,      0}, {268, 0}}}, //I keyboard key for kodi
  {284151855, "Guide",     {{-1,      0}, {3,      10}, {-1,      0}, {282,  0}}},
  
  {284156955, "Up",        {{KEY_UP_ARROW,    0}, {-1, 12}, {KEY_UP_ARROW,    0}, {KEY_UP_ARROW,    0}}},
  {284119215, "Down",      {{KEY_DOWN_ARROW,  0}, {1,  12}, {KEY_DOWN_ARROW,  0}, {KEY_DOWN_ARROW,  0}}},
  {284116155, "Left",      {{KEY_LEFT_ARROW,  0}, {-1, 13}, {KEY_LEFT_ARROW,  0}, {KEY_LEFT_ARROW,  0}}},
  {284136555, "Right",     {{KEY_RIGHT_ARROW, 0}, {1,  13}, {KEY_RIGHT_ARROW, 0}, {KEY_RIGHT_ARROW, 0}}},
  {284120235, "Ok",        {{KEY_RETURN,      0}, {1,   10}, {KEY_RETURN,      0}, {KEY_RETURN,      0}}},
  
  {284133495, "Yellow",    {{MEDIA_STOP,       1}, {MEDIA_STOP,       1}, {MEDIA_STOP,       1}, {'x',              1}}},
  {284100855, "Green",     {{MEDIA_PREVIOUS,   1}, {MEDIA_PREVIOUS,   1}, {MEDIA_PREVIOUS,   1}, {277,              0}}}, //R (277) - rewind for kodi
  {284160015, "Red",       {{MEDIA_PLAY_PAUSE, 1}, {MEDIA_PLAY_PAUSE, 1}, {MEDIA_PLAY_PAUSE, 1}, {' ',              1}}},
  {284153895, "Blue",      {{MEDIA_NEXT,       1}, {MEDIA_NEXT,       1}, {MEDIA_NEXT,       1}, {265,              0}}} //F(265) - fast forward for kodi
};

String getDesc(unsigned long id) {
  for (byte i=0; i<kcount; i++) {
    if (id == keys[i].id) {
      return keys[i].desc;
    }
  }
  return "";
}

tcode getCode(unsigned long id) {
  for (byte i=0; i<kcount; i++) {
    if (id == keys[i].id) {
      return keys[i].codes[mode];
    }
  }
  return {-1, -1};
}

void codeSwitch(tcode code) {
  switch (code.type) {
    case -1:
      Serial.print("stub");
      break;
    case 0:
      switch (code.code) {
        case -1:
          Serial.print("Unknown keycode: ");
          Serial.print(results.value);
          Serial.print(" ");
          break;
        default:
          //last = results.value;
          Serial.print("Sending key code: ");
          Serial.print(code.code);
          Keyboard.write(KeyboardKeycode(code.code));
      } // End case 0
      break;
    case 1: // Sending not raw format
      Serial.print("Sending key code: ");
      Serial.print(code.code);
      Keyboard.write(code.code);
      break;
    case 2: // Macro mode
      switch (code.code) {
        case 1:
          break;
      }
      break;
    case 3: // Setting mode
      if (mode == code.code && mode != 0) {
        mode = 0;
        Serial.print("mode unsetted");
      } else {
        mode = code.code;
        Serial.print("mode setted");
      }
      break;
    case 10:
      switch (code.code) {
        case 1:
          Mouse.click(MOUSE_LEFT);
          break;
        case 2:
          Mouse.click(MOUSE_MIDDLE);
          break;
        case 3:
          Mouse.click(MOUSE_RIGHT);
          break;
      }
      break;
    case 11:
      switch (code.code) {
        case 1:
          if (Mouse.isPressed(MOUSE_LEFT)) {
            Mouse.release(MOUSE_LEFT);
          } else {
            Mouse.press(MOUSE_LEFT);
          }
          break;
        case 2:
          if (Mouse.isPressed(MOUSE_MIDDLE)) {
            Mouse.release(MOUSE_MIDDLE);
          } else {
            Mouse.press(MOUSE_MIDDLE);
          }
          break;
        case 3:  
          if (Mouse.isPressed(MOUSE_RIGHT)) {
            Mouse.release(MOUSE_RIGHT);
          } else {
            Mouse.press(MOUSE_RIGHT);
          }
          break;
      }
      break;
    case 12:
      Mouse.move(0, code.code*mouseshift, 0);
      break;
    case 13:
      Mouse.move(code.code*mouseshift, 0, 0);
      break;
    case 14:
      mouseshift = code.code;
      break;
    }  
  Serial.println();
}


void sendCode(unsigned long value) {
    tcode code = getCode(value);
    String desc = getDesc(value);
    Serial.print("RAW: ");
    Serial.print(value);
    Serial.print(" MODE: ");
    Serial.print(mode);
    Serial.print(' ');
    if (desc != "") {
      Serial.print("DESC: ");
      Serial.print(desc);
      Serial.print(" ");
    }

    if (code.code != repeat) {
      last = code;
      codeSwitch(code);
    } else {
      codeSwitch(last);
    }
}


void setup() {
  Serial.begin(9600);
  Serial.println("Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Enabled IRin");
  Mouse.begin();
  Keyboard.begin();

}


void loop() {
  if (irrecv.decode(&results)) {
    sendCode(results.value);
    irrecv.resume(); // Receive the next value
  }
  delay(sleep);
}
