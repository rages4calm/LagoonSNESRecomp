#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* A distinct owner for the existing source-qualified blank-screen RAM work.
 * No boot, script warp, submenu, boss script or fabricated return is admitted.
 * `at` points at the real JSL8703 return, followed by the actual door/main JSRs. */
static inline unsigned LagoonDoorWord(const uint8_t *r,unsigned p) {
  return r[p]|r[p+1]<<8;
}
static inline unsigned LagoonDoorTriple(const uint8_t *r,unsigned p) {
  return LagoonDoorWord(r,p)|r[p+2]<<16;
}
static inline int LagoonDoorOwner(const uint8_t *rom,size_t size,
    const uint8_t *r,unsigned at) {
  if(!rom||size!=0x100000||!r||at<0x100||at>0x1F9||
      r[0x100E]!=255||!(r[0x23D]&128)||LagoonDoorWord(r,0x48E)>=75||
      (r[0x500]&0xC0)!=0xC0||!r[0x520]||r[0x521])return 0;
  if(LagoonDoorTriple(r,at)!=0x018792||LagoonDoorWord(r,at+3)!=0xC8E4||
      LagoonDoorWord(r,at+5)!=0x80A2)return 0;
  return !memcmp(rom+0x878A,"\xA9\xFF\x8D\x0E\x10\x22\x03\x87\x01\x60",10)&&
      !memcmp(rom+0xC8E2,"\x20\x72\x87",3)&&
      !memcmp(rom+0x80A0,"\x20\xF6\xC7",3)&&
      !memcmp(rom+0x8703,"\xA9\x1B\x22\xB9\x80\x00",6)&&
      !memcmp(rom+0x8760,"\xA9\x1C\x22\xB9\x80\x00",6);
}
