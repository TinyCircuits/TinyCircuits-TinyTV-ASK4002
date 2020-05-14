//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Video Player
//
//  Changelog:
//  1.2.0 27 Nov 2019 - Add TinyTV mode with CRT effects, double buffered audio
//  1.1.0 15 Jul 2019 - Change to specifically support TinyScreen+ Video Player
//        Kit, adding IR reciever functionality and settings menu
//  1.0.3 15 Nov 2018 - Add option for TinyScreen+ button input
//  1.0.2 10 April 2017 - Cleanup, add messages about SD card/video detection
//  1.0.1 3 October 2016 - Add compatibility for new Arcade revision, change
//        folder structure for IDE compatibility
//  1.0.0 Initial release
//
//  Written by Ben Rose for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

#include <TinyScreen.h>
#include "SdFat.h"
#include <Wire.h>
#include <SPI.h>
#include "src/IRremote/IRremote.h"

// Default settings (Can be edited here or under the Settings menu on the TinyTV)
// To open TinyTV Settings: Turn TV off, then press and hold upper-right button
// while turning TV on. This should open the Settings & IR menu
long autoPlay = 1;
long showTimeBar = 1;
long loopVideo = 0;
long fullVolume = 1;
long tinyTVmode = 0;

// IR NEC button codes (Can be edited here or under the IR menu on the TinyTV)
// The IR menu is found with the Settings menu. To toggle buttons for different
// functionality, open the IR menu, navigate to the desired button, press
// the button on the new remote, and save the NEC code.
long powerCode = 0x10AF8877;
long volUpCode = 0x10AF708F;
long volDownCode = 0x10AFB04F;
long chanUpCode = 0x10AF807F;
long chanDownCode = 0x10AF20DF;
long selectCode = 0x10AF00FF;
long returnCode = 0x10AFF00F;

int RECV_PIN = 3;

IRrecv irrecv(RECV_PIN);

TinyScreen display = TinyScreen(TinyScreenPlus);

SdFat sd;
SdFile dir;
SdFile vidFile;
uint8_t buffer[96 * 64 * 2];
uint16_t audioBuffer1[1024 * 2];
uint16_t audioBuffer2[1024 * 2];
int currentAudioBuffer = 0;
volatile int lastFilledAudioBuffer = 0;
volatile uint32_t sampleIndex = 0;

int currentFileNum = 0;
int doVideo = 0;
int foundValidVideo = 0;
char currentVideoName[50] = "No video found";

int printNicelyOffset = 0;
int printNicelyDir = 0;
unsigned long lastPrintNicelyOffsetChange = 0;
unsigned long printNicelyOffsetChangeInterval = 200;

volatile int volume = 5;
int maxVolume = 5;
int oldVolume = 5;

int isOff = false;


void setup(void) {
  analogWrite(A0, analogRead(A0));//Set audio output to avoid picking up noise, even if audio isn't used
  tcConfigure(30720);
  display.begin();
  display.setBitDepth(1);
  display.setFlip(true);
  display.setFont(thinPixel7_10ptFontInfo);
  display.initDMA();
  irrecv.enableIRIn(); // Start the receiver
  pinMode(RECV_PIN, INPUT_PULLUP);
  SPI.begin();
  delay(50);
  if (!sd.begin(10, SPI_FULL_SPEED)) {
    cardNotFound();
  }
  SPI.setClockDivider(0);
  if (display.getButtons(TSButtonUpperRight)) {
    delay(10);
    if (display.getButtons(TSButtonUpperRight)) {
      settingsMainMenu();
    }
  }
  readCodes();
  readSettings();
  if (!fullVolume) {
    volume = 3;
  }
  sd.vwd()->rewind();
  getNextFile();
  if (autoPlay || tinyTVmode) {
    startNewVideo();
  }
}

void loop() {
  if (tinyTVmode) {
    tinyTVloop();
  } else {
    videoPlayerLoop();
  }
}

