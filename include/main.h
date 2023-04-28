#include <Arduino.h>
#include <U8g2lib.h>

#ifndef MAIN_H
#define MAIN_H

// default display settings

#ifndef DISP_CONTROLLER 
  #ifndef DISP_CUSTOM
    #define DISP_CONTROLLER SSD1306
    #define DISP_BRAND NONAME
  #endif
#endif

#ifndef DISP_WIDTH
  #define DISP_WIDTH 128
#endif

#ifndef DISP_HEIGHT
  #define DISP_HEIGHT 64
#endif

#ifndef DISP_INTERFACE
  #define DISP_INTERFACE I2C
#endif

#ifndef DISP_ROTATION
  #define DISP_ROTATION U8G2_R0
#endif

#define DISP_PAGECOUNT (DISP_HEIGHT / 8)


// time to show pop-up messages for value changes (e.g. volume...) and status changes (e.g. mute)

#ifndef SHOW_VALUE_TIME
  #define SHOW_VALUE_TIME   3000
#endif

#ifndef SHOW_STATUS_TIME
  #define SHOW_STATUS_TIME  1500
#endif

// brightness of display when LED is ON or OFF
// 0 = Display off, 255 = Highest brightness

#ifndef LED_ON_BRIGHTNESS
  #define LED_ON_BRIGHTNESS   255
#endif

#ifndef LED_OFF_BRIGHTNESS
  #define LED_OFF_BRIGHTNESS  255
#endif


#if DISP_WIDTH >= 240
// fonts used for long displays (one title row)

// fonts for top status line
#define TOP_SMALL_FONT audiofont_6x12
#define TOP_BIG_FONT   audiofont_9x15

// fonts for the title information
#define ARTIST_FONT    audiofont_6x12
#define TITLE_FONT     audiofont_9x15
#define ALBUM_FONT     audiofont_6x12
#define TITLE_ROWS     1

// font for bottom status line
#define BOTTOM_FONT    audiofont_6x12

#else
// fonts used for std displays (one or two title rows)

// fonts for top status line
#define TOP_SMALL_FONT audiofont_5x8
#define TOP_BIG_FONT   audiofont_6x12

// fonts for the title information
#define ARTIST_FONT    audiofont_5x8
#define TITLE_FONT     audiofont_6x12
#define ALBUM_FONT     audiofont_5x8
#define TITLE_ROWS     2

// font for bottom status line
#define BOTTOM_FONT    audiofont_5x8

#endif

// font for the value & status icons in popup display
#define ICON_FONT      audioicons_20x20
#define ICON_WIDTH_S   13
#define ICON_WIDTH_B   24


// default pin settings for SPI and I2C
#ifndef SPI_CS
  #define SPI_CS    0
#endif

#ifndef SPI_DC
  #define SPI_DC    16
#endif

#ifndef SPI_RS
  #define SPI_RS    15 
#endif

#ifndef SPI_CS
  #define SPI_CS    0
#endif

#ifndef I2C_DC
  #define I2C_DC    16
#endif

#ifndef I2C_RS
  #define I2C_RS    U8X8_PIN_NONE 
#endif

// settings for serial UART used for arylic & debugging
#if defined(ARDUINO_AVR_NANO_EVERY)
    #define SERIAL_DAT Serial1
    #define SERIAL_DBG Serial
#elif defined(ARDUINO_attinyxy4)
    #define SERIAL_DAT Serial
    #undef  SERIAL_DBG
#elif defined(ARDUINO_attinyxy6)
    #define SERIAL_DAT Serial
    #undef  SERIAL_DBG
#elif defined(ARDUINO_attinyxy7)
    #define SERIAL_DAT Serial
    #undef  SERIAL_DBG
#elif defined(USB_VID)  //if USB port existing we assume there are 2 UARTS
    #define SERIAL_DAT Serial1
    #define SERIAL_DBG Serial
#else
    #define SERIAL_DAT Serial
    #undef  SERIAL_DBG
#endif

// helper values to distinct the different interfaces
#define NO_IF  0
#define I2C    1
#define SPI_3W 2
#define SPI_4W 3

// buffer sizes
#define CMD_LENGTH 3
#define TEXT_LENGTH 50
#define BUFFER_LENGTH CMD_LENGTH + 1 + TEXT_LENGTH + 2

// value for the current audio source (SRC)
// numeric value is the related icon char value also
enum sourceEnum { SRC_UNKNOWN, BT, BTC, NET, NETC, USB, LINEIN, OPT, COAX };

// shuffle status (derived from LPM)
// numeric value is the icon char value also
enum shuffleEnum { SEQUENCE = 11, SHUFFLE };

// repeat status (derived from LPM)
// numeric value is the icon char value also
enum repeatEnum { REPEATNONE = 14, REPEATALL, REPEATONE };

// play status (PLA)
// numeric value is the icon character value also
enum playEnum { PLA_UNKNOWN, PLAY = 17, PAUSE, STOP, PREV, NEXT };


// channel status (CHN)
enum channelEnum { LEFT, RIGHT, STEREO };

// icon char values for speaker information
enum speakerEnum { SPC_SMALL = 9, L_ON = 22, L_MUTE, L_OFF, R_ON, R_MUTE, R_OFF, SPC_L, SPC_R, VBS_OFF = 128, VBS_ON, TREBLE, BASS, BALANCE };

// mute status (PLA)
// numeric value is the icon char value also
enum muteEnum { MUTE_OFF = R_ON, MUTE_ON };

// current setting to show in settings layout
enum settingType {NONE, VOL, BAS, TRE, BAL, SRC, MUT, CHN, REP, SHU, VBS};

#endif