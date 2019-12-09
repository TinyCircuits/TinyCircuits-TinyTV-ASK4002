void showPause() {
  int top = 20;
  int bottom = 40;
  int left = (96 / 2) - 10;
  int right = left + 8;
  for (int y = top; y < bottom; y++) {
    for (int x = left; x < right; x++) {
      byte color = 0xFF;
      if (y == top || y == bottom - 1)color = 0;
      if (x == left || x == right - 1)color = 0;
      buffer[(((y * 96) + x) * 2)] = color;
      buffer[(((y * 96) + x) * 2) + 1] = color;
    }
  }
  left = (96 / 2) + 2;
  right = left + 8;
  for (int y = top; y < bottom; y++) {
    for (int x = left; x < right; x++) {
      byte color = 0xFF;
      if (y == top || y == bottom - 1)color = 0;
      if (x == left || x == right - 1)color = 0;
      buffer[(((y * 96) + x) * 2)] = color;
      buffer[(((y * 96) + x) * 2) + 1] = color;
    }
  }
}

void showPlay() {
  int top = 20;
  int bottom = 40;
  int left = (96 / 2) - 3;
  int right = left;
  for (int y = top; y < bottom; y++) {
    if (y < top + ((bottom - top) / 2)) {
      right++;
    } else {
      right--;
    }
    for (int x = left; x < right; x++) {
      byte color = 0xFF;
      if (y == top || y == bottom - 1)color = 0;
      if (x == left || x == right - 1)color = 0;
      buffer[(((y * 96) + x) * 2)] = color;
      buffer[(((y * 96) + x) * 2) + 1] = color;
    }
  }
}