void videoPlayerLoop() {

  decode_results results;

  long IRresult = 0;
  if (irrecv.decode(&results)) {
    irrecv.resume(); // Receive the next value
    //SerialUSB.print(results.decode_type);
    //SerialUSB.print(" ");
    //SerialUSB.print(results.bits);
    //SerialUSB.print(" ");
    //SerialUSB.println(results.value, HEX);
    IRresult = results.value;
  }
  if (!isOff && IRresult == volUpCode) {
    if (!isPaused())increaseVolumeRequest() ;
  }
  if (!isOff && IRresult == volDownCode) {
    if (!isPaused())decreaseVolumeRequest() ;
  }
  if (!isOff && (IRresult == chanUpCode || checkNewButtonPress(TSButtonLowerRight))) {
    if (isStopped() ) {
      //stopVideo();
      getNextFile();
      //startNewVideo();
    } else {
      seekForward();
    }
  }
  if (!isOff && IRresult == chanDownCode || checkNewButtonPress(TSButtonLowerLeft)) {
    if (isStopped() ) {
      //stopVideo();
      getPreviousFile();
      //startNewVideo();
    } else {
      seekBackward();
    }
  }
  if (!isOff && IRresult == selectCode || checkNewButtonPress(TSButtonUpperRight)) {
    if (isStopped()) {
      startNewVideo();
    } else {
      if (isPaused()) {
        requestUnPause();
      } else {
        requestPause();
        while (!isPaused())
          playVideo();
      }
    }
  }
  if (!isOff && IRresult == returnCode || checkNewButtonPress(TSButtonUpperLeft)) {
    if (!isStopped()) {
      stopVideo();
    }
  }
  if (IRresult == powerCode || checkNewButtonPress(TSButtonLowerLeft)) {
    if (!isOff) {
      if (!isStopped()) {
        requestPause();
        delay(10);
        display.endTransfer();
      }
      display.off();
      isOff = true;
    } else {
      display.on();
      if (!isStopped()) {
        display.goTo(0, 0);
        display.startData();
        requestUnPause();
      }
      isOff = false;
    }
  }
  if (doVideo  && !isOff && !isStopped()) {
    playVideo();
  }
  if (isStopped() && !isOff) {
    bufferVideoFrame();
    printNicely(currentVideoName);
    writeToDisplay();
  }
}


void tinyTVloop() {
  decode_results results;

  long IRresult = 0;
  if (irrecv.decode(&results)) {
    irrecv.resume(); // Receive the next value
    //SerialUSB.print(results.decode_type);
    //SerialUSB.print(" ");
    //SerialUSB.print(results.bits);
    //SerialUSB.print(" ");
    //SerialUSB.println(results.value, HEX);
    IRresult = results.value;
  }

  if (IRresult == volUpCode || checkNewButtonPress(TSButtonUpperRight)) {
    if (!isPaused())increaseVolumeRequest() ;
  }
  if (IRresult == volDownCode || checkNewButtonPress(TSButtonLowerRight)) {
    if (!isPaused())decreaseVolumeRequest() ;
  }
  if (IRresult == returnCode) {
    if (!isPaused())toggleMute();
  }
  if ((IRresult == chanUpCode || checkNewButtonPress(TSButtonUpperLeft)) && !isPaused()) {
    stopVideo();
    getNextFile();
    startNewVideo();
  }
  if (IRresult == chanDownCode && !isPaused()) {
    stopVideo();
    getPreviousFile();
    startNewVideo();
  }
  if (IRresult == powerCode || checkNewButtonPress(TSButtonLowerLeft)) {
    if (isPaused()) {
      display.on();
      display.goTo(0, 0);
      display.startData();
      requestUnPause();
    } else {
      requestPause();
      while (!isPaused())
        playVideo();
      delay(10);
      display.endTransfer();
      display.off();
    }
  }
  if (doVideo && !isPaused()) {
    playVideo();
  }
}

void bufferVideoFrame() {
  if (!vidFile.available())
    vidFile.rewind();
  vidFile.read(buffer, 96 * 64 * 2);
  vidFile.read((uint8_t*)audioBuffer1, 1024 * 2);
}

void writeToDisplay() {
  display.goTo(0, 0);
  display.startData();
  display.writeBuffer(buffer, 96 * 64 * 2);
  display.endTransfer();
}

void printCentered(int x, int y, char * stringToPrint) {
  int xPosition = x - ((int)display.getPrintWidth(stringToPrint) / 2);
  display.setCursor(xPosition, y);
  display.print(stringToPrint);
}

