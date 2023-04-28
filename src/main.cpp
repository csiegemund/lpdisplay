
#include <main.h>
#include <my_fonts.h>
#include <strtools.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


// helper macros to build u8g2 object from display settings
#define U8G2_OBJECT(C, W, H, I) U8G2_OBJ(C, W, H, I)
#define U8G2_OBJ(C, W, H, I) U8G2_ ## C ## _ ## W ## X ## H ## _1_ ## I 

#define U8G2_OBJECT_B(C, W, H, B, I) U8G2_OBJ_B(C, W, H, B, I)
#define U8G2_OBJ_B(C, W, H, B, I) U8G2_ ## C ## _ ## W ## X ## H ## _ ## B ## _1_ ## I 

#define U8G2_OBJECT_C(CB, I) U8G2_OBJ_C(CB, I)
#define U8G2_OBJ_C(CB, I) U8G2_ ## CB ## _1_ ## I 

// initializer for the u8g2 display library - see the u8g2 repo for details
// we build the suitable u8g2 type from distinct defines for controller, width, height, brand and interface
// and use the wiring defines as parameter for the constructor
// you could combine values only when a predefined constructor exists in u8g2 library!

#if DISP_INTERFACE == NO_IF
      U8G2_NULL u8g2(U8G2_R0);
#elif defined(DISP_CUSTOM)
    // U8G2 contructur does not meet the standard structure -> set DISP_CUSTOM
    // DISP_WIDTH and DISP_HEIGHT must be set anyway!
    #if DISP_INTERFACE == I2C
      U8G2_OBJECT_C(DISP_CUSTOM, HW_I2C) u8g2(DISP_ROTATION, I2C_RS);  
    #elif DISP_INTERFACE == SPI_3W
      U8G2_OBJECT_C(DISP_CUSTOM, 3W_HW_SPI) u8g2(DISP_ROTATION, SPI_CS, SPI_RS);  
    #elif DISP_INTERFACE == SPI_4W
      U8G2_OBJECT_C(DISP_CUSTOM, 4W_HW_SPI) u8g2(DISP_ROTATION, SPI_CS, SPI_DC, SPI_RS); 
    #endif
#elif defined(DISP_BRAND)
  	// U8G2 contructor with brand name after resolution
    #if DISP_INTERFACE == I2C
      U8G2_OBJECT_B(DISP_CONTROLLER, DISP_WIDTH, DISP_HEIGHT, DISP_BRAND, HW_I2C) u8g2(DISP_ROTATION, I2C_RS);  
    #elif DISP_INTERFACE == SPI_3W
      U8G2_OBJECT_B(DISP_CONTROLLER, DISP_WIDTH, DISP_HEIGHT, DISP_BRAND, 3W_HW_SPI) u8g2(DISP_ROTATION, SPI_CS, SPI_RS);  
    #elif DISP_INTERFACE == SPI_4W
      U8G2_OBJECT_B(DISP_CONTROLLER, DISP_WIDTH, DISP_HEIGHT, DISP_BRAND, 4W_HW_SPI) u8g2(DISP_ROTATION, SPI_CS, SPI_DC, SPI_RS); 
    #endif
#else
  	// U8G2 contructor without brand name 
    #if DISP_INTERFACE == I2C
      U8G2_OBJECT(DISP_CONTROLLER, DISP_WIDTH, DISP_HEIGHT, HW_I2C) u8g2(DISP_ROTATION, I2C_RS);  
    #elif DISP_INTERFACE == SPI_3W
      U8G2_OBJECT(DISP_CONTROLLER, DISP_WIDTH, DISP_HEIGHT, 3W_HW_SPI) u8g2(DISP_ROTATION, SPI_CS, SPI_RS);  
    #elif DISP_INTERFACE == SPI_4W
      U8G2_OBJECT(DISP_CONTROLLER, DISP_WIDTH, DISP_HEIGHT, 4W_HW_SPI) u8g2(DISP_ROTATION, SPI_CS, SPI_DC, SPI_RS); 
    #endif
#endif

// precalculated values for layout positions
// derived from display size and selected fonts (main.h) 
// all values are filled during setup()
// and used by drawRow()

u8g2_int_t dispWidth, dispHeight;
u8g2_int_t yTop_s, yTop_b, yLineTop_s, yLineTop_b;
u8g2_int_t yBottom, yLineBottom, ySlider, hSlider, rSlider;
u8g2_int_t yArtist, yTitle, yTitle_1, yTitle_2, yAlbum;
u8g2_int_t xVolume, xBass, xTreble;
u8g2_int_t ySetTitle, ySetScale_h, ySetScale_l, ySetIcon;
u8g2_int_t barWidth_s, stepWidth_s, barWidth_b, stepWidth_b;


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


