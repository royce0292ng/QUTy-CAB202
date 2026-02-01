#include <avr/io.h>

// Initialize UART
void uart_init(void)
{
    PORTB.DIRSET = PIN2_bm;                         // Enable PB2 as output (USART0 TXD)
    USART0.BAUD = 4167;                             // 9600 baud @ 10 MHz
    USART0.CTRLA = USART_RXCIE_bm ;                 // Enable RX interrupts
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;   // Enable Tx/Rx
}

// Get character 
uint8_t uart_getc(void)
{
    while (!(USART0.STATUS & USART_RXCIF_bm)); // Wait for data
    return USART0.RXDATAL;
}

// Print character
void uart_putc(uint8_t c)
{
    while (!(USART0.STATUS & USART_DREIF_bm)); // Wait for TXDATA empty
    USART0.TXDATAL = c;
}

// Print String 
void uart_puts(char *string)
{
    const char *ptr = string;
    while (*ptr)
    {
        uart_putc(*(ptr++));
    }
}

// Convert character to Hex value 
uint8_t read_hex(uint8_t left, uint8_t right)
{
    uint8_t hex = 0;
    if (left >= 0x30 && left <= 0x39)
    {
        hex |= (left - 0x30) << 4;
    }
    else if (left >= 0x61 && left <= 0x66)
    {
        hex |= ((left - 0x61) + 0x0A) << 4;
    }
    if (right >= 0x30 && right <= 0x39)
    {
        hex |= (right - 0x30);
    }
    else if (right >= 0x61 && right <= 0x66)
    {
        hex |= ((right - 0x61) + 0x0A);
    }

    return hex;
}

