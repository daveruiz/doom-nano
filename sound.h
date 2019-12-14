/*
   For infos how this works see 
   http://fabiensanglard.net/gebbwolf3d_v2.1.pdf chapter 4.9.5
   and
   http://www.shikadi.net/moddingwiki/Inverse_Frequency_Sound_format
   and
   http://www.shikadi.net/moddingwiki/AudioT_Format
*/

#ifndef _sound_h
#define _sound_h

#include <avr/pgmspace.h>
#include "constants.h"

constexpr uint8_t GET_KEY_SND_LEN = 90;
constexpr uint8_t SHOOT_SND_LEN = 27;
constexpr uint8_t shoot_snd[] PROGMEM = { 0x10, 0x10 , 0x10 , 0x6e , 0x2a , 0x20 , 0x28 , 0x28 , 0x9b , 0x28 , 0x20 , 0x20 , 0x21 , 0x57 , 0x20 , 0x20 , 0x20 , 0x67 , 0x20 , 0x20 , 0x29 , 0x20 , 0x73 , 0x20 , 0x20 , 0x20 , 0x89};
constexpr uint8_t get_key_snd[] PROGMEM = {0x24, 0x24, 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x24 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x20 , 0x20 , 0x20 , 0x20 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x20 , 0x20 , 0x20 , 0x20 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x37 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 , 0x19 };
constexpr uint8_t HIT_WALL_SND_LEN = 8;
constexpr uint8_t hit_wall_snd[] PROGMEM = { 0x83 , 0x83 , 0x82 , 0x8e , 0x8a , 0x89 , 0x86 , 0x84};
constexpr uint8_t WALK1_SND_LEN = 3;
constexpr uint8_t walk1_snd[] PROGMEM = { 0x8f, 0x8e, 0x8e};
constexpr uint8_t WALK2_SND_LEN = 3;
constexpr uint8_t walk2_snd[] PROGMEM = { 0x84, 0x87, 0x84};

uint8_t idx = 0;
bool sound = false;
uint16_t snd_ptr = 0;
uint8_t snd_len = 0;

void sound_init() {
  pinMode(SOUND_PIN, OUTPUT);

  TCCR2A = (1 << WGM21); // CTC
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // prescaler 1024
  OCR2A = 112 - 1; // 16000000 / 1024 / 112 -> 139,5 Hz
  TIMSK2 = (1 << OCIE2A);
}

void playSound(const uint8_t* snd, uint8_t len) {
  snd_ptr = reinterpret_cast<uint16_t>(snd);
  snd_len = len;
  sound = true;
}

// Set the frequency that we will get on pin OCR1A
void setFrequency(uint16_t freq) {
  uint32_t requiredDivisor = (F_CPU / 2) / (uint32_t)freq;

  uint16_t prescalerVal;
  uint8_t prescalerBits;
  if (requiredDivisor < 65536UL) {
    prescalerVal = 1;
    prescalerBits = 1; // prescaler 1
  } else if (requiredDivisor < 8 * 65536UL) {
    prescalerVal = 8;
    prescalerBits = 2; // prescaler 8
  } else if (requiredDivisor < 64 * 65536UL) {
    prescalerVal = 64;
    prescalerBits = 3; // prescaler 64
  } else if (requiredDivisor < 256 * 65536UL) {
    prescalerVal = 256;
    prescalerBits = 4; // prescaler 256
  } else {
    prescalerVal = 1024;
    prescalerBits = 5; // prescaler 1024
  }

  uint16_t top = ((requiredDivisor + (prescalerVal / 2)) / prescalerVal) - 1;
  TCCR1A = _BV(COM1A0) /*+ _BV(COM1B0)*/;
  TCCR1B = (1 << WGM12) | prescalerBits;  // CTC
  TCCR1C = _BV(FOC1A);
  OCR1A = top;
}

void off() {
  TCCR1A = 0;
}

ISR(TIMER2_COMPA_vect) {
  if (sound) {
    if (idx++ < snd_len) {
      uint16_t freq = 1192030 / (60 * (uint16_t) pgm_read_byte(snd_ptr + idx)); // 1193181
      setFrequency(freq);
    } else {
      idx = 0;
      off();
      sound = false;
    }
  }
}

#endif