// define per row, which areas use a particular row.
// the draw routine for a particular row is called only if at least 
// one of the areas assigned here has a changed content.
const uint16_t layoutRows[4][DISP_PAGECOUNT] = {
  { //Layout NoTitles
  areaTopBig,
  areaTopBig,
  areaTopBig | areaVolume,
  areaVolume,
  areaVolume | areaBassTreb,
  areaBassTreb,
  areaBassTreb | areaBotLine,
  areaElapsed
  },
  {//Layout Titles
  areaTopSmall,
  areaTopSmall | areaArtist,
  areaArtist | areaTitle,
  areaArtist | areaTitle,
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
uint16_t rowLayout[DISP_PAGECOUNT];
// next row to draw  - currentRow = DISP_PAGECOUNT -> all rows drawn, do nothing
uint8_t currentRow = 0; // initially start with a complete screen refresh

// buffer which holds raw data from DATA_SERIAL interface
char inBuffer[BUFFER_LENGTH];
uint8_t posBuffer = 0;

// variables which hold the data sent by the linkplay UART
// filled by parseBuffer()
// used by processValues()

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
uint8_t source = SRC_UNKNOWN;
const char* sourceName[] = { "UNKNOWN", "BLUETOOTH", "BLUETOOTH", "NETWORK", "NETWORK", "USB", "LINE-IN", "OPTICAL", "COAX" };
const char* statusName[] = { "", "OFF", "ON", "OFF", "ON", "", "", "", "" };

// status information (SYS, BTC, WWW)
bool SysEnabled = false;
bool BTConnected = false;
bool NetConnected = false;

// shuffle status (derived from LPM)
uint8_t shuffleMode = SEQUENCE;

// repeat status (derived from LPM)
uint8_t repeatMode = REPEATNONE;

// play status (PLA)
uint8_t playState = PLA_UNKNOWN;


// channel status (CHN)
uint8_t channelState = STEREO;

// virtual bass status (VBS)
uint8_t virtbassState = VBS_OFF;

// mute status (PLA)
uint8_t muteState = MUTE_OFF;


// precalculated strings and screen positions
// filled by processValues()
// used by drawRow()

// top text and icons (small and big versions)
char r_top_left_small[20] = "";
char r_top_left_big[20] = "";
char r_top_right_small[20] = "";
char r_top_right_big[20] = "";

// formatted title information
char r_artist[TEXT_LENGTH] = "";
char r_title_1[TEXT_LENGTH] = "";
char* r_title_2 = "";
char r_album[TEXT_LENGTH] = "";

// formatted setting (changed value) information
const char* settingName[] = {"NONE", "VOLUME", "BASS", "TREBLE", "BALANCE", "SOURCE", "MUTE", "CHANNEL", "REPEAT", "SHUFFLE", "VIRTUAL\11BASS" };
const char* onoffName[] = {"OFF", "ON"};
const char* repeatName[] = {"OFF", "ALL", "ONE"};
const char* channelName[] = {"LEFT", "RIGHT", "STEREO"};
uint8_t r_setting;
char r_setting_text[20] = "";
char r_setting_icon[3] = "";

// bottom text, icons and elapsed slider position
char r_bottom_left[20] = "";
char r_bottom_center[20] = "";
char r_bottom_right[20] = "";
u8g2_uint_t r_bottom_SliderX1 = 0;
u8g2_uint_t r_bottom_SliderX2 = 0;
u8g2_uint_t r_bottom_SliderW = 0xff;


// buffer to send requests to the UART
// to manage to send one request per 100 ms
char sendBuffer[50];
char* sendLast = sendBuffer;
unsigned long nextBufferCmdTime = 0;


// extract one request from sendBuffer and send it to the arylic board
// move forward remaining requests
// will be periodically called from main loop. 
boolean sendBufferCmd() {

  if ((sendLast > sendBuffer) && (millis() >= nextBufferCmdTime)) {
    SERIAL_DAT.print(sendBuffer);
    debugOut(sendBuffer, '>');
    nextBufferCmdTime = millis() + 100; //wait time before next request will be sent
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

// add another request to the request buffer queue
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

// parse the source from string into enumeration value
// consider the connection flags for NET and BT
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

// returns the left speaker charcter (icon)
// depends on channelState & muteState
speakerEnum getSpeakerIconLeft() {
  
  if (channelState == RIGHT) {
      return L_OFF;
  } else if (muteState == MUTE_ON) {
      return L_MUTE;
  } else {
      return L_ON;
  }
}

// returns the right speaker charcter (icon)
// depends on channelState & muteState
speakerEnum getSpeakerIconRight() {
  
  if (channelState == LEFT) {
      return R_OFF;
  } else if (muteState == MUTE_ON) {
      return R_MUTE;
  } else {
      return R_ON;
  }
}

// setValue functions: They set a value to a status variable, if new value differs
//                     and then set the assigned flag in the updValues bit vector

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

// set new layout screen and mark all rows for updating
// if we switch to a new standard layout (< laySetvalue) 
// we save this as lastLayout. We go back to this layout
// after a setting screen is shown for a short duration
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

// change to a new audio source
// this will reset title info, play state and elapsed times
// then we request current status information
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

// create the title of the setting screens 
// (wich are shown in case of changes within arylic board)
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

// special version of above for changing audio source
// if a vendor is set, show this instaed of ON / OFF
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

// prepare string & screen to display a changed numeric value
void setSettingScale(char icon) {
    r_setting_icon[0] = char(icon);
    r_setting_icon[1] = char(0);
    updAreas |= areaSetScale;
    setLayout(laySetValue);
    timeToLastLayout = millis() + SHOW_VALUE_TIME;
}

// prepare string & screen to display a changed status value
void setSettingIcon(char icon1, char icon2 = 0) {
    r_setting_icon[0] = char(icon1);
    r_setting_icon[1] = char(icon2);
    r_setting_icon[2] = char(0);
    updAreas |= areaSetIcon;
    setLayout(laySetIcon);
    timeToLastLayout = millis() + SHOW_STATUS_TIME;
}

// read incoming char from UART buffer and forward this 
// to inBuffer. If a line end is reached remove a potential ';' at
// the end.
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

// empty inBuffer
void resetBuffer() {
  posBuffer = 0;
  inBuffer[0] = char(0);
}

// parse the received line in inBuffer and fill all affected status
// values from the inBuffer content. Update assigned updValues bit vector
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

            // compare reveived times with current ones, set update flag if changed
            if ((tmpElapsed != timeElapsed) || (tmpTotal != timeTotal)) {
              timeElapsed = tmpElapsed;
              timeTotal = tmpTotal;
              updValues |= valElapsed;
            }
          }
        }
    } else if (strncmp(inCmd, "TIT", CMD_LENGTH) == 0) {
        // convert title string to ANSI and update, if changed. Switch to layTitle 
        strcopyext(&u8g2, title, inData, TEXT_LENGTH, Utf2Ansi, &updValues, valTitle);
        if (title[0] != char(0)) setLayout(layTitles);
    } else if (strncmp(inCmd, "ART", CMD_LENGTH) == 0) {
        // convert artist string to ANSI and update, if changed. Switch to layTitle 
        strcopyext(&u8g2, artist, inData, TEXT_LENGTH, Utf2Ansi, &updValues, valArtist);
        if (artist[0] != char(0)) setLayout(layTitles);
    } else if (strncmp(inCmd, "ALB", CMD_LENGTH) == 0) {
        // convert album string to ANSI and update, if changed. Switch to layTitle 
        strcopyext(&u8g2, album, inData, TEXT_LENGTH, Utf2Ansi, &updValues, valAlbum);
        if (album[0] != char(0)) setLayout(layTitles);
    } else if (strncmp(inCmd, "VND", CMD_LENGTH) == 0) {
        // convert vendor string to ANSI and update, if changed. Reset elapsed time and playstate
        // request new status information
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
        // update audio source
        setSource(getSourceID(inData));
    } else if (strncmp(inCmd, "WWW", CMD_LENGTH) == 0) {
        // update network connect status
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
        // update bluetooth connect status
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
        // update play mode status
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
        // update mute status
        switch (inData[0]) {
          case '0': setValueUint(muteState, MUTE_OFF, valMute); break;
          case '1': setValueUint(muteState, MUTE_ON, valMute); break;
        };
        updAreas |= areaVolume;
    } else if (strncmp(inCmd, "VBS", CMD_LENGTH) == 0) {
        // update virtual bass status
        switch (inData[0]) {
          case '0': setValueUint(virtbassState, VBS_OFF, valVBass); break;
          case '1': setValueUint(virtbassState, VBS_ON, valVBass); break;
        }
    } else if (strncmp(inCmd, "LPM", CMD_LENGTH) == 0) {
        // update shuffle mode and repeat mode status
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
        // update channel status
        switch (inData[0]) {
          case 'L': setValueUint(channelState, LEFT, valChannel); break;
          case 'R': setValueUint(channelState, RIGHT, valChannel); break;
          case 'S': setValueUint(channelState, STEREO, valChannel); break;
        }
    } else if (strncmp(inCmd, "VOL", CMD_LENGTH) == 0) {
        // update audio volume
        if (setValueInt(volume, atol(inData), valVolume)){
        };
    } else if (strncmp(inCmd, "BAS", CMD_LENGTH) == 0) {
        // update audio bass
        if (setValueInt(bass, atol(inData), valBass)){
        };
    } else if (strncmp(inCmd, "TRE", CMD_LENGTH) == 0) {
        // update audio treble
        if (setValueInt(treble, atol(inData), valTreble)){
        };
    } else if (strncmp(inCmd, "BAL", CMD_LENGTH) == 0) {
        // update audio balance
        // currently not shown in the display as not changeable
        // via remote or app
        setValueInt(balance, atol(inData), valBalance);
    } else if (strncmp(inCmd, "LED", CMD_LENGTH) == 0) {
        // if configured, change display brightness when arylic LED is swiched on / off
        // if LED not visible in the target device this is a convienient way
        // to dim the display through remote
        switch (inData[0]) {
          case '0': u8g2.setContrast(LED_OFF_BRIGHTNESS); break;
          case '1': u8g2.setContrast(LED_ON_BRIGHTNESS); break;
        }
    } else if (strncmp(inCmd, "SYS", CMD_LENGTH) == 0) {
        // query current status values when the arylic board is switched on
        // switch the display on / off according to board status
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
        // parse combined status message
        // SRC,MUT,VOL,TRE,BAS,NET,WWW,PLA,LED,UPGRADING
        char* strToken = strtok(inData, ",");
        if (strToken != NULL) {
          // SRC
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
          // VOL
          setValueInt(volume, atol(strToken), valVolume);
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // TRE
          setValueInt(treble, atol(strToken), valTreble);
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // BAS
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
          // PLA
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
          // LED
          switch (strToken[0]) {
            case '0': u8g2.setContrast(LED_OFF_BRIGHTNESS); break;
            case '1': u8g2.setContrast(LED_ON_BRIGHTNESS); break;
          }
          strToken = strtok(NULL, ",");
        }
        if (strToken != NULL) {
          // UPGRADING state -> ignore
        }
    }
  }
}

