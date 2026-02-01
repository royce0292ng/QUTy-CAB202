#include <avr/io.h>

// PB variable
volatile uint8_t pb_state = 0xFF;
uint8_t pb = 0xFF;         
uint8_t previous_pb = 0xFF; 
uint8_t pb_falling = 0;
uint8_t pb_rising = 0;
uint8_t pb_change = 0;

void buttons_init(void)
{
    // Enable pull-up resistors for PBs
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm;
}

void buttons_update(void)
{
    previous_pb = pb;
    pb = pb_state;
    pb_change = (pb ^ previous_pb);
    pb_falling = pb_change & ~pb;
    pb_rising = pb_change & pb;
}