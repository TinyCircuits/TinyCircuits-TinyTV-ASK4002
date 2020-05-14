
int paused = false;
int stopped = true;
int videoFrames = 0;
int videoSeconds = 0;
int currentFrame = 0;
int currentSeconds = 0;
int displayMinutes = 0;
int displaySeconds = 0;
char totalTimeString[] = "00:00";
int currentBarVisible = 0;
int showBar = 0;
int getSeekFrame = false;

const uint16_t colorOrMask1 = 0x1084;
const uint16_t colorOrMask2 = 0x18C6;
const uint16_t colorOrMask3 = 0x1CE7;
const uint16_t colorOrMask4 = 0x9EF7;

const uint16_t colorAndMask1 = 0x6108;
const uint16_t colorAndMask2 = 0xE318;
const uint16_t colorAndMask3 = 0xE739;
const uint16_t colorAndMask4 = 0xEF7B;

//int paused = false;
int showVolume = 0;
int showChannel = 0;
int showStatic = 0;
int showStaticMax = 12;
int pauseRadius = 0;


#include "videoPlayerEffects.h"


bool isPaused() {
  if (paused && (pauseRadius == 0))
    return true;
  return false;
}

bool isStopped() {
  if (stopped)
    return true;
  return false;
}

void requestPause() {
  if (!stopped && !paused) {
    tcDisable();
    paused = true;
    if (tinyTVmode) {
      pauseRadius = 30 * 2;
    }
    showVolume = 0;
    showChannel = 0;
    //showStatic = showStaticMax/2;
  }
}

void requestUnPause() {
  if (!stopped && paused) {
    tcStartCounter();
    paused = false;
    if (tinyTVmode) {
      showStatic = showStaticMax;
    }
  }
}
void increaseVolumeRequest() {
  if (tinyTVmode) {
    showVolume = 50;
  }
  if (volume < maxVolume)
    volume++;
  oldVolume = volume;
}

void decreaseVolumeRequest() {
  if (tinyTVmode) {
    showVolume = 50;
  }
  if (volume > 0)
    volume--;
  oldVolume = volume;
}

void toggleMute() {
  if (volume) {
    oldVolume = volume;
    volume = 0;
  } else {
    volume = oldVolume;
  }
  if (tinyTVmode) {
    showVolume = 50;
  }
}

int startNewVideo() {
  display.goTo(0, 0);
  display.startData();

  vidFile.rewind();
  long videoSize = vidFile.fileSize();
  videoFrames = (long)(videoSize / ((96 * 64 * 2) + (1024 * 2)));
  videoSeconds = videoFrames / 30;

  if (tinyTVmode) {
    long seekToo = millis() * 30 / 1000 % videoFrames;
    vidFile.seekSet((seekToo * ((96 * 64 * 2) + (1024 * 2))));
  }

  memset((uint8_t *)audioBuffer1, 0, 1024 * 2);
  memset((uint8_t *)audioBuffer2, 0, 1024 * 2);
  tcStartCounter();

  if (!tinyTVmode) {
    currentSeconds = videoFrames / 30;
    displayMinutes = currentSeconds / 60;
    displaySeconds = currentSeconds % 60;
    totalTimeString[0] = displayMinutes / 10 + '0';
    totalTimeString[1] = displayMinutes % 10 + '0';
    totalTimeString[3] = displaySeconds / 10 + '0';
    totalTimeString[4] = displaySeconds % 10 + '0';
    if (showTimeBar) {
      showBar = 50;
    } else {
      showBar = 0;
    }
  } else {
    showStatic = showStaticMax;
    showChannel = 50;
  }
  stopped = false;
}

int stopVideo() {
  tcDisable();
  memset((uint8_t *)audioBuffer1, 0, 1024 * 2);
  memset((uint8_t *)audioBuffer2, 0, 1024 * 2);
  sampleIndex = 0;
  display.endTransfer();
  vidFile.rewind();
  delay(15);
  stopped = true;
}

void startNextVideo() {
  stopVideo();
  getNextFile();
  startNewVideo();
}

void startPreviousVideo() {
  stopVideo();
  getPreviousFile();
  startNewVideo();
}

void seekForward() {
  int frames = 5 * 30;
  vidFile.seekSet(vidFile.curPosition() + (frames * ((96 * 64 * 2) + (1024 * 2))));
  showBar = 50;
  getSeekFrame = true;
}

void seekBackward() {
  int frames = 5 * 30;
  vidFile.seekSet(vidFile.curPosition() - (frames * ((96 * 64 * 2) + (1024 * 2))));
  showBar = 50;
  getSeekFrame = true;
}


void playVideo() {
  if (!vidFile.available()) {
    if (loopVideo || tinyTVmode) {
      vidFile.rewind();
    } else {
      startNextVideo();
    }
  }
  if (!paused || getSeekFrame) {
    //delay(1);//needed?
    vidFile.read(buffer, 96 * 64 * 2);
    if (!paused)while (sampleIndex < 500);//make sure we're not exceeded
    int i = 0;
    uint8_t staticVal = showStatic;
    if (currentAudioBuffer == 1) {
      vidFile.read((uint8_t*)audioBuffer2, 1024 * 2);
      if (tinyTVmode && staticVal && showStaticMax - staticVal < 6) {
        while (i <= 1023) {
          audioBuffer2[i] += (int8_t)((uint8_t)(((uint8_t)SysTick->VAL & 0x7F) | ((uint8_t)(SysTick->VAL << 7) & 0x80))) / 2;
          i += (uint8_t)SysTick->VAL & 0x03;
        }
      }
    } else {
      vidFile.read((uint8_t*)audioBuffer1, 1024 * 2);
      if (tinyTVmode && staticVal && showStaticMax - staticVal < 6) {
        while (i <= 1023) {
          audioBuffer1[i] += (int8_t)((uint8_t)(((uint8_t)SysTick->VAL & 0x7F) | ((uint8_t)(SysTick->VAL << 7) & 0x80))) / 2;
          i += (uint8_t)SysTick->VAL & 0x03;
        }
      }
    }
    getSeekFrame = false;
  }

  if (showBar) {
    showPlayPositionBar();
  }

  if (showStatic) {
    showStaticEffect();
  }

  if (showVolume) {
    showVolumeBar();
  }

  if (showChannel) {
    showChannelNumber();
  }

  if (!tinyTVmode) {
    if (paused) {
      showPause();
      delay(30);
    } else {
      if (showPlaying) {
        if (showPlaying != 25)
          showPlay();
        showPlaying--;
      }
    }
  } else {
    if (paused) {
      tinyTVtubeOff();
    }
  }
  display.writeBufferDMA(buffer, 96 * 64 * 2);
}
