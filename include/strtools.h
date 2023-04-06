#include <Arduino.h>
#include <U8g2lib.h>

enum copyFlags {None = 0, Utf2Ansi = 1, ToUpper = 2, ToLower = 4};
enum alignment {alnLeft, alnCenter, alnRight};
enum specialChars {V_10 = 30, V_100, HYPHEN = 173};

extern U8G2_SSD1309_128X64_NONAME0_1_4W_HW_SPI u8g2;  

boolean strcopyext(char* dest, const char* src, uint8_t maxlength, uint8_t flags = None, uint16_t *chgFlags = NULL, uint16_t fieldFlag = 0, u8g2_uint_t maxWidth = 0, char** dest2 = NULL, char* endMarker = "\x85", const char replace = char(164));

void drawStrAligned(u8g2_int_t y, const char *s, alignment a = alnLeft, u8g2_int_t x1 = 0, u8g2_int_t x2 = 0xff);
void drawScale(u8g2_int_t x, u8g2_int_t y, uint8_t barWidth, uint8_t stepWidth, uint8_t barCount, int8_t barStart, int8_t barEnd, int8_t valueStart, int8_t valueDelta);

char* formatTime(char* strTime, uint32_t time);
char* formatValue(char* strValue, int8_t value, uint8_t maxdigits);

