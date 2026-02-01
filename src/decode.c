#include <stdint.h>

volatile uint32_t state = 0x10944125; // Student ID as descrambling state

#define MASK 0xB4BCD35C // Descrambling Mask 

// Base64 table
static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

// Identify Base64 character value  
uint8_t table_index(uint8_t alphabet)
{
    uint8_t i;
    for (i = 0; i < 64; i++)
    {
        if (encoding_table[i] == alphabet)
        {
            break;
        }
    }
    return i;
}

// Base64 Decoding 
void base64_decode_three(uint8_t *input, uint8_t *output)
{
    *output = ((table_index(*input) << 2) | (table_index(*(input + 1)) >> 4));
    *(output + 1) = ((table_index(*(input + 1)) << 4) | (table_index(*(input + 2)) >> 2));
    *(output + 2) = ((table_index(*(input + 2)) << 6) | (table_index(*(input + 3))));
}

// Increase state value by one step
void next(void)
{
    if (state & 0x01)
    {
        state = state >> 1;
        state ^= MASK;
    }
    else
    {
        state = state >> 1;
    }
}

// Descramble Byte 
void descramble(uint8_t *pointer)
{
    *pointer ^= (state & 0xFF);
    next();
}