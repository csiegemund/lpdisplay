#include <strtools.h>

u8g2_uint_t getAreaWidth(U8G2* u8g2, char* strFrom, char* strTo) {
  
  char tmp =  *strTo;
  *strTo = char(0);
  u8g2_uint_t curLen = u8g2->getStrWidth(strFrom);
  *strTo = tmp;
  return curLen;
}

boolean strcopyext(U8G2* u8g2, char* dest, const char* src, uint8_t maxlength, uint8_t flags = None, uint16_t *chgFlags = NULL, uint16_t fieldFlag = 0, u8g2_uint_t maxWidth = 0, char** dest2 = NULL, const char* endMarker = ellipse, char replace = char(164))
{
  boolean docopy = false; 
  boolean dest2used = (dest2 == NULL);  //if dest2 is not requested we take it as used
  char c;
  char* last_spc = dest;
  char* cursor = dest;
  char* cur_row = dest;

  u8g2_uint_t endMarkerWith = u8g2->getStrWidth(endMarker) + 1;
  if (replace == 0) replace = 0x7f; // marker for skip char
  

  for (uint8_t i=0; ; i++)
  {
    if (i < (maxlength-1)) {
      c = *src++;
    } else {
      c = 0;
    }
    if ((uint8_t(c) >= 0x80) && ((flags & Utf2Ansi) != 0)) {
        //no ASCII char and utf-8 conversion requested
      if (uint8_t(c) < 0xC0) {
          //Utf-8 following char -> ignore
          c = 0x7f;  // marker for skip char
      } else if (uint8_t(c) < 0xC4) {
          //Utf-8 first char in upper Ansi range, holds bits 6 and 7
          c <<= 6;
          //Only Unicode 0xA0 to 0xFF map to ANSI
          if ((uint8_t(c) == 0xC0) || (uint8_t(c) >= 0xA0)) {
              //Utf-8 second char holds bits 0 to 5
              c |= (*src++ & 0x3F);
          }
          else
          {
              //Replace or Ignore unicode chars between 0x80 and 0x9F
              c = replace;
          }
      } else {
          //Replace or Ignore all other unicode chars
          c = replace;
      }
    }
    if (c != 0x7f) {
      if ((flags & ToUpper) != 0) {
          switch(c)
          {
            case 0x00 ... 0x89:
              c = toupper(c);
              break;
            case 0xe0 ... 0xf6:  // between à && þ but not ÷
            case 0xf8 ... 0xfe:
              c -= 0x20;
              break;
            case 0x9A:			// š
            case 0x9C:			// œ
            case 0x9E:			// ž
              c -= 0x10;
            case 0xFF:			// ÿ
              c = 0x9F;		  // Ÿ
          } 
      } else if ((flags & ToLower) != 0) {
          switch(c)
          {
            case 0x00 ... 0x89:
              c = tolower(c);
              break;
            case 0xc0 ... 0xd6:  // between à && þ but not ÷
            case 0xd8 ... 0xde:
              c += 0x20;
              break;
            case 0x8a:			// š
            case 0x8c:			// œ
            case 0x8e:			// ž
              c += 0x10;
            case 0x9f:			// ÿ
              c = 0xff;		  // Ÿ
          }
      }
      if ((maxWidth > 0) && ((c == 32) || (c == 0))) {
        if (getAreaWidth(u8g2, cur_row, cursor) <= (maxWidth - (dest2used ? endMarkerWith : 0))) {
          last_spc = cursor;
        } else if (!dest2used) {
          if (*last_spc != char(0)) {
            *last_spc = char(0);
            docopy = true;
          }
          cur_row = last_spc + 1;
          *dest2 = cur_row;
          dest2used = true;
        } else {
          cursor = last_spc;
          while (*endMarker != char(0)) {
            if (*cursor != *endMarker) {
              *cursor = *endMarker;
              docopy = true;
            }
            cursor++; endMarker++;
          }
          c = 0;
        }
      }
      if (*cursor != c) {
        *cursor = c;
        docopy = true;
      }
      if (c == 0) break;
      cursor++;
    }
  }

  //if dest2 was requested, but not needed -> set to empty row
  if (!dest2used) {
      *dest2 = cursor;
  }

  //set field flags if target string changed
  if (docopy && (chgFlags != NULL)) {
    *chgFlags |= fieldFlag;
  }
  return (docopy);
}