void showPlayPositionBar() {
  currentFrame = (long)(vidFile.curPosition() / ((96 * 64 * 2) + (1024 * 2)));
  currentSeconds = currentFrame / 30;
  displayMinutes = currentSeconds / 60;
  displaySeconds = currentSeconds % 60;
  char currentTimeString[] = "00:00";
  currentTimeString[0] = displayMinutes / 10 + '0';
  currentTimeString[1] = displayMinutes % 10 + '0';
  currentTimeString[3] = displaySeconds / 10 + '0';
  currentTimeString[4] = displaySeconds % 10 + '0';
  int timeStringWidth = display.getPrintWidth(totalTimeString);
  if (currentBarVisible < 10)currentBarVisible++;
  if (currentBarVisible > showBar)currentBarVisible = showBar;
  int yOffset = 63 - currentBarVisible;
  //if (yOffset < 53)yOffset = 53;
  for (int y = yOffset + 0; y < yOffset + 12 && y < 64; y++) {
    for (int x = 0; x < 96; x++) {
      ((uint16_t *)buffer)[(y * 96) + x] = (((uint16_t *)buffer)[(y * 96) + x] >> 2) & 0xE739; //0xEF7B
    }
  }
  for (int y = yOffset; y < yOffset + 15; y++) {
    if (y < 64)
      putString(y, 1, yOffset + 1, currentTimeString, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
  for (int y = yOffset; y < yOffset + 15; y++) {
    if (y < 64)
      putString(y, 95 - timeStringWidth, yOffset + 1, totalTimeString, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
  int barTotal = 93 - timeStringWidth - (timeStringWidth + 2);
  int progressBar = (long)((currentFrame * barTotal) / videoFrames);
  //SerialUSB.print(currentFrame);SerialUSB.print(" ");SerialUSB.print(videoFrames);SerialUSB.print(" ");SerialUSB.print(barTotal);SerialUSB.print(" ");SerialUSB.println(progressBar);
  for (int y = yOffset + 4; y < yOffset + 8 && y < 64; y++) {
    for (int x = timeStringWidth + 2; x < 93 - timeStringWidth; x++) {
      if (x - timeStringWidth - 2 <= progressBar) {
        ((uint16_t *)buffer)[(y * 96) + x] =  0xEF7B;
      } else {
        ((uint16_t *)buffer)[(y * 96) + x] =  0xE739;
      }
    }
  }
  if (!paused) {
    showBar--;
  }
}

void showStaticEffect() {
  showStatic--;
  uint16_t staticPos = 0;
  while (staticPos < 96 * 64) {
    uint8_t currentRand = SysTick->VAL;
    uint8_t currentRandSmall = ((currentRand >> 6 - ((showStaticMax - showStatic)) / 2)) & 3;
    if (currentRandSmall == 3) {
      ((uint16_t *)buffer)[staticPos] = colorAndMask1;//black
    } else if (currentRandSmall == 2) {
      ((uint16_t *)buffer)[staticPos] = colorOrMask4;//white
    } else if (currentRandSmall == 1) {
      ((uint16_t *)buffer)[staticPos] = colorOrMask1;//black/grey
      //((uint16_t *)buffer)[staticPos] = colorAndMask4;//grey
      //((uint16_t *)buffer)[staticPos] = colorAndMask1;black
    }
    staticPos += (currentRand & 3);
  }
}


void showVolumeBar() {
  char volumeString[] = "|------|";
  volumeString[1 + volume] = '+';
  int yOffset = 50;
  int xWidth = display.getPrintWidth(volumeString);
  int xPosition = 48 - (xWidth / 2);

  for (int y = yOffset + 0; y < yOffset + 12 && y < 64; y++) {
    for (int x = xPosition - 4; x < xPosition + xWidth + 3; x++) {
      ((uint16_t *)buffer)[(y * 96) + x] = (((uint16_t *)buffer)[(y * 96) + x] >> 2) & 0xE739; //0xEF7B
    }
  }
  for (int y = yOffset; y < yOffset + 15; y++) {
    if (y < 64)
      putString(y, xPosition, yOffset + 1, volumeString, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
  showVolume--;
}

void showChannelNumber() {
  char channelString[] = "0";
  //if(currentFileNum>9){
  //   channelString[0]+= currentFileNum/10;
  //}
  channelString[0] += (currentFileNum % 10);
  int yOffset = 50;
  int xPosition = 80;
  int xWidth = display.getPrintWidth(channelString);

  for (int y = yOffset + 0; y < yOffset + 12 && y < 64; y++) {
    for (int x = xPosition - 4; x < xPosition + xWidth + 3; x++) {
      ((uint16_t *)buffer)[(y * 96) + x] = (((uint16_t *)buffer)[(y * 96) + x] >> 2) & 0xE739; //0xEF7B
    }
  }
  for (int y = yOffset; y < yOffset + 15; y++) {
    if (y < 64)
      putString(y, xPosition, yOffset + 1, channelString, buffer + (96 * 2 * y), thinPixel7_10ptFontInfo);
  }
  showChannel--;
}

void tinyTVtubeOff() {
  delay(5);
  if (pauseRadius) {
    int xCircle = pauseRadius / 2;
    int yCircle = 0;
    int radiusError = 1 - xCircle;
    int radiusLimits[64];
    memset(radiusLimits, 0, sizeof(radiusLimits));
    while (xCircle >= yCircle) {
      radiusLimits[32 + yCircle] = xCircle * 3 / 2;
      radiusLimits[32 - yCircle] = xCircle * 3 / 2;
      radiusLimits[32 - xCircle] = yCircle * 3 / 2;
      radiusLimits[32 + xCircle] = yCircle * 3 / 2;
      yCircle++;
      if (radiusError < 0)
      {
        radiusError += 2 * yCircle + 1;
      } else {
        xCircle--;
        radiusError += 2 * (yCircle - xCircle) + 1;
      }
    }
    for (int y = 0; y < 64; y++) {
      for (int x = 0; x < 48 - radiusLimits[y]; x++) {
        ((uint16_t *)buffer)[(y * 96) + x] = (((uint16_t *)buffer)[(y * 96) + x] >> 1) & 0xE739;
      }
      for (int x = 48 + radiusLimits[y]; x < 96; x++) {
        ((uint16_t *)buffer)[(y * 96) + x] = (((uint16_t *)buffer)[(y * 96) + x] >> 1) & 0xE739;
      }
    }
    for (int y = 0; y < 64; y++) {
      if (radiusLimits[y]) {
        for (int x = 48 - radiusLimits[y] - 3; x < 48 + radiusLimits[y] + 3; x) {
          uint8_t currentRand = SysTick->VAL;
          uint8_t currentRandSmall = ((currentRand >> 4) & 3);
          if (x == 48 - radiusLimits[y] - 3)
            x += (currentRand & 3);
          if (currentRandSmall == 3) {
            ((uint16_t *)buffer)[(y * 96) + x] = colorAndMask1;//black
          } else if (currentRandSmall == 2) {
            ((uint16_t *)buffer)[(y * 96) + x] = colorOrMask4;//white
          } else if (currentRandSmall == 1) {
            ((uint16_t *)buffer)[(y * 96) + x] = colorOrMask1;//black/grey
          }
          x += (currentRand & 3) * 2;
        }
      }
    }

    pauseRadius -= pauseRadius / 12;
    if (pauseRadius < 12) {
      pauseRadius--;

      if (pauseRadius < 6 && pauseRadius > 3 ) {
        for (int y = 31; y < 33; y++) {
          for (int x = 48 - 30; x < 48 + 30; x++) {
            uint8_t currentRand = SysTick->VAL;
            uint8_t currentRandSmall = ((currentRand >> 4) & 3);
            if (x == 48 - radiusLimits[y] - 3)
              x += (currentRand & 3);
            if (currentRandSmall == 3) {
              ((uint16_t *)buffer)[(y * 96) + x] = colorAndMask1;//black
            } else if (currentRandSmall == 2) {
              ((uint16_t *)buffer)[(y * 96) + x] = colorOrMask4;//white
            } else if (currentRandSmall == 1) {
              ((uint16_t *)buffer)[(y * 96) + x] = colorOrMask1;//black/grey
            }
            x += (currentRand & 3) * 2;
          }
        }
      }
      if (pauseRadius < 6) {
        delay((6 - pauseRadius) * 15);
      }
    }
    if (pauseRadius <= 0) {
      pauseRadius = 0;
      memset(buffer, 0, sizeof(buffer));
    }
  }
}
