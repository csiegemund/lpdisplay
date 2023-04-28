#include <strtools.h>

u8g2_uint_t getAreaWidth(U8G2* u8g2, char* strFrom, char* strTo) 
{
  // util to get the string width for a partial string
  // uses u8g2 current configured font -> set used font before calling this function

  char tmp =  *strTo;
  *strTo = char(0); // temporarily replace end of partial string with null
  u8g2_uint_t curLen = u8g2->getStrWidth(strFrom);
  *strTo = tmp; // restore original char
  return curLen;
}

boolean strcopyext(U8G2* u8g2, char* dest, const char* src, uint8_t maxlength, uint8_t flags = None, uint16_t *chgFlags = NULL, uint16_t fieldFlag = 0, u8g2_uint_t maxWidth = 0, char** dest2 = NULL, const char* endMarker = ellipse, char replace = char(164))
{
  // this is the "swiss army knife" for string operations in this project:
  // can convert utf-8 to ansi (flags |= Utf2Ansi).
  // not convertible chars can be replaced by special char (default: ¤)
  // can convert toUpper or toLower for all ansi chars (flags |= ToUpper / ToLower)
  // can cut off the string at word boundary to stay below maxWidth
  // and / or can split the string to two rows to stay below maxWidth (char** dest2)
  // if string is shortened, a marker can show this (default: …)
  // does only copy changed characters and can set a flag if something changed (*chgFlags |= fieldFlag)
  // returns true if dest string differs from version before
  //
  // ... and all this is done during one loop through the chars only!

  boolean docopy = false;               // dest string is modified
  boolean dest2used = (dest2 == NULL);  // if dest2 is not requested we take it as used
  char c;                               // the current char
  char* last_spc = dest;                // last space found during loop
  char* cursor = dest;                  // current pos in dest
  char* cur_row = dest;                 // start of current row

  u8g2_uint_t endMarkerWith = u8g2->getStrWidth(endMarker) + 1;
  if (replace == 0) replace = 0x7f; // marker for skip char
  
  for (uint8_t i=0; ; i++)  // endless loop, will be exited by break
  {
    if (i < (maxlength-1)) { // restrict loop to maxlength
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
        // string should fit to maxWidth at word boundary 
        // -> we just found a blank or end of string
        if (getAreaWidth(u8g2, cur_row, cursor) <= (maxWidth - (dest2used ? endMarkerWith : 0))) {
          // still fits into maxWidth, store pos and go ahead
          last_spc = cursor;
        } else if (!dest2used) {
          // we have another row to fill
          // -> split row at last word boundary
          if (*last_spc != char(0)) {
            *last_spc = char(0);
            docopy = true;
          }
          cur_row = last_spc + 1;
          *dest2 = cur_row;
          dest2used = true;
        } else {
          // we need to cut the string at last word boundary
          cursor = last_spc;
          // add end marker string to indicate that string is not complete 
          while (*endMarker != char(0)) {
            if (*cursor != *endMarker) {
              *cursor = *endMarker;
              docopy = true;
            }
            cursor++; endMarker++;
          }
          c = 0; // finish dest string here
        }
      }
      // copy char to dest if it differs
      if (*cursor != c) {
        *cursor = c;
        docopy = true;
      }
      // stop processing here
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
  // draw the string at pos x with width w
  // default is the whole screen width
  // align string within this at left, right or center
  // string must fit into area, this is not checked!

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
  // draw a scale graph

  for (uint8_t i = 0; i < barCount; i++, x+= stepWidth) {
    // draw barCount zero-height bar indicators
    u8g2->drawHLine(x, y, barWidth);
    if ((i >= barStart) && (i <= barEnd)) {
      // draw real bars from barStart to barEnd 
      if (valueStart > 0) {
        u8g2->drawBox(x, y - valueStart, barWidth, valueStart);
      } else {
        u8g2->drawBox(x, y, barWidth, -valueStart);
      }
      // height (=value) is changed on every bar by valueDelta
      valueStart += valueDelta;
    }
  }
}

char* formatTime(char* strTime, unsigned long time, bool withmillis = false) {
  // format elapsed / total time to a string format
  // time = seconds or milliseconds (withmillis = true)
  // chars are placed into strtime
  // return value is pointer to next char AFTER the 
  // time string (for easy concatenation)
  uint8_t seconds, minutes, hours;
  uint16_t milliseconds = 0;

  if (withmillis) {
    milliseconds = time % 1000;
    time = time / 1000;
  }
  seconds = time % 60;
  time = time / 60;
  minutes = time % 60;
  hours = min(time / 60, 999); //we show max 999 hours

  // hours are shown only if present
  if (hours > 99) {
    *strTime++ = char('0' + (hours / 100));
  }
  if (hours > 9) {
    *strTime++ = char('0' + ((hours % 100) / 10));
  }
  if (hours > 0) {
    *strTime++ = char('0' + (hours % 10));
    *strTime++ = ':';
  }
  // minutes are with two digits if hours existing or > 10
  if ((minutes > 9) || (hours > 0)) {
    *strTime++ = char('0' + (minutes / 10));
  }
  // at least minutes are shown always with one digit
  *strTime++ = char('0' + (minutes % 10));
  *strTime++ = ':';
  // seconds are shown always
  *strTime++ = char('0' + (seconds / 10));
  *strTime++ = char('0' + (seconds % 10));
  if (withmillis) {
    *strTime++ = '.';
    *strTime++ = char('0' + (milliseconds / 100));
    *strTime++ = char('0' + (milliseconds % 100) / 10);
    *strTime++ = char('0' + (milliseconds % 10));
  }
  // close string with char(0) and return pointer to this
  // if string gets more chars the char(0) will be overwritten...
  *strTime = char(0);
  return (strTime);
}

char* formatValue(char* strValue, int8_t value, uint8_t maxdigits) {
  // format value into string
  // for maxdigits = 1 it will use a special character 
  // for 10 (the maximum value) 
  // for maxdigits = 2 it will use a special character 
  // for 100 (the maximum value) 
  // for minus a special short version is used
  //
  // this function only works for customized fonts wich
  // contain this special chars!

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
        *strValue++ = char(V_10); // 10 as one special char
        break;
      }
    case 11 ... 99:
      *strValue++ = char('0' + (value / 10));
      *strValue++ = char('0' + (value % 10));
      break;
    case 100:
      if (maxdigits <= 2) {
        *strValue++ = char(V_100); // 100 as one special char
        break;
      }
    default: // for values > 100 the std c function is used
      strValue = itoa(value, strValue, 10); 
      strValue+= strlen(strValue);
      break;
  }
  // close string with char(0) and return pointer to this
  // if string gets more chars the char(0) will be overwritten...
  *strValue = char(0);
  return (strValue);
}
  
void debugOut(const char* message, const char msgtype = '!', const int* value = NULL) {
  // print formatted text with timestamp to serial
  // this is only used  when:
  // - a special UART for debugging is defined (SERIAL_DBG)
  //   (depends on used board type)
  // - a debug msg type filter is defined in
  //   platformio_overide.ini (DEBUG_FILTER)
#ifdef SERIAL_DBG
  #ifdef DEBUG_FILTER
    if ((strchr(DEBUG_FILTER, '*') != NULL) || (strchr(DEBUG_FILTER, msgtype) != NULL)) {
      // msgtype is in filter string or filter string is "*"
      char prefix[15];
      char* cursor = prefix;
      cursor = formatTime(cursor, millis(), true); // add time stamp
      *cursor++ = ' ';
      *cursor++ = msgtype; // add message type
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

