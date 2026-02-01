#include <avr/io.h>

volatile uint8_t left_byte, right_byte;
uint8_t led_hex[16];

void spi_init(void);
void spi_write(uint8_t data);
void display_hex(uint8_t index);