// check the updValues bit vector for changed fields and prepare
// the strings / positions used for presentation on the display
// mark the related display areas for redrawing
void processValues() {

  if ((updValues & (valSource | valBass | valTreble | valVBass | valVolume | valChannel | valMute)) > 0) {
    // update top row strings (right & left)
    // IMPORTANT: r_top_right_* must be updated before r_top_left_*
    char tmp[20] = "";
    char* cursor = tmp;

    // icon for virtual bass
    if (virtbassState != VBS_OFF) {
        *cursor++ = char(virtbassState);
    }
    // for the 'big' top row no more info is shown on right side
    // as the other values are shown in the main area
    // if changed copy to assigned string
    *cursor = NULL;
    strcopyext(&u8g2, r_top_right_big, tmp, 20, None, &updAreas, areaTopBig);
    
    
    cursor = tmp; // reuse tmp string
    // icon & value for the bass value (if not neutral)
    if (bass != 0) {
        *cursor++ = char(BASS);
        cursor = formatValue(cursor, bass, 1);
        *cursor++ = char(SPC_SMALL);
    }
    // icon & value for the treble value (if not neutral)
    if (treble != 0) {
        *cursor++ = char(TREBLE);
        cursor = formatValue(cursor, treble, 1);
        *cursor++ = char(SPC_SMALL);
    }
    // icon for virtual bass
    if (virtbassState != VBS_OFF) {
        *cursor++ = char(virtbassState);
        *cursor++ = char(SPC_SMALL);
    }
    // icon for speakers (show mute and channel state) & volume value
    *cursor++ = char(getSpeakerIconLeft());
    cursor = formatValue(cursor, volume, 2);
    *cursor++ = char(getSpeakerIconRight());
    *cursor = NULL;
    // if changed copy to assigned string
    strcopyext(&u8g2, r_top_right_small, tmp, 20, None, &updAreas, areaTopSmall);

    // build source information: icon + name or vendor
    tmp[0] = char(source);
    tmp[1] = char(SPC_SMALL);
    if ((*vendor == NULL) || (source != NETC)) {
      strncpy(&tmp[2], sourceName[source], 17);
    } else {
      strncpy(&tmp[2], vendor, 17);
    }
    
    // if changed copy to assigned string and shorten if too long
    u8g2.setFont(TOP_SMALL_FONT);
    strcopyext(&u8g2, r_top_left_small, tmp, 20, None, &updAreas, areaTopSmall, dispWidth - u8g2.getStrWidth(r_top_right_small) - 2);

    // if changed copy to assigned string and shorten if too long
    u8g2.setFont(TOP_BIG_FONT);
    strcopyext(&u8g2, r_top_left_big, tmp, 20, None, &updAreas, areaTopBig, dispWidth - u8g2.getStrWidth(r_top_right_big) - 2);
  }

    if ((updValues & (valVolume | valChannel | valMute)) > 0) {
      // No formats needed, just forward change flag to affected area
      updAreas |= areaVolume;
  }

  if ((updValues & (valTreble | valBass)) > 0) {
      // No formats needed, just forward change flag to affected area
      updAreas |= areaBassTreb;
  }

  if ((updValues & valArtist) > 0) {
    // if changed copy to assigned string and shorten if too long
    u8g2.setFont(ARTIST_FONT);
    strcopyext(&u8g2, r_artist, artist, TEXT_LENGTH, None, &updAreas, areaArtist, dispWidth);
  }

  if ((updValues & valTitle) > 0) {
    u8g2.setFont(TITLE_FONT);
    #if TITLE_ROWS == 1
    // if changed copy to assigned string and shorten if too long
    strcopyext(&u8g2, r_title_1, title, TEXT_LENGTH, None, &updAreas, areaTitle, dispWidth );  
    #elif TITLE_ROWS == 2
    // if changed copy to assigned string and split to two rows if too long
    strcopyext(&u8g2, r_title_1, title, TEXT_LENGTH, None, &updAreas, areaTitle, dispWidth, &r_title_2); 
    #endif
  }
  
  if ((updValues & valAlbum) > 0) {
    // if changed copy to assigned string and shorten if too long
    u8g2.setFont(ALBUM_FONT); 
    strcopyext(&u8g2, r_album, album, TEXT_LENGTH, None, &updAreas, areaAlbum, dispWidth);
  }

  if ((updValues & (valPlay | valRepeat | valShuffle | valElapsed)) > 0) {
    // update bottom row strings (right, center, left)
    char tmp[20] = "";
    char* cursor = tmp;

    if (playState != PLA_UNKNOWN) {
      // icon for shuffle mode if not switched off
      if (shuffleMode != SEQUENCE) {
        *cursor++ = char(shuffleMode);
        *cursor++ = char(SPC_SMALL);
      }
      // icon for repeat mode if not switched off
      if (repeatMode != REPEATNONE) {
        *cursor++ = char(repeatMode);
        *cursor++ = char(SPC_SMALL);
      }
      // icon for play state
      *cursor++ = char(playState);
      *cursor++ = char(SPC_SMALL);
    }
    // if there is complete elapsed information -> add elapsed time
    if ((timeElapsed != 0) && (timeTotal != 0)) {
      formatTime(cursor, timeElapsed);
    } else {
      *cursor = char(0);
    }
    strcopyext(&u8g2, r_bottom_left, tmp, 20, None, &updAreas, areaElapsed);


    // if there is elapsed information, but no total time 
    // -> show this centered without slider
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

    // if there is complete elapsed information -> add total time
    if ((timeElapsed != 0) && (timeTotal != 0)) {
      formatTime(tmp, timeTotal);
    } else {
      tmp[0] = char(0);
    }
    strcopyext(&u8g2, r_bottom_right, tmp, 20, None, &updAreas, areaElapsed);

    if ((updAreas & areaElapsed) > 0) {
      //some time data has changed -> recalc slider
      u8g2.setFont(BOTTOM_FONT);
      r_bottom_SliderX1 = u8g2.getStrWidth(r_bottom_left) + rSlider + 3;
      r_bottom_SliderX2 = dispWidth - u8g2.getStrWidth(r_bottom_right) - rSlider - 3;
      if ((timeElapsed != 0) && (timeTotal != 0)) {
        r_bottom_SliderW = (timeElapsed * (r_bottom_SliderX2 - r_bottom_SliderX1) / timeTotal);
      } else {
        // special marker to disable slider
        r_bottom_SliderW = 0xff;
      }
    }
  }
}

