
char codeNames[][15] = {"   On/Off   ", "   Vol Up   ", "   Vol Down   ", "  Up/Right  ", " Down/Left ", "  Select  ", "  Back/Mute  "};
long * codeList[] = {&powerCode, &volUpCode, &volDownCode, &chanUpCode, &chanDownCode, &selectCode, &returnCode};
int numCodes = 7;
int currentCode = 0;

void readCodes() {
  SdFile codeFile;
  codeFile.open("IRCodes.txt", FILE_READ);
  currentCode = 0;
  SerialUSB.print("Opening IRCodes.txt.. ");
  if (codeFile.available()) {
    SerialUSB.println("opened! ");
    while (codeFile.available()) {
      if (codeFile.read() == '0') {
        if (codeFile.read() == 'x') {
          char strBuf[20] = "            ";
          uint32_t tempInt = 0;
          codeFile.read(strBuf, 8);
          tempInt = strtoul(strBuf, NULL, 16);
          *codeList[currentCode] = tempInt;
          /*
            SerialUSB.print(codeNames[currentCode]);
            SerialUSB.print("read as: ");
            SerialUSB.print(strBuf);
            SerialUSB.print("strtoul output: ");
            SerialUSB.println(tempInt,HEX);
          */
          currentCode++;
          if (currentCode >= numCodes)codeFile.close();
        }
      }
    }
  } else {
    SerialUSB.println("error! ");
  }
  codeFile.close();
}

void writeCodes() {
  SdFile codeFile;
  codeFile.open("IRCodes.txt", FILE_WRITE | O_CREAT);
  if (codeFile.isOpen()) {
    codeFile.seekSet(0);
    currentCode = 0;
    while (currentCode < numCodes) {
      char strBuf[20];
      sprintf(strBuf, "0x%08X\r\n", *codeList[currentCode]);
      codeFile.print(strBuf);
      currentCode++;
    }
    codeFile.sync();
  }
  codeFile.close();
}

void programCodes() {
  display.clearScreen();
  delay(100);
  if (!sd.begin(10, SPI_FULL_SPEED)) {
    if (!sd.begin(10, SPI_FULL_SPEED)) {
      printCentered(48, 30, "Card error!");
      while (1);
    }
  }
  readCodes();
  currentCode = 0;
  display.setCursor(3, 8);
  display.print("<");
  display.setCursor(96 - 3 - 6, 8);
  display.print(">");
  display.setCursor(96 - 3 - display.getPrintWidth("Save+Exit >"), 48);
  display.print("Save+Exit >");

  display.setCursor(3, 48);
  display.print("< Set");

  char resultString[20] = "----------";
  decode_results results;
  long IRresult = 0;
  while (1) {
    if (checkNewButtonPress(TSButtonUpperRight)) {
      currentCode++;
      if (currentCode >= numCodes) {
        currentCode = 0;
      }
      IRresult = 0;
    }
    if (checkNewButtonPress(TSButtonUpperLeft)) {
      currentCode--;
      if (currentCode < 0) {
        currentCode = numCodes - 1;
      }
      IRresult = 0;
    }
    if (checkNewButtonPress(TSButtonLowerLeft)) {
      if (IRresult) {
        *codeList[currentCode] = IRresult;
      }
    }
    if (checkNewButtonPress(TSButtonLowerRight)) {
      writeCodes();
      return;
    }
    if (irrecv.decode(&results)) {
      irrecv.resume(); // Receive the next value
      if (results.value != 0xFFFFFFFF) {
        IRresult = results.value;
      }
    }

    char currentCodeString[20] = "0x00000000";
    sprintf(currentCodeString, "0x%08X", *codeList[currentCode]);
    if (IRresult) {
      sprintf(resultString, "0x%08X", IRresult);
    } else {
      sprintf(resultString, "----------");
    }
    printCentered(48, 8, codeNames[currentCode]);
    display.setCursor(3, 22);
    display.print("Set: ");
    display.print(currentCodeString);
    display.setCursor(3, 32);
    display.print("New: ");
    display.print(resultString);
  }
}




char settingNames[][15] = {" TinyTV Mode ", "  Auto Play  ", "  Show Time  ", " Loop Video ", "  Full Vol  ", "  Chan Dwn  "};
long * settingList[] = {&tinyTVmode, &autoPlay, &showTimeBar, &loopVideo, &fullVolume, &chanDwn};
int numSettings = 6;
int currentSetting = 0;

void readSettings() {
  SdFile settingsFile;
  settingsFile.open("Settings.txt", FILE_READ);
  currentSetting = 0;
  SerialUSB.print("Opening Settings.txt.. ");
  if (settingsFile.available()) {
    SerialUSB.println("opened! ");
    while (settingsFile.available()) {
      if (settingsFile.read() == '0') {
        if (settingsFile.read() == 'x') {
          char strBuf[20] = "            ";
          uint32_t tempInt = 0;
          settingsFile.read(strBuf, 8);
          tempInt = strtoul(strBuf, NULL, 16);
          *settingList[currentSetting] = tempInt;
          currentSetting++;
          if (currentSetting >= numSettings)settingsFile.close();
        }
      }
    }
  } else {
    SerialUSB.println("error! ");
  }
  settingsFile.close();
}

void writeSettings() {
  SdFile settingsFile;
  settingsFile.open("Settings.txt", FILE_WRITE | O_CREAT);
  if (settingsFile.isOpen()) {
    settingsFile.seekSet(0);
    currentSetting = 0;
    while (currentSetting < numSettings) {
      char strBuf[20];
      sprintf(strBuf, "0x%08X\r\n", *settingList[currentSetting]);
      settingsFile.print(strBuf);
      currentSetting++;
    }
    settingsFile.sync();
  }
  settingsFile.close();
}

void changeSettings() {
  display.clearScreen();
  delay(100);
  if (!sd.begin(10, SPI_FULL_SPEED)) {
    if (!sd.begin(10, SPI_FULL_SPEED)) {
      printCentered(48, 30, "Card error!");
      while (1);
    }
  }
  readSettings();
  currentSetting = 0;
  display.setCursor(3, 8);
  display.print("<");
  display.setCursor(96 - 3 - 6, 8);
  display.print(">");
  int xOffset = 96 - 3 - display.getPrintWidth("Exit >");
  display.setCursor(xOffset, 48 - 10);
  display.print("Save+");
  display.setCursor(xOffset, 48);
  display.print("Exit >");

  display.setCursor(3, 48);
  display.print("< Toggle");

  while (1) {
    if (checkNewButtonPress(TSButtonUpperRight)) {
      currentSetting++;
      if (currentSetting >= numSettings) {
        currentSetting = 0;
      }
    }
    if (checkNewButtonPress(TSButtonUpperLeft)) {
      currentSetting--;
      if (currentSetting < 0) {
        currentSetting = numSettings - 1;
      }
    }
    if (checkNewButtonPress(TSButtonLowerLeft)) {
      if (*settingList[currentSetting]) {
        *settingList[currentSetting] = 0;
      } else {
        *settingList[currentSetting] = 1;
      }
    }
    if (checkNewButtonPress(TSButtonLowerRight)) {
      writeSettings();
      return;
    }

    printCentered(48, 8, settingNames[currentSetting]);
    display.setCursor(3, 22 + 4);
    //display.print("Value: ");
    if (*settingList[currentSetting]) {
      display.print("ON  ");
    } else {
      display.print("OFF  ");
    }

  }
}
