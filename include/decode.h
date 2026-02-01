#include <stdint.h>

#define MASK 0xB4BCD35C
uint32_t state;

uint8_t table_index(uint8_t alphabet);

void base64_decode_three(uint8_t *input, uint8_t *output);

void next(void);
void descramble(uint8_t *pointer);