// check for changed values and if any,
// format and show the apropriate change screen
void showSettingScreen() {
  if ((updValues & valSource) > 0) {
    // initiate pop-up screen for source change
    setSettingSource();
    setSettingIcon(source);
  }

  if (r_setting != SRC) {
    // other setting screens are only shown, if
    // no source change is displayed at the moment
    
    if ((updValues & valVolume) > 0) {
      // initiate pop-up screen for volume change
      setSettingTitle(VOL, NULL, volume);
      setSettingScale(R_ON);
    }

    if ((updValues & valBass) > 0) {
      // initiate pop-up screen for bass change
      setSettingTitle(BAS, NULL, bass);
      setSettingScale(BASS);
    }

    if ((updValues & valTreble) > 0) {
      // initiate pop-up screen for treble change
      setSettingTitle(TRE, NULL, treble);
      setSettingScale(TREBLE);
    }

    if ((updValues & valRepeat) > 0) {
      // initiate pop-up screen for repeat mode change
      setSettingTitle(REP, repeatName[repeatMode - REPEATNONE]);
      setSettingIcon(repeatMode);
    }

    if ((updValues & valShuffle) > 0) {
      // initiate pop-up screen for shuffle mode change
      setSettingTitle(SHU, onoffName[shuffleMode - SEQUENCE]);
      setSettingIcon(shuffleMode);
    }
    
    if ((updValues & valMute) > 0) {
      // initiate pop-up screen for mute mode change
      setSettingTitle(MUT, onoffName[muteState - MUTE_OFF]);
      setSettingIcon(muteState);
    }

    if ((updValues & valVBass) > 0) {
      // initiate pop-up screen for virtual bass change
      setSettingTitle(VBS, onoffName[virtbassState-VBS_OFF]);
      setSettingIcon(virtbassState);
    }

    if ((updValues & valChannel) > 0) {
      // initiate pop-up screen for channel mode change
      setSettingTitle(CHN, channelName[channelState]);
      setSettingIcon(channelState==RIGHT?L_OFF:L_ON, channelState==LEFT?R_OFF:R_ON);
    }
  }
}