void drawStrAligned(U8G2* u8g2, u8g2_int_t y, const char *s, alignment a = alnLeft, u8g2_int_t x = 0, u8g2_int_t w = 0xff) {

  if (w==0xff) {w = u8g2->getDisplayWidth() - x;}
  switch (a) {
    case alnLeft:
      u8g2->drawStr(x, y, s);
      break;
    case alnCenter:
      u8g2->drawStr(x + (w - u8g2->getStrWidth(s)) / 2, y, s);
      break;
    case alnRight:
      u8g2->drawStr(x + w - u8g2->getStrWidth(s), y, s);
      break;
  }
}

void drawScale(U8G2* u8g2, u8g2_int_t x, u8g2_int_t y, uint8_t barWidth, uint8_t stepWidth, uint8_t barCount, int8_t barStart, int8_t barEnd, int8_t valueStart, int8_t valueDelta) {

  for (uint8_t i = 0; i < barCount; i++, x+= stepWidth) {
    u8g2->drawHLine(x, y, barWidth);
    if ((i >= barStart) && (i <= barEnd)) {
      if (valueStart > 0) {
        u8g2->drawBox(x, y - valueStart, barWidth, valueStart);
      } else {
        u8g2->drawBox(x, y, barWidth, -valueStart);
      }
      valueStart += valueDelta;
    }
  }
}

char* formatTime(char* strTime, unsigned long time, bool withmillis = false) {
  uint8_t seconds, minutes, hours;
  uint16_t milliseconds = 0;

  if (withmillis) {
    milliseconds = time % 1000;
    time = time / 1000;
  }
  seconds = time % 60;
  time = time / 60;
  minutes = time % 60;
  hours = min(time / 60, 99);

  if (hours > 9) {
    *strTime++ = char('0' + (hours / 10));
  }
  if (hours > 0) {
    *strTime++ = char('0' + (hours % 10));
    *strTime++ = ':';
  }
  if ((minutes > 9) || (hours > 0)) {
    *strTime++ = char('0' + (minutes / 10));
  }
  *strTime++ = char('0' + (minutes % 10));
  *strTime++ = ':';
  *strTime++ = char('0' + (seconds / 10));
  *strTime++ = char('0' + (seconds % 10));
  if (withmillis) {
    *strTime++ = '.';
    *strTime++ = char('0' + (milliseconds / 100));
    *strTime++ = char('0' + (milliseconds % 100) / 10);
    *strTime++ = char('0' + (milliseconds % 10));
  }
  *strTime = char(0);
  return (strTime);
}

char* formatValue(char* strValue, int8_t value, uint8_t maxdigits) {
    if (value < 0){
      *strValue++ = char(HYPHEN); //short hyphen as minus
      value = -value;
    }
    switch (value) {
      case 0 ... 9:
        *strValue++ = char('0' + value);
        break;
      case 10:
        if (maxdigits = 1) {
          *strValue++ = char(V_10);
          break;
        }
      case 11 ... 99:
        *strValue++ = char('0' + (value / 10));
        *strValue++ = char('0' + (value % 10));
        break;
      case 100:
        if (maxdigits <= 2) {
          *strValue++ = char(V_100);
          break;
        }
      default:
        strValue = itoa(value, strValue, 10); 
        strValue+= strlen(strValue);
        break;
    }
  return (strValue);
}
  
void debugOut(const char* message, const char msgtype = '!', const int* value = NULL) {
#ifdef SERIAL_DBG
  #ifdef DEBUG_SERIAL
    if ((strchr(DEBUG_SERIAL, '*') != NULL) || (strchr(DEBUG_SERIAL, msgtype) != NULL)) {
      char prefix[15];
      char* cursor = prefix;
      cursor = formatTime(cursor, millis(), true);
      *cursor++ = ' ';
      *cursor++ = msgtype;
      *cursor++ = ' ';
      *cursor = char(0);
      SERIAL_DBG.print(prefix);
      SERIAL_DBG.print(message);
      if (value != NULL) {
        SERIAL_DBG.print(": ");
        SERIAL_DBG.print(*value);
      }
      SERIAL_DBG.println();
    }
  #endif
#endif
}

