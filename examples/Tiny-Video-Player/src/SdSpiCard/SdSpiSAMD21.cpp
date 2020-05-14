/* Arduino SdSpi Library
 * Copyright (C) 2013 by William Greiman
 *
 * This file is part of the Arduino SdSpi Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdSpi Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "SdSpi.h"
#if defined(ARDUINO_ARCH_SAMD)
//------------------------------------------------------------------------------
void SdSpi::begin() {
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
}
//------------------------------------------------------------------------------
//  initialize SPI controller
void SdSpi::beginTransaction(uint8_t sckDivisor) {
#if ENABLE_SPI_TRANSACTIONS
  SPI.beginTransaction(SPISettings());
#endif  // ENABLE_SPI_TRANSACTIONS
}
//------------------------------------------------------------------------------
void SdSpi::endTransaction() {
#if ENABLE_SPI_TRANSACTIONS
  SPI.endTransaction();
#endif  // ENABLE_SPI_TRANSACTIONS
}
//------------------------------------------------------------------------------
static inline uint8_t spiTransfer(uint8_t b) {
  SERCOM1->SPI.DATA.bit.DATA=(b);
  while(SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
  return SERCOM1->SPI.DATA.bit.DATA & 0xFF;
}
//------------------------------------------------------------------------------
/** SPI receive a byte */
uint8_t SdSpi::receive() {
  return spiTransfer(0XFF);
}
//------------------------------------------------------------------------------
/** SPI receive multiple bytes */
uint8_t SdSpi::receive(uint8_t* buf, size_t n) {
  for (size_t i = 0; i < n; i++) {
    SERCOM1->SPI.DATA.bit.DATA = 0XFF;
    while(SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
    buf[i] = SERCOM1->SPI.DATA.bit.DATA;
  }
  return 0;
}
//------------------------------------------------------------------------------
/** SPI send a byte */
void SdSpi::send(uint8_t b) {
  spiTransfer(b);
}
//------------------------------------------------------------------------------
void SdSpi::send(const uint8_t* buf , size_t n) {
  uint8_t temp;
  SERCOM1->SPI.DATA.bit.DATA=buf[0];
  for(size_t j=1;j<n;j++){
    temp=buf[j];
    while(SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
    SERCOM1->SPI.DATA.bit.DATA=temp;
  }
  while(SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
  // leave RDR empty (not implemented)
  //uint8_t b = pSpi->SPI_RDR;
}
#endif  // defined((ARDUINO_ARCH_SAMD)
