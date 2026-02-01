#include <avr/io.h>
#include <avr/interrupt.h>

// Display variable
volatile uint8_t left_byte, right_byte;

// Display List for 0-9 a-f
uint8_t led_hex[16] = {
    0b00001000,
    0b01101011,
    0b01000100,
    0b01000001,
    0b00100011,
    0b00010001,
    0b00010000,
    0b01001011,
    0b00000000,
    0b00000011,
    0b00000010,
    0b00110000,
    0b00011100,
    0b01100000,
    0b00010100,
    0b00010110
};

// Inisialize spi unit
void spi_init(void)
{
    PORTB.OUTSET = PIN1_bm;                     // output HIGH (LED on)
    PORTB.DIRSET = PIN1_bm;                     // enable as output (will be driven by WO2 when enabled)
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;   // SPI pins on PC0-3
    PORTA.DIRSET = PIN1_bm;                     // Enable display latch
    PORTC.DIR = (PIN0_bm | PIN2_bm);            // Set SCK (PC0) and MOSI (PC2) as outputs
    
    SPI0.CTRLA = (SPI_MASTER_bm | SPI_ENABLE_bm); // Master, /4 prescaler, MSB first & Enable
    SPI0.CTRLB = SPI_SSD_bm;                    // Mode 0, client select disable, unbuffered
    SPI0.INTCTRL = SPI_IE_bm;                   // Enable Interupt
    
}

// Write data in spi
void spi_write(uint8_t data)
{
    SPI0.DATA = data; // Note DATA register used for both Tx and Rx
}

// Display number by Hex
void display_hex(uint8_t index)
{
    uint8_t left = (index >> 4);
    uint8_t right = (index & 0x0F);

    left_byte = (led_hex[left] | 0x80);
    right_byte = led_hex[right];
}

ISR(SPI0_INT_vect)
{
    // Activate Display latch
    PORTA.OUTCLR = PIN1_bm;
    PORTA.OUTSET = PIN1_bm;

    SPI0.INTFLAGS = SPI_IF_bm; // Clear interrupt flag
}

ISR(TCB1_INT_vect)
{
    // Switching LED Left and Right Display
    static uint8_t B_flag = 0;
    if (B_flag)
    {
        spi_write(left_byte);
    }
    else
    {
        spi_write(right_byte);
    }
    B_flag = ~B_flag;

    TCB1.INTFLAGS = TCB_CAPT_bm; // Clear interrupt flag
}