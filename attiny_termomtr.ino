#include <util/delay.h>

#define DS_PIN  PB2 
#define CLK_PIN PB1 
#define DIO_PIN PB0 

const uint8_t digits[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};

// --- Функції DS18B20 (залишаються без змін) ---
uint8_t ds_reset() {
  uint8_t r;
  DDRB |= (1 << DS_PIN); PORTB &= ~(1 << DS_PIN);
  _delay_us(480);
  DDRB &= ~(1 << DS_PIN); _delay_us(70);
  r = !(PINB & (1 << DS_PIN)); _delay_us(410);
  return r;
}

void ds_write_bit(uint8_t bit) {
  DDRB |= (1 << DS_PIN); _delay_us(1);
  if (bit) DDRB &= ~(1 << DS_PIN);
  _delay_us(60); DDRB &= ~(1 << DS_PIN);
}

uint8_t ds_read_bit() {
  uint8_t r = 0;
  DDRB |= (1 << DS_PIN); _delay_us(1);
  DDRB &= ~(1 << DS_PIN); _delay_us(14);
  if (PINB & (1 << DS_PIN)) r = 1;
  _delay_us(45); return r;
}

void ds_write_byte(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) ds_write_bit(data >> i & 1);
}

uint8_t ds_read_byte() {
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) if (ds_read_bit()) data |= (1 << i);
  return data;
}

// --- Функції TM1637 (залишаються без змін) ---
void tm_send(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    PORTB &= ~(1 << CLK_PIN);
    if (data & 1) PORTB |= (1 << DIO_PIN); else PORTB &= ~(1 << DIO_PIN);
    data >>= 1; _delay_us(1);
    PORTB |= (1 << CLK_PIN); _delay_us(1);
  }
  PORTB &= ~(1 << CLK_PIN); DDRB &= ~(1 << DIO_PIN);
  PORTB |= (1 << CLK_PIN); _delay_us(1); DDRB |= (1 << DIO_PIN);
}

void tm_start() {
  PORTB |= (1 << CLK_PIN); PORTB |= (1 << DIO_PIN);
  PORTB &= ~(1 << DIO_PIN);
}

void tm_stop() {
  PORTB &= ~(1 << CLK_PIN); PORTB &= ~(1 << DIO_PIN);
  PORTB |= (1 << CLK_PIN); PORTB |= (1 << DIO_PIN);
}

void setup() {
  DDRB |= (1 << CLK_PIN) | (1 << DIO_PIN);
  
  // Ініціалізація дисплея
  tm_start(); tm_send(0x40); tm_stop();
  tm_start(); tm_send(0x89); tm_stop();

  // --- НАЛАШТУВАННЯ ДАТЧИКА НА 9 БІТ (ШВИДКИЙ РЕЖИМ) ---
  ds_reset();
  ds_write_byte(0xCC); // Skip ROM
  ds_write_byte(0x4E); // Write Scratchpad
  ds_write_byte(0x4B); // TH (не важливо)
  ds_write_byte(0x46); // TL (не важливо)
  ds_write_byte(0x1F); // Конфігурація: 0x1F = 9 біт (0x7F = 12 біт)
  ds_reset();
}

void loop() {
  ds_reset();
  ds_write_byte(0xCC);
  ds_write_byte(0x44); // Convert T
  
  _delay_ms(100); // --- ТЕПЕР ЧЕКАЄМО ВСЬОГО 100 мс ЗАМІСТЬ 750 ---

  ds_reset();
  ds_write_byte(0xCC);
  ds_write_byte(0xBE);
  
  uint8_t low = ds_read_byte();
  uint8_t high = ds_read_byte();
  int16_t raw = (high << 8) | low;
  int8_t temp = raw >> 4;

  tm_start();
  tm_send(0xC0);
  tm_send(0x00);
  tm_send(digits[(temp / 10) % 10]);
  tm_send(digits[temp % 10]);
  tm_send(0x39);
  tm_stop();
}