void displayCardNotFoundOptions() {
  display.clearScreen();
  display.setCursor(3, 8);
  display.print("< Settings ");
  display.setCursor(96 - 3 - display.getPrintWidth("IR >"), 8);
  display.print("IR >");
  display.setCursor(96 - 3 - display.getPrintWidth("Exit >"), 48);
  display.print("Exit >");
}

void settingsMainMenu() {
  displayCardNotFoundOptions();
  while (1) {
    if (checkNewButtonPress(TSButtonUpperRight)) {
      programCodes();
      displayCardNotFoundOptions();
    }
    if (checkNewButtonPress(TSButtonUpperLeft)) {
      changeSettings();
      displayCardNotFoundOptions();
    }
    if (checkNewButtonPress(TSButtonLowerRight)) {
      return;
    }
  }
}

void cardNotFound() {
  display.clearScreen();
  printCentered(48, 8, "Card not found!");
  printCentered(48, 25, "Insert card to");
  printCentered(48, 36, "change settings or");
  printCentered(48, 47, "program IR codes");
  unsigned long lastInit = millis();
  unsigned long initTimeout = 500;
  while (1) {
    if (millis() - lastInit > initTimeout) {
      lastInit = millis();
      if (sd.begin(10, SPI_FULL_SPEED)) {
        settingsMainMenu();
      }
    }
    decode_results results;
    long IRresult = 0;
    if (irrecv.decode(&results)) {
      IRresult = results.value;
      irrecv.resume(); // Receive the next value
      if (IRresult != 0xFFFFFFFF) {
        char resultString[20];
        sprintf(resultString, "0x%08X", IRresult);
        printCentered(44, 55, resultString);
      }
    }
  }
}

void getNextFile() {
  doVideo = 0;
  vidFile.close();
  int foundVideo = 0;
  while (!foundVideo) {
    dir.close();
    if (dir.openNext(sd.vwd(), O_READ)) {
      currentFileNum++;
    } else if (!foundValidVideo) {
      //This should mean there are no valid videos or SD card problem
      int xPosition = 48 - ((int)display.getPrintWidth("No video files!") / 2);
      display.setCursor(xPosition, 26 - 10);
      display.print("No video files!");
      xPosition = 48 - ((int)display.getPrintWidth("Place .TSV files") / 2);
      display.setCursor(xPosition, 26);
      display.print("Place .TSV files");
      xPosition = 48 - ((int)display.getPrintWidth("in main directory.") / 2);
      display.setCursor(xPosition, 26 + 10);
      display.print("in main directory.");
      while (1);
    } else {
      sd.vwd()->rewind();
      dir.openNext(sd.vwd(), O_READ);
      currentFileNum = 1;
    }
    if (dir.isFile()) {
      char fileName[100];
      memset(fileName, 0, 100);
      dir.getName(fileName, 50);
      if (!strcmp(fileName + strlen(fileName) - 4, ".tsv")) {
        dir.close();
        if (vidFile.open(fileName, O_READ)) {
          doVideo = 1;
          foundValidVideo = 1;
          foundVideo = 1;
        }
      }
    }
  }
  vidFile.getName(currentVideoName, 50);
  currentVideoName[strlen(currentVideoName) - 4] = '\0';
  printNicelyOffset = 0;
  printNicelyDir = 0;
  lastPrintNicelyOffsetChange = millis();
}

void getPreviousFile() {
  doVideo = 0;
  vidFile.close();
  int foundVideo = 0;
  while (!foundVideo) {
    if (currentFileNum > 1) {
      currentFileNum--;
    } else {
      currentFileNum = 100;
      int i;
      for (i = 1; i < currentFileNum; i++) {
        dir.close();
        if (!dir.openNext(sd.vwd(), O_READ))break;
      }
      currentFileNum = i;
    }
    sd.vwd()->rewind();
    int i;
    for (i = 1; i <= currentFileNum; i++) {
      dir.close();
      if (!dir.openNext(sd.vwd(), O_READ))break;
    }
    if (dir.isFile()) {
      char fileName[100];
      memset(fileName, 0, 100);
      dir.getName(fileName, 50);
      if (!strcmp(fileName + strlen(fileName) - 4, ".tsv")) {
        dir.close();
        if (vidFile.open(fileName, O_READ)) {
          doVideo = 1;
          foundVideo = 1;
        }
      }
    }
  }
  vidFile.getName(currentVideoName, 50);
  currentVideoName[strlen(currentVideoName) - 4] = '\0';
  printNicelyOffset = 0;
  printNicelyDir = 0;
  lastPrintNicelyOffsetChange = millis();
}