// draw all areas which are set in the rowLayout bit vector
void drawRow(uint16_t rowLayout) {
  if ((rowLayout & areaTopSmall ) > 0) 
  {
    //draw small top row + top divider line
    u8g2.setFont(TOP_SMALL_FONT);
    drawStrAligned(&u8g2, yTop_s, r_top_left_small, alnLeft);
    drawStrAligned(&u8g2, yTop_s, r_top_right_small, alnRight);
    u8g2.drawHLine(0, yLineTop_s, dispWidth);
  } else if ((rowLayout & areaTopBig ) > 0) 
  {
    //draw big top row + top divider line
    u8g2.setFont(TOP_BIG_FONT);
    drawStrAligned(&u8g2, yTop_b, r_top_left_big, alnLeft);
    drawStrAligned(&u8g2, yTop_b, r_top_right_big, alnRight);
    u8g2.drawHLine(0, yLineTop_b, dispWidth);
  }
  if ((rowLayout & areaArtist) > 0) 
  {
    //draw artist
    u8g2.setFont(ARTIST_FONT); 
    drawStrAligned(&u8g2, yArtist, r_artist, alnLeft);
  }
  if ((rowLayout & areaTitle) > 0) 
  {
    //draw title
    u8g2.setFont(TITLE_FONT); 
    alignment aln = alnLeft;
    if ((timeTotal == 0) && (*r_artist == char(0)) && (*r_album == char(0))) {
      //title contains radio station -> center
      aln = alnCenter;
    }
    if (*r_title_2 == char(0)) 
    {
      // one title tow -> vertical centered
      drawStrAligned(&u8g2, yTitle, r_title_1, aln);
    } else 
    {
      // two title rows
      drawStrAligned(&u8g2, yTitle_1, r_title_1, aln);
      drawStrAligned(&u8g2, yTitle_2, r_title_2, aln);
    }
  }
  if ((rowLayout & areaAlbum) > 0) 
  {
    //draw album
    u8g2.setFont(ALBUM_FONT);
    drawStrAligned(&u8g2, yAlbum, r_album);
  }
  if ((rowLayout & areaVolume) > 0) 
  {
    //draw volume
    u8g2.setFont(ICON_FONT);
    u8g2.drawGlyph(xVolume - ICON_WIDTH_B - 3, 31, getSpeakerIconLeft());
    drawScale(&u8g2, xVolume, 28, barWidth_b, stepWidth_b, 10, 0, (volume + 5) / 10 - 1, 1, 1);
    u8g2.drawGlyph(xVolume + stepWidth_b * 10 + 1, 31, getSpeakerIconRight());
  }
  if ((rowLayout & areaBassTreb) > 0) 
  {
    // draw bass 
    u8g2.setFont(ICON_FONT);
    u8g2.drawGlyph(xBass - ICON_WIDTH_S - 2, 46, BASS);
    if (bass < 0) {
      drawScale(&u8g2, xBass, 42, barWidth_s, stepWidth_s, 10, 5 + (bass - 1) / 2, 5, ((bass - 1) / 2) * 2 - 1, 2);
    } else {
      drawScale(&u8g2, xBass, 42, barWidth_s, stepWidth_s, 10, 5, 4 + (bass + 1) / 2, 2, 2);
    }
    // draw treble
    u8g2.drawGlyph(xTreble - ICON_WIDTH_S - 2, 46, TREBLE);
    if (treble < 0) {
      drawScale(&u8g2, xTreble, 42, barWidth_s, stepWidth_s, 10, 5 + (treble - 1) / 2, 5, ((treble - 1) / 2) * 2 - 1, 2);
    } else {
      drawScale(&u8g2, xTreble, 42, barWidth_s, stepWidth_s, 10, 5, 4 + (treble + 1) / 2, 2, 2);
    }
  }
  if ((rowLayout & areaSetTitle) > 0) 
  {
    // draw setting text
    u8g2.setFont(TOP_BIG_FONT);
    drawStrAligned(&u8g2, ySetTitle, r_setting_text, alnCenter);
  }
  if ((rowLayout & areaSetScale) > 0) 
  {
    // draw scale icon
    u8g2.setFont(ICON_FONT);
    drawStrAligned(&u8g2, ySetIcon, r_setting_icon, alnLeft);

    u8g2_int_t xScale = ICON_WIDTH_B + (dispWidth - stepWidth_b * 20 - ICON_WIDTH_B) / 2;
    switch (r_setting)
    {
      case VOL:
        // draw volume scale
        drawScale(&u8g2, xScale, ySetScale_l, barWidth_b, stepWidth_b, 20, 0, (volume + 2) / 5 - 1, 2, 2);
        break;
      case BAS:
        // draw bass scale
        if (bass < 0) {
          drawScale(&u8g2, xScale, ySetScale_h, barWidth_b, stepWidth_b, 20, 10 + bass, 9, bass * 2 - 1, 2);
        } else {
          drawScale(&u8g2, xScale, ySetScale_h, barWidth_b, stepWidth_b, 20, 10, 9 + bass, 2, 2);
        }
        break;
      case TRE:
        // draw treble scale
        if (treble < 0) {
          drawScale(&u8g2, xScale, ySetScale_h, barWidth_b, stepWidth_b, 20, 10 + treble, 9, treble * 2 - 1, 2);
        } else {
          drawScale(&u8g2, xScale, ySetScale_h, barWidth_b, stepWidth_b, 20, 10, 9 + treble, 2, 2);
        }
        break;
      case BAL:
        // not supported now
        break;
    }
  }
  if ((rowLayout & areaSetIcon) > 0) 
  {
    // draw setting icon
    u8g2.setFont(ICON_FONT);
    drawStrAligned(&u8g2, 34, r_setting_icon, alnCenter);
  }
  if ((rowLayout & areaBotLine) > 0) 
  {
    //draw bottom divider line
    u8g2.drawHLine(0, yLineBottom, dispWidth);
  }
  if ((rowLayout & areaElapsed) > 0) 
  {
    //draw bottom status row
    u8g2.setFont(BOTTOM_FONT);
    drawStrAligned(&u8g2, yBottom, r_bottom_left, alnLeft);
    if (r_bottom_SliderW != 0xff) {
      u8g2.drawRBox(r_bottom_SliderX1 - rSlider, ySlider, r_bottom_SliderW + 2 * rSlider, hSlider, rSlider);
    } else {
      drawStrAligned(&u8g2, yBottom, r_bottom_center, alnCenter);
    }
    drawStrAligned(&u8g2, yBottom, r_bottom_right, alnRight);
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

  // pre calc display layout positions 
  // according to choosen fonts and display size

  #if DISP_INTERFACE == NO_IF
      // simulate display geometry if no display connected
      dispWidth  = DISP_WIDTH;
      dispHeight = DISP_HEIGHT;
  #else
      // use real display geometry
      dispWidth  = u8g2.getDisplayWidth();
      dispHeight = u8g2.getDisplayHeight();
  #endif

  // top row starts at y = 0 (but u8g2 draws text at font character base line)
  u8g2.setFont(TOP_SMALL_FONT);
  yTop_s      = u8g2.getMaxCharHeight() + u8g2.getDescent(); // descent is a negative value!
  // divider 1 px below font
  yLineTop_s  = u8g2.getMaxCharHeight() + 1;

  // the same, but with other font (which changes u8g2 return values)
  u8g2.setFont(TOP_BIG_FONT);
  yTop_b      = u8g2.getMaxCharHeight() + u8g2.getDescent();
  yLineTop_b  = u8g2.getMaxCharHeight() + 1;

  // bottom font shows ascent part of characters + one pixel above and below
  u8g2.setFont(BOTTOM_FONT);
  yBottom     = dispHeight - 1;

  // bottom divider 1 px above
  yLineBottom = yBottom - u8g2.getAscent() - 3;

  // slider fits horizontal to ascent area of font
  hSlider     = u8g2.getAscent();
  ySlider     = dispHeight - hSlider - 1;

  // complete rounded left and right sides -> radius = 1/2 of height
  rSlider     = hSlider / 2;

  // artist 1 px below top divider
  u8g2.setFont(ARTIST_FONT);
  yArtist     = yLineTop_s + u8g2.getMaxCharHeight() + u8g2.getDescent() + 1;
  u8g2_int_t yArtist_b = yArtist - u8g2.getDescent();

  // album 1 px above bottom divider
  u8g2.setFont(ALBUM_FONT);
  yAlbum      = yLineBottom + u8g2.getDescent() - 1;
  u8g2_int_t yAlbum_t = yAlbum - u8g2.getMaxCharHeight() - u8g2.getDescent();

  // title (single row) centers between artist & album
  // title (two rows) go below artist with 1 px between rows
  u8g2.setFont(TITLE_FONT);
  yTitle      = yArtist_b + (yAlbum_t - yArtist_b - u8g2.getMaxCharHeight() ) / 2 + u8g2.getMaxCharHeight() + u8g2.getDescent();
  yTitle_1    = yArtist_b + 1 + u8g2.getMaxCharHeight() + u8g2.getDescent();
  yTitle_2    = yArtist_b + 2 + 2 * u8g2.getMaxCharHeight() + u8g2.getDescent();

  // setting title formats like big top row
  u8g2.setFont(TOP_BIG_FONT);
  ySetTitle   = yTop_b;

  // setting scale (h) with + / - areas centers vertical in main area
  ySetScale_h = yLineTop_b + (yLineBottom - yLineTop_b) / 2 - 2;

  // setting scale (l) with + area only 1 px above bottom divider
  ySetScale_l = yLineBottom - 2;

  // icon baseline aligned with scale (h)
  ySetIcon    = ySetScale_h;

  // set width of slider bars according to display width
  switch (dispWidth) {
    case 0 ... 63:    barWidth_s = 1; stepWidth_s = 1; barWidth_b = 1; stepWidth_b = 1; break;
    case 64 ... 83:   barWidth_s = 1; stepWidth_s = 1; barWidth_b = 1; stepWidth_b = 2; break;
    case 84 ... 123:  barWidth_s = 1; stepWidth_s = 2; barWidth_b = 2; stepWidth_b = 3; break;
    case 124 ... 223: barWidth_s = 2; stepWidth_s = 4; barWidth_b = 3; stepWidth_b = 5; break;
    default:          barWidth_s = 6; stepWidth_s = 8; barWidth_b = 9; stepWidth_b = 11;
  }

  // volume scale horizontal centered
  xVolume     = (dispWidth - 10 * stepWidth_b) / 2;

  // bass & treble scales (including the icon) center horizontal on left and right half of display
  xBass       = (dispWidth / 2 - ICON_WIDTH_S - 10 * stepWidth_s) / 2 + ICON_WIDTH_S;
  xTreble     = xBass + (dispWidth / 2);
}

void loop(void) {
  // this is a priorized state machine
  // it will process one step per loop cycle
  // the steps are sorted according their priority.
  // a step with lower priority is processed only
  // when all steps with higher priority are idle.
  //
  // this ensures that the UART receive buffer does 
  // never overflow!
  //
  // update the display through SPI or I2C is the most 
  // time consuming task and has therefore a very low
  // priority. 

  if (SERIAL_DAT.available() > 0) {
      // data from arylic UART is avaiable
      while (SERIAL_DAT.available() > 0) {
        if (fillBuffer(SERIAL_DAT.read())) {
          // complete line received -> parse line and reset buffer
          debugOut(inBuffer, '<');
          parseBuffer();
          resetBuffer();
          // as we have received the response of the last request from the board
          // we can stop waiting for sending next request
          nextBufferCmdTime = 0;
        }
      }
  } else if (sendBufferCmd()) {
      // action is done in sendBufferCmd:
      // send next request if any and waiting time is finished
  } else if ((nextELP > 0) && (millis() >= nextELP)) {  
      // We want an ELP update and waiting time has been reached
      addBufferCmd("ELP;");  //request ELP update
      if (playState == PLAY) nextELP += 1000; //next ELP request in 1s if no answer to this request
  } else if (updValues > 0) {
      // there were some changed values received
      // prepare related strings for drawing
      processValues();
      // iniate display of related settings pop-up screen
      showSettingScreen();
      // all updates processed
      updValues = 0;
  } else if (updAreas > 0) {
      // there are some display areas which need to be redrawn
      // propagate the update flags to all affected rows
      for (uint8_t row = 0; row < 8; row++) {
        if ((updAreas & layoutRows[currentLayout][row]) > 0) {
          // if one area of a row must be updated
          // update all areas which populate the row
          // as we don't have a persistant screen buffer
          rowLayout[row] |= layoutRows[currentLayout][row];
        }
      }
      // all update flags propagated to single rows -> clear general flag
      updAreas = 0;
      // start display update cycle
      currentRow = 0;
  } 
  else if (currentRow < DISP_PAGECOUNT) {
      // we need to update the current display row (and all subseeding)
      // when we reached the last row draw routine went idle
      if (currentRow == 0) debugOut("Start Screen update", '#');
      if((DISP_ROTATION==U8G2_R0) || (DISP_ROTATION==U8G2_R2)) {
          // selective optimized drawing of screen object works
          // for horizontal rows only!
          if (rowLayout[currentRow] > 0) {
            // reverse row position if 180 degrees rotation
            u8g2.setBufferCurrTileRow((DISP_ROTATION==U8G2_R0)?currentRow:DISP_PAGECOUNT-currentRow-1); 
            // clear page buffer and draw all contained areas
            u8g2.clearBuffer();
            drawRow(rowLayout[currentRow]);
            u8g2.sendBuffer();
          }
      } else {
          // allways draw all areas for each row (untested)
          u8g2.setBufferCurrTileRow(currentRow); 
          u8g2.clearBuffer();
          drawRow(0xffff);
          u8g2.sendBuffer();
      }
      // reset change flags of row
      rowLayout[currentRow] = 0;
      // update next row in next loop cycle
      currentRow++;
      if (currentRow == DISP_PAGECOUNT) debugOut("Finish Screen update", '#');
  } else if ((timeToLastLayout > 0) && (millis() >= timeToLastLayout)) {  
      // switch back to normal layout after pop up duration has finished
      setLayout(lastLayout);
      timeToLastLayout = 0; //done
      r_setting = NONE; 
  }
}
   
