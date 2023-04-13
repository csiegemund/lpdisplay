
#include <main.h>
#include <my_fonts.h>
#include <strtools.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define U8G2_OBJECT(O, I) U8G2_OBJ(O, I)
#define U8G2_OBJ(O, I) U8G2_ ## O ## _1_ ## I 

#if INTERFACE == I2C
  U8G2_OBJECT(OLED, HW_I2C) u8g2(ROTATION, I2C_RS);  
#elif INTERFACE == SPI_3W
  U8G2_OBJECT(OLED, 3W_HW_SPI) u8g2(ROTATION, SPI_CS, SPI_RS);  
#elif INTERFACE == SPI_4W
  U8G2_OBJECT(OLED, 4W_HW_SPI) u8g2(ROTATION, SPI_CS, SPI_DC, SPI_RS); 
#elif INTERFACE == NULLIF
  U8G2_NULL u8g2(U8G2_R0);
#endif




// initializer for the u8g2 display library - see the u8g2 repo for details
// there you'll find a lot more initializers for various displays
// the params define the physical wiring of the selected display
// adapt this to your board layout

// small & medium SSD1306 OLED at I2C
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g3(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// big SSD1309 OLED at SPI
//U8G2_SSD1309_128X64_NONAME0_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 0, /* dc=*/ 16, /* reset=*/ 15);  

// big SSD1309 OLED at I2C
//U8G2_SSD1309_128X64_NONAME2_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define CMD_LENGTH 3
#define TEXT_LENGTH 50
#define BUFFER_LENGTH CMD_LENGTH + 1 + TEXT_LENGTH + 2

// fonts for top status line
#define TOP_SMALL_FONT audiofont_5x8
#define TOP_BIG_FONT   audiofont_6x12

// fonts for the title information
#define ARTIST_FONT    audiofont_5x8
#define TITLE_FONT     audiofont_6x12
#define ALBUM_FONT     audiofont_5x8

// font for bottom status line
#define BOTTOM_FONT    audiofont_5x8

// font for the value & status icons in popup display
#define ICON_FONT      audioicons_20x20

// time to show pop-up messages for value changes (e.g. volume...) and status changes (e.g. mute)
#define SHOW_VALUE     3000
#define SHOW_STATUS    1500

// bit vector values for all values received by the linkplay device
// this values will be stored together in a 16-bit value for easy update processing
enum values {
  valNone     = 0x0000,
  valSource   = 0x0001,
  valRepeat   = 0x0002,
  valShuffle  = 0x0004,
  valPlay     = 0x0008,
  valChannel  = 0x0010,
  valMute     = 0x0020,
  valVolume   = 0x0040,
  valBass     = 0x0080,
  valTreble   = 0x0100,
  valBalance  = 0x0200,
  valVBass    = 0x0400,
  valArtist   = 0x0800,
  valTitle    = 0x1000,
  valAlbum    = 0x2000,
  valElapsed  = 0x4000
};

// bit vector values for all display areas used in the defined display layouts
// this values will be stored together in a 16-bit value for easy display update processing
enum areas {
  areaNone     = 0x0000,
  areaTopSmall = 0x0001,
  areaTopBig   = 0x0002,
  areaArtist   = 0x0004,
  areaTitle    = 0x0008,
  areaAlbum    = 0x0010,
  areaVolume   = 0x0020,
  areaBassTreb = 0x0040,
  areaSetTitle = 0x0080,
  areaSetScale = 0x0100,
  areaSetIcon  = 0x0200,
  areaBotLine  = 0x0400,
  areaElapsed  = 0x0800,
  areaEmpty    = 0x8000   //draw nothing
};

uint16_t updValues = 0; // bit field for updated values
uint16_t updAreas = 0;  // bit field for display areas to be updated

// define the screen layouts
enum layout {layNoTitles, layTitles, laySetValue, laySetIcon};
layout currentLayout = layNoTitles;

layout lastLayout;
unsigned long timeToLastLayout = 0;


// define per row, which areas are using a particular row
// the draw routine per row is called only if the area is defined here
const uint16_t layoutRows[4][8] = {
  { //Layout NoTitles
  areaTopBig,
  areaTopBig,
  areaVolume,
  areaVolume,
  areaBassTreb,
  areaBassTreb,
  areaBassTreb | areaBotLine,
  areaElapsed
  },
  {//Layout Titles
  areaTopSmall,
  areaTopSmall | areaArtist,
  areaArtist,
  areaTitle,
  areaTitle,
  areaTitle | areaAlbum,
  areaAlbum | areaBotLine,
  areaElapsed
  },
  { //Layout SetValue
  areaSetTitle,
  areaSetTitle | areaSetScale,
  areaSetScale,
  areaSetScale,
  areaSetScale,
  areaSetScale,
  areaBotLine | areaSetScale,
  areaElapsed
  },
  { //Layout SetIcon
  areaSetTitle,
  areaSetTitle,
  areaSetIcon,
  areaSetIcon,
  areaSetIcon,
  areaSetIcon,
  areaBotLine | areaSetIcon,
  areaElapsed
  }
};

// actual areas which needed to be drawn
uint16_t rowLayout[8];
// next row to draw  - currentRow = 8 -> all rows drawn, do nothing
uint8_t currentRow = 0;

// buffer which holds raw data from DATA_SERIAL interface
char inBuffer[BUFFER_LENGTH];
uint8_t posBuffer = 0;


// variables which hold the data sent by the linkplay UART
// title information (if available) (TIT, ALB, ART, VND)
char title[TEXT_LENGTH] = "";
char album[TEXT_LENGTH] = "";
char artist[TEXT_LENGTH] = "";
char vendor[TEXT_LENGTH] = "";

// basic audio settings (VOL, TRE, BAS, BAL)
int8_t volume = 0;
int8_t treble = 0;
int8_t bass = 0;
int8_t balance = 0;

// elaspsed time and duration of current song (ELP)
uint32_t timeElapsed = 0;
uint32_t timeTotal = 0;

// millis counter when next elapsed time should be requested (0 = no request)
unsigned long nextELP = 0;

// value and related text info for the current audio source (SRC)
// numeric value is the related icon char value also
enum sourceEnum { SRC_UNKNOWN, BT, BTC, NET, NETC, USB, LINEIN, OPT, COAX };
uint8_t source = SRC_UNKNOWN;
const char* sourceName[] = { "UNKNOWN", "BLUETOOTH", "BLUETOOTH", "NETWORK", "NETWORK", "USB", "LINE-IN", "OPTICAL", "COAX" };
const char* statusName[] = { "", "OFF", "ON", "OFF", "ON", "", "", "", "" };

// status information (SYS, BTC, WWW)
bool SysEnabled = false;
bool BTConnected = false;
bool NetConnected = false;

// shuffle status (derived from LPM)
// numeric value is the icon char value also
enum shuffleEnum { SEQUENCE = 11, SHUFFLE };
uint8_t shuffleMode = SEQUENCE;

// repeat status (derived from LPM)
// numeric value is the icon char value also
enum repeatEnum { REPEATNONE = 14, REPEATALL, REPEATONE };
uint8_t repeatMode = REPEATNONE;

// play status (PLA)
// numeric value is the icon character value also
enum playEnum { PLA_UNKNOWN, PLAY = 17, PAUSE, STOP, PREV, NEXT };
uint8_t playState = PLA_UNKNOWN;


// channel status (CHN)
enum channelEnum { LEFT, RIGHT, STEREO };
uint8_t channelState = STEREO;

// icon char values for speaker information
enum speakerEnum { SPC_SMALL = 9, L_ON = 22, L_MUTE, L_OFF, R_ON, R_MUTE, R_OFF, SPC_L, SPC_R, VBS_OFF = 128, VBS_ON, TREBLE, BASS, BALANCE };
// virtual bass status (VBS)
uint8_t virtbassState = VBS_OFF;

// mute status (PLA)
// numeric value is the icon char value also
enum muteEnum { MUTE_OFF = R_ON, MUTE_ON };
uint8_t muteState = MUTE_OFF;



char r_top_left_small[20] = "";
char r_top_left_big[20] = "";
char r_top_right_small[20] = "";
char r_top_right_big[20] = "";

char r_artist[TEXT_LENGTH] = "";
char r_title_1[TEXT_LENGTH] = "";
char* r_title_2 = "";
char r_album[TEXT_LENGTH] = "";

enum settingType {NONE, VOL, BAS, TRE, BAL, SRC, MUT, CHN, REP, SHU, VBS};
const char* settingName[] = {"NONE", "VOLUME", "BASS", "TREBLE", "BALANCE", "SOURCE", "MUTE", "CHANNEL", "REPEAT", "SHUFFLE", "VIRTUAL\11BASS" };
const char* onoffName[] = {"OFF", "ON"};
const char* repeatName[] = {"OFF", "ALL", "ONE"};
const char* channelName[] = {"LEFT", "RIGHT", "STEREO"};
uint8_t r_setting;
char r_setting_text[20] = "";
char r_setting_icon[3] = "";

char r_bottom_left[20] = "";
char r_bottom_center[20] = "";
char r_bottom_right[20] = "";
u8g2_uint_t r_bottom_SliderX1 = 0;
u8g2_uint_t r_bottom_SliderX2 = 0;
u8g2_uint_t r_bottom_SliderW = 0xff;

char sendBuffer[25];
char* sendLast = sendBuffer;
unsigned long nextBufferCmdTime = 0;



boolean sendBufferCmd() {

  if ((sendLast > sendBuffer) && (millis() >= nextBufferCmdTime)) {
    SERIAL_DAT.print(sendBuffer);
    debugOut(sendBuffer, '>');
    nextBufferCmdTime = millis() + 100;
    char *to , *from;
    for (to = sendBuffer, from = sendBuffer + strlen(sendBuffer) + 1; from < sendLast; to++, from++) {
      *to = *from;
    }
    sendLast = to;
    return (true);
  } else {
    return (false);
  }
}

boolean addBufferCmd(const char* cmd) {
  if ((sendLast + strlen(cmd) + 1) <= (sendBuffer + sizeof(sendBuffer))) {
    while (*cmd != char(0)) {
      *sendLast++ = *cmd++;
    }
    *sendLast++ = char(0);
    return (true);
  } else {
    return (false);
  }
}

sourceEnum getSourceID(const char* sourceText) 
{
  if (strcmp(sourceText, "NET") == 0) {
      if (NetConnected) {
          return NETC;
      } else {
          return NET;
      }
  } else if (strcmp(sourceText, "USB") == 0) {
      return USB;
  } else if (strcmp(sourceText, "USBDAC") == 0) {
      return USB;
  } else if (strcmp(sourceText, "LINE-IN") == 0) {
      return LINEIN;
  } else if (strcmp(sourceText, "LINE-IN2") == 0) {
      return LINEIN;
  } else if (strcmp(sourceText, "BT") == 0) {
      if (BTConnected) {
          return BTC;
      } else {
          return BT;
      }
  } else if (strcmp(sourceText, "OPT") == 0) {
      return OPT;
  } else if (strcmp(sourceText, "COAX") == 0) {
      return COAX;
  } else {
    return SRC_UNKNOWN;
  }
}

speakerEnum getSpeakerIconLeft() {
  
  if (channelState == RIGHT) {
      return L_OFF;
  } else if (muteState == MUTE_ON) {
      return L_MUTE;
  } else {
      return L_ON;
  }
}

speakerEnum getSpeakerIconRight() {
  
  if (channelState == LEFT) {
      return R_OFF;
  } else if (muteState == MUTE_ON) {
      return R_MUTE;
  } else {
      return R_ON;
  }
}

boolean setValueUint(uint8_t &value, uint8_t newValue, values updVal = valNone) {
  if (value != newValue) {
    value = newValue;
    updValues|= updVal;
    return (true);
  }
  else {
    return (false);
  }
}

boolean setValueInt(int8_t &value, int8_t newValue, values updVal = valNone) {
  if (value != newValue) {
    value = newValue;
    updValues|= updVal;
    return (true);
  }
  else {
    return (false);
  }
}

boolean setValueBool(boolean &value, boolean newValue, values updVal = valNone) {
  if (value != newValue) {
    value = newValue;
    updValues|= updVal;
    return (true);
  }
  else {
    return (false);
  }
}

void setLayout(layout newLayout) {
  if (currentLayout < laySetValue) lastLayout = currentLayout;
  if (currentLayout != newLayout){
    for (uint8_t row = 0; row < 8; row++) {
      if (layoutRows[currentLayout][row] != layoutRows[newLayout][row]) {
        rowLayout[row] = layoutRows[newLayout][row];
      }
    }
    currentLayout = newLayout;
    currentRow = 0;
  }
}

boolean setSource(uint8_t newSource) {
  if (setValueUint(source, newSource, valSource)) {
    strcopyext(&u8g2, artist, "", TEXT_LENGTH, Utf2Ansi, &updValues, valArtist);
    strcopyext(&u8g2, title, "", TEXT_LENGTH, Utf2Ansi, &updValues, valTitle);
    strcopyext(&u8g2, album, "", TEXT_LENGTH, Utf2Ansi, &updValues, valAlbum);
    playState = PLA_UNKNOWN;
    timeElapsed = 0;
    timeTotal = 0;
    nextELP = 0;
    updValues |= valElapsed;
    setLayout(layNoTitles);
    addBufferCmd("STA;");
    addBufferCmd("LPM;");
    addBufferCmd("VBS;");
    return (true);
  } else {
    return (false);
  }
}

void setSettingTitle(settingType setting, const char* valText, int8_t valNum = 0) {
    r_setting = setting;
    char text[20];
    strcpy(text, settingName[setting]);
    char* cursor = text + strlen(text);
    *cursor++ = ':';
    *cursor++ = char(SPC_SMALL);
    if (valText != NULL) {
      strcpy(cursor, valText);
    } else {
      itoa(valNum, cursor, 10);
    }
    strcopyext(&u8g2, r_setting_text, text, 20, None, &updAreas, areaSetTitle);
}

void setSettingSource() {
    r_setting = SRC;
    char text[20];
    strcpy(text, sourceName[source]);
    if (*statusName[source] != char(0)) {
      char* cursor = text + strlen(text);
      *cursor++ = ':';
      *cursor++ = char(SPC_SMALL);
      if ((source == NETC) && (vendor[0] != char(0))) {
        strcpy(cursor, vendor);
      } else {
        strcpy(cursor, statusName[source]);
      }
    }
    strcopyext(&u8g2, r_setting_text, text, 20, None, &updAreas, areaSetTitle);
}

void setSettingScale(char icon) {
    r_setting_icon[0] = char(icon);
    r_setting_icon[1] = char(0);
    updAreas |= areaSetScale;
    setLayout(laySetValue);
    timeToLastLayout = millis() + SHOW_VALUE;
}

void setSettingIcon(char icon1, char icon2 = 0) {
    r_setting_icon[0] = char(icon1);
    r_setting_icon[1] = char(icon2);
    r_setting_icon[2] = char(0);
    updAreas |= areaSetIcon;
    setLayout(laySetIcon);
    timeToLastLayout = millis() + SHOW_STATUS;
}


boolean fillBuffer(char c) {

  if (c == char(13)) {
    inBuffer[posBuffer] = char(0);
    if (posBuffer > 0) {
      posBuffer--;
      if (inBuffer[posBuffer] = ';') {
        inBuffer[posBuffer] = char(0);
      }
    }

    return (posBuffer > 0);
  } else {
    if ((uint8_t(c) >= 32) && (posBuffer < BUFFER_LENGTH - 1)) {
      inBuffer[posBuffer++] = c;
    }
    return false;
  }
}

void resetBuffer() {
  posBuffer = 0;
  inBuffer[0] = char(0);
}

void parseBuffer() {

  if (posBuffer >= CMD_LENGTH) {
    //Split inBuffer into cmd and data strings at fixed pos CMD_LENGTH
    char* inCmd = inBuffer;
    inCmd[CMD_LENGTH] = char(0);
    char* inData = inBuffer + CMD_LENGTH + 1;

    if (strncmp(inCmd, "ELP", CMD_LENGTH) == 0) {
        if (playState = PLAY){
          nextELP += 1000; // default action: request next ELP in 1 sec
        } else {
          nextELP = 0; // no ELP needed further more
        }
        char* time = strtok(inData, "/");
        if (time != NULL) {
          char* strMilli = time + strlen(time) - 3;   //points to last 3 digits = milliseconds
          if (strcmp(strMilli, "000") != 0) {
            //'Elapsed' with 000 as milliseconds are estimated, but complete wrong values -> ignore

            uint32_t tmpElapsed = 0;
            uint32_t tmpTotal = 0;

            if (playState = PLAY){
              // here ELP will start the next second (+ 100 ms extra for the 'jitter')
              // then we want to query another ELP 
              nextELP = millis() + 1100 - atol(strMilli); 
            }
            *strMilli = char(0); //Shorten time string to full seconds
            tmpElapsed = atol(time);
            
            time = strtok(NULL, "/");
            if (time != NULL) {
              strMilli = time + strlen(time) - 3;   //points to last 3 digits = milliseconds
              *strMilli = char(0); //Shorten time string to full seconds
              tmpTotal = atol(time);
            }

            if ((tmpElapsed != timeElapsed) || (tmpTotal != timeTotal)) {
              timeElapsed = tmpElapsed;
              timeTotal = tmpTotal;
              updValues |= valElapsed;
            }
          }
        }
    } else if (strncmp(inCmd, "TIT", CMD_LENGTH) == 0) {
        strcopyext(&u8g2, title, inData, TEXT_LENGTH, Utf2Ansi, &updValues, valTitle);
        if (title[0] != char(0)) setLayout(layTitles);
    } else if (strncmp(inCmd, "ART", CMD_LENGTH) == 0) {
        strcopyext(&u8g2, artist, inData, TEXT_LENGTH, Utf2Ansi, &updValues, valArtist);
        if (artist[0] != char(0)) setLayout(layTitles);
    } else if (strncmp(inCmd, "ALB", CMD_LENGTH) == 0) {
        strcopyext(&u8g2, album, inData, TEXT_LENGTH, Utf2Ansi, &updValues, valAlbum);
        if (album[0] != char(0)) setLayout(layTitles);
    } else if (strncmp(inCmd, "VND", CMD_LENGTH) == 0) {
        if (strcopyext(&u8g2, vendor, inData, TEXT_LENGTH, Utf2Ansi | ToUpper, &updValues, valSource)) {
          playState = PLA_UNKNOWN;
          timeElapsed = 0;
          timeTotal = 0;
          updValues |= valElapsed;
          if (title[0] == char(0)) setLayout(layNoTitles);
          addBufferCmd("STA;");
          addBufferCmd("LPM;");
          addBufferCmd("VBS;");
          addBufferCmd("ELP;");
        };
    } else if (strncmp(inCmd, "SRC", CMD_LENGTH) == 0) {
        setSource(getSourceID(inData));
    } else if (strncmp(inCmd, "WWW", CMD_LENGTH) == 0) {
        switch (inData[0]) {
          case '0': 
            setValueBool(NetConnected, false); 
            if (source == NETC) setValueUint(source, NET, valSource);
            break;
          case '1': 
            setValueBool(NetConnected, true); 
            if (source == NET) setValueUint(source, NETC, valSource);
            break;
        }
    } else if (strncmp(inCmd, "BTC", CMD_LENGTH) == 0) {
        switch (inData[0]) {
          case '0': 
            setValueBool(BTConnected, false); 
            if (source == BTC) setValueUint(source, BT, valSource);
            break;
          case '1': 
            setValueBool(BTConnected, true); 
            if (source == BT) setValueUint(source, BTC, valSource);
            break;
        }
    } else if (strncmp(inCmd, "PLA", CMD_LENGTH) == 0) {
        switch (inData[0]) {
          case '0':
            if (setValueUint(playState, PAUSE, valPlay)) {
              nextELP = 0; // no ELP needed further more
            }
            break;
          case '1': 
            if (setValueUint(playState, PLAY, valPlay)) {
              nextELP = 1; // force ELP request
            }
            break;
        }
    } else if (strncmp(inCmd, "MUT", CMD_LENGTH) == 0) {
        switch (inData[0]) {
          case '0': setValueUint(muteState, MUTE_OFF, valMute); break;
          case '1': setValueUint(muteState, MUTE_ON, valMute); break;
        };
        updAreas |= areaVolume;
    } else if (strncmp(inCmd, "VBS", CMD_LENGTH) == 0) {
        switch (inData[0]) {
          case '0': setValueUint(virtbassState, VBS_OFF, valVBass); break;
          case '1': setValueUint(virtbassState, VBS_ON, valVBass); break;
        }
    } else if (strncmp(inCmd, "LPM", CMD_LENGTH) == 0) {
        if (strcmp(inData, "SEQUENCE") == 0) {
          setValueUint(shuffleMode, SEQUENCE, valShuffle);
          setValueUint(repeatMode, REPEATNONE, valRepeat);
        } else if (strcmp(inData, "SHUFFLE") == 0) {
          setValueUint(shuffleMode, SHUFFLE, valShuffle);
          setValueUint(repeatMode, REPEATNONE, valRepeat);
        } else if (strcmp(inData, "REPEATALL") == 0) {
          setValueUint(shuffleMode, SEQUENCE, valShuffle);
          setValueUint(repeatMode, REPEATALL, valRepeat);
        } else if (strcmp(inData, "REPEATONE") == 0) {
          setValueUint(shuffleMode, SEQUENCE, valShuffle);
          setValueUint(repeatMode, REPEATONE, valRepeat);
        } else if (strcmp(inData, "REPEATSHUFFLE") == 0) {
          setValueUint(shuffleMode, SHUFFLE, valShuffle);
          setValueUint(repeatMode, REPEATALL, valRepeat);
        }
    } else if (strncmp(inCmd, "CHN", CMD_LENGTH) == 0) {
        switch (inData[0]) {
          case 'L': setValueUint(channelState, LEFT, valChannel); break;
          case 'R': setValueUint(channelState, RIGHT, valChannel); break;
          case 'S': setValueUint(channelState, STEREO, valChannel); break;
        }
    } else if (strncmp(inCmd, "VOL", CMD_LENGTH) == 0) {
        if (setValueInt(volume, atol(inData), valVolume)){
        };
    } else if (strncmp(inCmd, "BAS", CMD_LENGTH) == 0) {
        if (setValueInt(bass, atol(inData), valBass)){
        };
    } else if (strncmp(inCmd, "TRE", CMD_LENGTH) == 0) {
        if (setValueInt(treble, atol(inData), valTreble)){
        };
    } else if (strncmp(inCmd, "BAL", CMD_LENGTH) == 0) {
        setValueInt(balance, atol(inData), valBalance);
    } else if (strncmp(inCmd, "SYS", CMD_LENGTH) == 0) {
        if (strcmp(inData, "ON") == 0) {
          setValueBool(SysEnabled, true);
          addBufferCmd("STA;");
          addBufferCmd("LPM;");
          addBufferCmd("VBS;");
          updAreas = 0xffff;  //force all areas to be drawn on screen
        } else if (strcmp(inData, "STANDBY") == 0) {
          setValueBool(SysEnabled, false);
        } else if (strcmp(inData, "OFF") == 0) {
          setValueBool(SysEnabled, false);
        }
        u8g2.setPowerSave(SysEnabled?0:1);
    } else if (strncmp(inCmd, "STA", CMD_LENGTH) == 0) {
        char* strToken = strtok(inData, ",");
        if (strToken != NULL) {
          //SRC
          setSource(getSourceID(strToken));
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // MUT
          switch (strToken[0]) {
            case '0': setValueUint(muteState, MUTE_OFF, valMute); break;
            case '1': setValueUint(muteState, MUTE_ON, valMute); break;
          }
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          //VOL
          setValueInt(volume, atol(strToken), valVolume);
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          //TRE
          setValueInt(treble, atol(strToken), valTreble);
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          //BAS
          setValueInt(bass, atol(strToken), valBass);
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // NET state -> ignore
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // WWW 
          switch (strToken[0]) {
            case '0': 
              setValueBool(NetConnected, false); 
              if (source == NETC) setValueUint(source, NET, valSource);
              break;
            case '1': 
              setValueBool(NetConnected, true); 
              if (source == NET) setValueUint(source, NETC, valSource);
              break;
          }
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          //PLA
          switch (strToken[0]) {
            case '0': 
              if (playState != PLA_UNKNOWN) {
                if (setValueUint(playState, PAUSE, valPlay)) {
                  nextELP = 0; // no ELP needed further more
                }
              }
              break;
            case '1': 
              if (setValueUint(playState, PLAY, valPlay)) {
                nextELP = 1; // force ELP query
              }
              break;
          }
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // LED state -> ignore
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // UPGRADING state -> ignore
        }
    }
  }
}


void processValues() {

  if ((updValues & ( valBass | valTreble | valVBass | valVolume | valChannel | valMute)) > 0) {
    //IMPORTANT: r_top_right_* must be updated before r_top_left_*
    char tmp[20] = "";
    char* cursor = tmp;

    if (virtbassState != VBS_OFF) {
        *cursor++ = char(virtbassState);
    }
    *cursor = NULL;
    strcopyext(&u8g2, r_top_right_big, tmp, 20, None, &updAreas, areaTopBig);

    cursor = tmp;
    if (bass != 0) {
        *cursor++ = char(BASS);
        cursor = formatValue(cursor, bass, 1);
        *cursor++ = char(SPC_SMALL);
    }
    if (treble != 0) {
        *cursor++ = char(TREBLE);
        cursor = formatValue(cursor, treble, 1);
        *cursor++ = char(SPC_SMALL);
    }
    if (virtbassState != VBS_OFF) {
        *cursor++ = char(virtbassState);
        *cursor++ = char(SPC_SMALL);
    }
    *cursor++ = char(getSpeakerIconLeft());
    cursor = formatValue(cursor, volume, 2);
    *cursor++ = char(getSpeakerIconRight());
    *cursor = NULL;
    strcopyext(&u8g2, r_top_right_small, tmp, 20, None, &updAreas, areaTopSmall);
  }

  if ((updValues & valSource) > 0) {
    char tmp[20] = {char(source), char(SPC_SMALL)};

    if ((*vendor == NULL) || (source != NETC)) {
      strncpy(&tmp[2], sourceName[source], 17);
    } else {
      strncpy(&tmp[2], vendor, 17);
    }

    u8g2.setFont(TOP_SMALL_FONT);
    strcopyext(&u8g2, r_top_left_small, tmp, 20, None, &updAreas, areaTopSmall, 128 - u8g2.getStrWidth(r_top_right_small) - 2);

    u8g2.setFont(TOP_BIG_FONT);
    strcopyext(&u8g2, r_top_left_big, tmp, 20, None, &updAreas, areaTopBig, 128 - u8g2.getStrWidth(r_top_right_big) - 2);

    setSettingSource();
    setSettingIcon(source);
  }

  if ((updValues & (valVolume | valChannel | valMute)) > 0) {
      //No formats needed
      updAreas |= areaVolume;
  }

  if ((updValues & (valTreble | valBass)) > 0) {
      //No formats needed
      updAreas |= areaBassTreb;
  }

  if ((updValues & valArtist) > 0) {
    u8g2.setFont(ARTIST_FONT);
    strcopyext(&u8g2, r_artist, artist, TEXT_LENGTH, None, &updAreas, areaArtist, u8g2.getDisplayWidth());
  }

  if ((updValues & valTitle) > 0) {
    u8g2.setFont(TITLE_FONT);
    strcopyext(&u8g2, r_title_1, title, TEXT_LENGTH, None, &updAreas, areaTitle, u8g2.getDisplayWidth(), &r_title_2, "\x85", 164); //, dbg );
  }
  
  if ((updValues & valAlbum) > 0) {
    u8g2.setFont(ALBUM_FONT); 
    strcopyext(&u8g2, r_album, album, TEXT_LENGTH, None, &updAreas, areaAlbum, u8g2.getDisplayWidth());
  }

  if (r_setting != SRC) {
    if ((updValues & valVolume) > 0) {
      setSettingTitle(VOL, NULL, volume);
      setSettingScale(R_ON);
    }

    if ((updValues & valBass) > 0) {
      setSettingTitle(BAS, NULL, bass);
      setSettingScale(BASS);
    }

    if ((updValues & valTreble) > 0) {
      setSettingTitle(TRE, NULL, treble);
      setSettingScale(TREBLE);
    }

    if ((updValues & valRepeat) > 0) {
      setSettingTitle(REP, repeatName[repeatMode - REPEATNONE]);
      setSettingIcon(repeatMode);
    }

    if ((updValues & valShuffle) > 0) {
      setSettingTitle(SHU, onoffName[shuffleMode - SEQUENCE]);
      setSettingIcon(shuffleMode);
    }
    
    if ((updValues & valMute) > 0) {
      setSettingTitle(MUT, onoffName[muteState - MUTE_OFF]);
      setSettingIcon(muteState);
    }

    if ((updValues & valVBass) > 0) {
      setSettingTitle(VBS, onoffName[virtbassState-VBS_OFF]);
      setSettingIcon(virtbassState);
    }

    if ((updValues & valChannel) > 0) {
      setSettingTitle(CHN, channelName[channelState]);
      setSettingIcon(channelState==RIGHT?L_OFF:L_ON, channelState==LEFT?R_OFF:R_ON);
    }
  }

  if ((updValues & (valPlay | valRepeat | valShuffle | valElapsed)) > 0) {
    char tmp[20] = "";
    char* cursor = tmp;

    u8g2.setFont(BOTTOM_FONT);
    if (playState != PLA_UNKNOWN) {
      if (shuffleMode != SEQUENCE) {
        *cursor++ = char(shuffleMode);
        *cursor++ = char(SPC_SMALL);
      }
      if (repeatMode != REPEATNONE) {
        *cursor++ = char(repeatMode);
        *cursor++ = char(SPC_SMALL);
      }
      *cursor++ = char(playState);
      *cursor++ = char(SPC_SMALL);
    }
    if ((timeElapsed != 0) && (timeTotal != 0)) {
      formatTime(cursor, timeElapsed);
    } else {
      *cursor = char(0);
    }
    strcopyext(&u8g2, r_bottom_left, tmp, 20, None, &updAreas, areaElapsed);

    if ((timeElapsed != 0) && (timeTotal == 0)) {
      formatTime(tmp, timeElapsed);
    } else {
      tmp[0] = char(0);
    }
    if ((*r_bottom_center == char(0)) != (*tmp == char(0))) {
      //display switched from title view to radio view or vice versa
      updAreas |= areaTitle; //redraw title area
    }
    strcopyext(&u8g2, r_bottom_center, tmp, 20, None, &updAreas, areaElapsed);

    if ((timeElapsed != 0) && (timeTotal != 0)) {
      formatTime(tmp, timeTotal);
    } else {
      tmp[0] = char(0);
    }
    strcopyext(&u8g2, r_bottom_right, tmp, 20, None, &updAreas, areaElapsed);

    if ((updAreas & areaElapsed) > 0) {
      //some time data has changed -> recalc slider
      r_bottom_SliderX1 = u8g2.getStrWidth(r_bottom_left) + 5;
      r_bottom_SliderX2 = 128 - u8g2.getStrWidth(r_bottom_right) - 5;
      if ((timeElapsed != 0) && (timeTotal != 0)) {
        r_bottom_SliderW = (timeElapsed * (r_bottom_SliderX2 - r_bottom_SliderX1) / timeTotal);
      } else {
        r_bottom_SliderW = 0xff;
      }
    }

  }
}

void drawRow(uint16_t rowLayout) {
      
  if ((rowLayout & areaTopSmall ) > 0) {
    //draw small top row
    u8g2.setFont(TOP_SMALL_FONT);
    drawStrAligned(&u8g2, 10, r_top_left_small, alnLeft);
    drawStrAligned(&u8g2, 10, r_top_right_small, alnRight);
    u8g2.drawHLine(0, 13, 128);
  }
  if ((rowLayout & areaTopBig ) > 0) {
    //draw big top row
    u8g2.setFont(TOP_BIG_FONT);
    drawStrAligned(&u8g2, 11, r_top_left_big, alnLeft);
    drawStrAligned(&u8g2, 11, r_top_right_big, alnRight);
    u8g2.drawHLine(0, 14, 128);
  }
  if ((rowLayout & areaArtist) > 0) {
    //draw artist
    u8g2.setFont(ARTIST_FONT); 
    drawStrAligned(&u8g2, 21, r_artist, alnLeft);
  }
  if ((rowLayout & areaTitle) > 0) {
    //draw title
    u8g2.setFont(TITLE_FONT); 
    alignment aln = alnLeft;
    if ((timeTotal == 0) && (*r_artist == char(0)) && (*r_album == char(0))) {
      //title contains radio station -> center
      aln = alnCenter;
    }
    if (*r_title_2 == char(0)) {
      drawStrAligned(&u8g2, 37, r_title_1, aln);
    } else {
      drawStrAligned(&u8g2, 32, r_title_1, aln);
      drawStrAligned(&u8g2, 43, r_title_2, aln);
    }
  }
  if ((rowLayout & areaAlbum) > 0) {
    //draw album
    u8g2.setFont(ALBUM_FONT);
    drawStrAligned(&u8g2, 52, r_album);
  }
  if ((rowLayout & areaVolume) > 0) {
    //draw volume
    u8g2.setFont(ICON_FONT);
    u8g2.drawGlyph(14, 30, getSpeakerIconLeft());
    drawScale(&u8g2, 40, 28, 3, 5, 10, 0, (volume + 5) / 10 - 1, 1, 1);
    u8g2.drawGlyph(92, 30, getSpeakerIconRight());
  }
  if ((rowLayout & areaBassTreb) > 0) {
    //draw bass & treble
    u8g2.setFont(ICON_FONT);
    u8g2.drawGlyph(3, 46, BASS);
    if (bass < 0) {
      drawScale(&u8g2, 21, 42, 2, 4, 10, 5 + (bass - 1) / 2, 5, ((bass - 1) / 2) * 2 - 1, 2);
    } else {
      drawScale(&u8g2, 21, 42, 2, 4, 10, 5, 4 + (bass + 1) / 2, 2, 2);
    }
    u8g2.drawGlyph(68, 46, TREBLE);
    if (treble < 0) {
      drawScale(&u8g2, 84, 42, 2, 4, 10, 5 + (treble - 1) / 2, 5, ((treble - 1) / 2) * 2 - 1, 2);
    } else {
      drawScale(&u8g2, 84, 42, 2, 4, 10, 5, 4 + (treble + 1) / 2, 2, 2);
    }
  }
  if ((rowLayout & areaSetTitle) > 0) {
    //draw setting text
    u8g2.setFont(TOP_BIG_FONT);
    drawStrAligned(&u8g2, 9, r_setting_text, alnCenter);
  }
  if ((rowLayout & areaSetScale) > 0) {
    //draw setting scale
    u8g2.setFont(ICON_FONT);
    drawStrAligned(&u8g2, 34, r_setting_icon, alnLeft);
    switch (r_setting)
    {
      case VOL:
        drawScale(&u8g2, 24, 51, 3, 5, 20, 0, (volume + 2) / 5 - 1, 2, 2);
        break;
      case BAS:
        if (bass < 0) {
          drawScale(&u8g2, 24, 31, 3, 5, 20, 10 + bass, 9, bass * 2 - 1, 2);
        } else {
          drawScale(&u8g2, 24, 31, 3, 5, 20, 10, 9 + bass, 2, 2);
        }
        break;
      case TRE:
        if (treble < 0) {
          drawScale(&u8g2, 24, 31, 3, 5, 20, 10 + treble, 9, treble * 2 - 1, 2);
        } else {
          drawScale(&u8g2, 24, 31, 3, 5, 20, 10, 9 + treble, 2, 2);
        }
        break;
      case BAL:
        break;
    }
  }
  if ((rowLayout & areaSetIcon) > 0) {
    //draw setting icon
    u8g2.setFont(ICON_FONT);
    drawStrAligned(&u8g2, 34, r_setting_icon, alnCenter);
  }
  if ((rowLayout & areaBotLine) > 0) {
    //draw bottom line
    u8g2.drawHLine(0, 54, 128);
  }
  if ((rowLayout & areaElapsed) > 0) {
    //draw bottom status row
    u8g2.setFont(BOTTOM_FONT);
    drawStrAligned(&u8g2, 63, r_bottom_left, alnLeft);
    if (r_bottom_SliderW != 0xff) {
      u8g2.drawRBox(r_bottom_SliderX1 - 3, 57, r_bottom_SliderW + 6, 7, 3);
    } else {
      drawStrAligned(&u8g2, 63, r_bottom_center, alnCenter);
    }
    drawStrAligned(&u8g2, 63, r_bottom_right, alnRight);
  }
}


void setup(void) {
 
  #ifdef SERIAL_DBG
    SERIAL_DBG.begin(115200);				// Start writing to DBG_SERIAL communication interface
  #endif
  SERIAL_DAT.begin(115200);				// Start reading from DATA_SERIAL communication interface

  debugOut("Init start");
  u8g2.initDisplay();
  u8g2.clearDisplay();
  u8g2.setPowerSave(0);
  u8g2.setFontMode(1);
  debugOut("Init finished");
}

void loop(void) {

  if (SERIAL_DAT.available() > 0) {
      while (SERIAL_DAT.available() > 0) {
        if (fillBuffer(SERIAL_DAT.read())) {
          debugOut(inBuffer, '<');
          parseBuffer();
          resetBuffer();
          sendBufferCmd();
        }
      }
  } else if (sendBufferCmd()) {
      //action is done in sendBufferCmd
  } else if ((nextELP > 0) && (millis() >= nextELP)) {  //We want an ELP update and waiting time has been reached
      addBufferCmd("ELP;");  //request ELP update
      //debugOut("nextELP", '#');
      if (playState == PLAY) nextELP += 1000; //next ELP request in 1s if no answer to this request
  } else if (updValues > 0) {
      processValues();
      updValues = 0;


  } else if (updAreas > 0) {
      for (uint8_t row = 0; row < 8; row++) {
        if ((updAreas & layoutRows[currentLayout][row]) > 0) {
          rowLayout[row] |= layoutRows[currentLayout][row];
        }
      }
      updAreas = 0;
      currentRow = 0;
  } 
  else if (currentRow < 8) {
      if (currentRow == 0) debugOut("Start Screen update");
      if (rowLayout[currentRow] > 0) {
        u8g2.setBufferCurrTileRow(currentRow); 
        u8g2.clearBuffer();
        drawRow(rowLayout[currentRow]);
        u8g2.sendBuffer();
        rowLayout[currentRow] = 0;
      }
      currentRow++;
      if (currentRow == 8) debugOut("Finish Screen update");
  } else if ((timeToLastLayout > 0) && (millis() >= timeToLastLayout)) {  //switch back to normal layout
      setLayout(lastLayout);
      timeToLastLayout = 0; //done
      r_setting = NONE;
  
  }
}
   