void printNicely(char * name) {
  int yOffset = 34;
  for (int y = yOffset + 0; y < yOffset + 12; y++) {
    for (int x = 0; x < 96; x++) {
      ((uint16_t *)buffer)[(y * 96) + x] = (((uint16_t *)buffer)[(y * 96) + x] >> 2) & 0xE739; //0xEF7B
    }
  }
  char displayName[50];
  strcpy(displayName, name + printNicelyOffset);

  int xPosition = 48 - ((int)display.getPrintWidth(displayName) / 2);

  if (millis() > lastPrintNicelyOffsetChange + printNicelyOffsetChangeInterval) {
    lastPrintNicelyOffsetChange = millis();
    if (xPosition < 9) {
      printNicelyOffset++;
    } else {
      printNicelyOffset = 0;
    }
  }

  while (xPosition < 9) {
    displayName[strlen(displayName) - 1] = '\0';
    xPosition = 48 - ((int)display.getPrintWidth(displayName) / 2);
  }

  for (int y = yOffset + 1; y < yOffset + 15; y++) {
    putString(y, xPosition, yOffset + 1, displayName, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
  char chbuf[] = "<";
  for (int y = yOffset + 1; y < yOffset + 15; y++) {
    putString(y, 1, yOffset + 1, chbuf, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
  chbuf[0] = '>';
  for (int y = yOffset + 1; y < yOffset + 15; y++) {
    putString(y, 87 + 5, yOffset + 1, chbuf, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
}

void putString(int y, int fontX, int fontY, char * string, uint8_t * buff, const FONT_INFO & fontInfo) {
  const FONT_CHAR_INFO* fontDescriptor = fontInfo.charDesc;
  int fontHeight = fontInfo.height;
  if (y >= fontY && y < fontY + fontHeight) {
    const unsigned char* fontBitmap = fontInfo.bitmap;
    int fontFirstCh = fontInfo.startCh;
    int fontLastCh = fontInfo.endCh;
    //if(!_fontFirstCh)return 1;
    //if(ch<_fontFirstCh || ch>_fontLastCh)return 1;
    //if(_cursorX>xMax || _cursorY>yMax)return 1;
    int stringChar = 0;
    int ch = string[stringChar++];
    while (ch) {
      uint8_t chWidth = pgm_read_byte(&fontDescriptor[ch - fontFirstCh].width);
      int bytesPerRow = chWidth / 8;
      if (chWidth > bytesPerRow * 8)
        bytesPerRow++;
      unsigned int offset = pgm_read_word(&fontDescriptor[ch - fontFirstCh].offset) + (bytesPerRow * fontHeight) - 1;

      for (uint8_t byte = 0; byte < bytesPerRow; byte++) {
        uint8_t data = pgm_read_byte(fontBitmap + offset - (y - fontY) - ((bytesPerRow - byte - 1) * fontHeight));
        uint8_t bits = byte * 8;
        for (int i = 0; i < 8 && (bits + i) < chWidth; i++) {
          if (data & (0x80 >> i)) {
            buff[(fontX) * 2] = 0xFFFF >> 8;
            buff[(fontX) * 2 + 1] = 0xFFFF;
            // setPixelInBuff(y,16+fontX,0);
            //lineBuffer[16+fontX]=0;
          } else {
            //SPDR=_fontBGcolor;
          }
          fontX++;
        }
      }
      fontX += 1;
      ch = string[stringChar++];
    }
  }
}

uint8_t buttonState = TSButtonUpperLeft | TSButtonUpperRight | TSButtonLowerLeft | TSButtonLowerRight;

bool checkNewButtonPress(uint8_t mask) {
  uint8_t currentButtonState = display.getButtons(mask);
  bool returnval = false;
  if (((mask & buttonState) == 0) && ((mask & currentButtonState) == mask)) {
    delay(10);
    currentButtonState = display.getButtons(mask);
    if (((mask & buttonState) == 0) && ((mask & currentButtonState) == mask)) {
      returnval = true;
    }
  }
  buttonState &= (~mask);
  buttonState |= currentButtonState;
  return returnval;
}

int showPlaying = 0;
