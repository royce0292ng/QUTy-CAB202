#include <avr/io.h>

volatile uint8_t pb_state;
uint8_t pb;          
uint8_t previous_pb; 
uint8_t pb_falling;
uint8_t pb_rising;
uint8_t pb_change;

void buttons_init(void);
void buttons_update(void);