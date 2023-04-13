#include <Arduino.h>
#include <U8g2lib.h>

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

#define NULLIF 0
#define I2C    1
#define SPI_3W 2
#define SPI_4W 3

#ifndef OLED
  #define OLED SSD1309_128X64_NONAME2
#endif

#ifndef INTERFACE
  #define INTERFACE I2C
#endif

#ifndef ROTATION
  #define ROTATION U8G2_R0
#endif

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