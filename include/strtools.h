#include <main.h>


enum copyFlags {None = 0, Utf2Ansi = 1, ToUpper = 2, ToLower = 4};
enum alignment {alnLeft, alnCenter, alnRight};
enum specialChars {V_10 = 30, V_100, HYPHEN = 173};

const char ellipse[] = "\x85";

boolean strcopyext(U8G2* u8g2, char* dest, const char* src, uint8_t maxlength, uint8_t flags = None, uint16_t *chgFlags = NULL, uint16_t fieldFlag = 0, u8g2_uint_t maxWidth = 0, char** dest2 = NULL, const char* endMarker = ellipse, char replace = char(164));

void drawStrAligned(U8G2* u8g2, u8g2_int_t y, const char *s, alignment a = alnLeft, u8g2_int_t x1 = 0, u8g2_int_t x2 = 0xff);
void drawScale(U8G2* u8g2, u8g2_int_t x, u8g2_int_t y, uint8_t barWidth, uint8_t stepWidth, uint8_t barCount, int8_t barStart, int8_t barEnd, int8_t valueStart, int8_t valueDelta);

char* formatTime(char* strTime, unsigned long time, bool withmillis = false);
char* formatValue(char* strValue, int8_t value, uint8_t maxdigits);

void debugOut(const char* message, const char msgtype = '!', const int* value = NULL);

