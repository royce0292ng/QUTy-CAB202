#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>

#include "sequence.h"
#include "decode.h"
#include "uart.h"
#include "spi.h"
#include "timer.h"
#include "buttons.h"

// Enum for Mode selection
enum control
{
    SEQUENCE_SELECT,
    SEQUENCING,
    TEST
};
// Enum for Sequence select Mode serial interface
enum cmd_1
{
    START_1,
    ESCAPE_1,
    ID_1,
    SYN_1,
    SEQIDX_PAYLOAD_1,
    SEQIDX_PAYLOAD_2
};
// Enum for Sequence Mode serial interface
enum cmd_2
{
    START_2,
    ESCAPE_2,
    ID_2,
    SYN_2
};
// Enum for Test Mode serial interface
enum cmd_3
{
    START_3,
    ESCAPE_3,
    ID_3,
    SYN_3,
    SEQ_3
};
// Enum for serial interface response
enum response
{
    R_ACK,
    R_NACK,
    R_DEBUG
};
// Enum for pause
enum pause
{
    WAIT,
    ONE_STEP,
    RE_PLAY,
    EXIT
};

// Default enum & payload variable
enum control modes = SEQUENCE_SELECT;
enum cmd_1 cmd_select = START_1;
enum cmd_2 cmd_seq = START_2;
enum cmd_3 cmd_test = START_3;
enum response res = R_NACK;
enum pause pause_state = WAIT;
uint8_t first_p;
uint8_t second_p;

// Sequence select mode flag
volatile uint8_t trap_ss = 1;

// Serial interface Flag
volatile uint8_t play = 0;
volatile uint8_t pause = 0;

// Sequence mode variable
uint8_t seq_index = 0;
uint16_t duration;
volatile uint16_t duration_count;
uint8_t brightness;
uint8_t octave, note_index;
uint32_t note_cycle = 363636;
uint32_t note[12] = {363636, 343227, 323963, 305781, 288618, 272419, 257130, 242698, 229077, 216219, 204084, 192630};

// Studio initiation
static int stdio_putchar(char c, FILE *stream);
static int stdio_getchar(FILE *stream);

static FILE stdio = FDEV_SETUP_STREAM(stdio_putchar, stdio_getchar, _FDEV_SETUP_RW);

static int stdio_putchar(char c, FILE *stream)
{
    uart_putc(c);
    return c; // the putchar function must return the character written to the stream
}
static int stdio_getchar(FILE *stream)
{
    return uart_getc();
}
void stdio_init(void)
{
    // Assumes serial interface is initialised elsewhere
    stdout = &stdio;
    stdin = &stdio;
}

int main(void)
{
    // Set CPU Clock Speed to 10 MHz
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm;

    // Initialize on board hardware
    cli();
    uart_init();
    stdio_init();
    timer_init();
    spi_init();
    buttons_init();
    pwm_init();
    adc_init();
    PORTB.DIRSET = PIN0_bm | PIN1_bm;
    sei();

    // Create Decoding variable
    uint8_t steps[3];
    char sequence[4];

    // Create offset position variable
    uint16_t offset = 0;
    uint8_t pos = 0;

    // Main Loop (Contain whole system)
main_loop:
    while (1)
    {
        // Mode Switch cases
        switch (modes)
        {
        // Sequence selecting mode
        case SEQUENCE_SELECT:
            TCB1.INTCTRL = TCB_CAPT_bm; // Enable TCB1 (Switching Left and Right Display)
            state = 0x10944125;         // Reset state value
            display_hex(seq_index);     // Display Current sequence index
            trap_ss = 1;                // Sequence select Flag
            // Loop in Sequence select mode
            while (trap_ss)
            {
                buttons_update();
                // S1 press
                if (pb_falling & PIN4_bm)
                {
                    // constantly read Potentiometer reading
                    while (1)
                    {
                        seq_index = ADC0.RESULT;
                        display_hex(seq_index);
                        buttons_update();
                        // S1 release
                        if ((pb_rising & PIN4_bm))
                        {
                            break;
                        }
                    }
                }
                // S2 press
                if (pb_falling & PIN5_bm)
                {
                    if (seq_index > 0)
                    {
                        seq_index--;
                        display_hex(seq_index);
                    }
                }
                // S3 press
                if (pb_falling & PIN6_bm)
                {
                    if (seq_index < 255)
                    {
                        seq_index++;
                        display_hex(seq_index);
                    }
                }
                // S4 press
                if (pb_falling & PIN7_bm)
                {
                    modes = SEQUENCING;
                    break;
                }
            }
            break;

        // Sequence mode
        case SEQUENCING:
            TCB1.INTCTRL &= (~TCB_CAPT_bm); // Disable TCB1 (Switching Left and Right Display)
            spi_write(0x80);                // Set Display on LHS with 8

            // Increment state step
            for (uint16_t i = 0; i < seq_index * 8 * 3; i++)
            {
                next();
            }

            // Run throught the Seqence
            for (offset = seq_index; offset < 256; offset++)
            {
                for (pos = 0; pos < 32; pos += 4)
                {
                    memcpy_P(sequence, &SEQUENCE[offset * 32 + pos], 4); // Copy 4 character
                    base64_decode_three(sequence, steps);                // BASE64 decoding

                    // Descramble Bytes
                    for (uint8_t n = 0; n < 3; n++)
                    {
                        descramble(steps + n);
                    }

                    // Read instruction
                    duration = steps[0] << 1;
                    brightness = steps[1];
                    octave = steps[2] >> 4;
                    note_index = steps[2] & 0xF;
                    note_cycle = (note[note_index] >> (octave));

                    // Buzzer
                    if (octave == 0)
                    {
                        TCA0.SINGLE.CMP0BUF = 0;
                    }
                    else
                    {
                        TCA0.SINGLE.PERBUF = note_cycle;
                        TCA0.SINGLE.CMP0BUF = note_cycle >> 1;
                    }

                    // LED
                    TCA0.SINGLE.CMP1BUF = ((uint32_t)(note_cycle) * (uint32_t)(brightness)) >> 8;

                    // Sequence termination
                    if (duration == 0)
                    {
                        modes = SEQUENCE_SELECT;
                        TCA0.SINGLE.CMP1BUF = note_cycle;
                        goto main_loop;
                    }

                    // Reset flag value
                    duration_count = 0;
                    play = 1;

                    // Within Duration
                    while (1)
                    {
                        buttons_update();

                        // S4 press or CMD pause
                        if (pb_falling & PIN7_bm || pause)
                        {
                            play = 0; // Stop duration counter

                            // wait for next instrution
                            while (1)
                            {
                                buttons_update();
                                // Switch case when pause
                                switch (pause_state)
                                {
                                case WAIT:
                                    // S2 press
                                    if (pb_falling & PIN5_bm)
                                        pause_state = EXIT;
                                    // S3 press
                                    if (pb_falling & PIN6_bm)
                                        pause_state = ONE_STEP;
                                    // S4 press
                                    if (pb_falling & PIN7_bm)
                                        pause_state = RE_PLAY;
                                    break;
                                case EXIT:
                                    TCA0.SINGLE.CMP0BUF = 0;          // Stop Buzzer
                                    TCA0.SINGLE.CMP1BUF = note_cycle; // Full Brightness
                                    modes = SEQUENCE_SELECT;          // Switch mode back to sequence select
                                    pause = 0;                        // Cancelling pause
                                    pause_state = WAIT;
                                    goto main_loop;
                                    break;
                                case ONE_STEP:
                                    pause = 1;    // Keep pausing
                                    pause_state = WAIT;
                                    goto loop_end;
                                    break;
                                case RE_PLAY:
                                    pause = 0;   // Cancelling pause
                                    pause_state = WAIT;
                                    goto loop_end;
                                    break;

                                default:
                                    pause_state = WAIT;
                                    break;
                                }
                            }
                        }
                        // EXIT sequence mode
                        if (pause_state == EXIT)
                        {
                            TCA0.SINGLE.CMP0BUF = 0;          // Stop Buzzer
                            TCA0.SINGLE.CMP1BUF = note_cycle; // Full Brightness
                            modes = SEQUENCE_SELECT;          // Switch mode back to sequence select
                            pause_state = WAIT;
                            goto main_loop;
                        }
                        if (duration_count >= duration)
                        {
                            play = 0; // Stop duration counter
                            break;    // break and start next step
                        }
                    }
                loop_end:;
                }
            }
            break;

        case TEST:
            // Display " - - "
            left_byte = 0xF7;
            right_byte = 0x77;
            break;

        default:
            modes = SEQUENCE_SELECT;
            break;
        }
    }
} // end main()

// TCB 16-Bit Timer overflow
ISR(TCB0_INT_vect)
{
    // Button Debouncing
    static uint8_t count0 = 0;
    static uint8_t count1 = 0;

    volatile uint8_t pb_sample = PORTA.IN;
    volatile uint8_t pb_changed = pb_sample ^ pb_state;

    count1 = (count1 ^ count0) & pb_changed;
    count0 = ~count0 & pb_changed;

    pb_state ^= (count1 & count0) | (pb_changed & pb_state);

    // Duration counting
    if (play)
    {
        duration_count++;
    }

    TCB0.INTFLAGS = TCB_OVF_bm; // Clear interrupt flag
}

ISR(USART0_RXC_vect)
{
    uint8_t c = USART0.RXDATAL; // Read new data into c
    // Different CMD with different response 
    switch (modes)
    {
    // Sequence Select mode CMD
    case SEQUENCE_SELECT:
        switch (cmd_select)
        {
        // Check for backslash
        case START_1:
            if (c == 0x5C)
            {
                cmd_select = ESCAPE_1;
            }
            break;
        // Check for backslash
        case ESCAPE_1:
            if (c == 0x5C)
            {
                cmd_select = ID_1;
            }
            else
            {
                cmd_select = START_1;
            }
            break;
        // Check CMD ID
        case ID_1:
            // Play 's'
            if (c == 0x73)
            {
                res = R_ACK;
                modes = SEQUENCING;
                cmd_select = START_1;
                trap_ss = 0;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            // Test mode 't'
            else if (c == 0x74)
            {
                res = R_ACK;
                modes = TEST;
                cmd_select = START_1;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
                trap_ss = 0;
            }
            // Sequence index 'i'
            else if (c == 0x69)
            {
                cmd_select = SEQIDX_PAYLOAD_1;
            }
            // CMD sync 'y'
            else if (c == 0x79)
            {
                cmd_select = SYN_1;
            }
            else
            {
                res = R_NACK;
                cmd_select = START_1;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            break;
        // Take first payload
        case SEQIDX_PAYLOAD_1:
            first_p = c;
            cmd_select = SEQIDX_PAYLOAD_2;
            break;
        // Take second payload and process 
        case SEQIDX_PAYLOAD_2:
            second_p = c;
            if (!((second_p >= 0x30 && second_p <= 0x39) || (second_p >= 0x61 && second_p <= 0x66)) || !((first_p >= 0x30 && first_p <= 0x39) || (first_p >= 0x61 && first_p <= 0x66)))
            {
                res = R_NACK;
                cmd_select = START_1;
            }
            else
            {
                seq_index = read_hex(first_p, second_p);
                display_hex(seq_index);
                cmd_select = START_1;
                res = R_ACK;
            }
            USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            break;
        // SYNC case
        case SYN_1:

            break;

        default:
            cmd_select = START_1;
            break;
        }
        break;
    // Sequence mode CMD
    case SEQUENCING:
        switch (cmd_seq)
        {
        // Check for backslash
        case START_2:
            if (c == 0x5C)
            {
                cmd_seq = ESCAPE_2;
            }
            break;
        // Check for backslash
        case ESCAPE_2:
            if (c == 0x5C)
            {
                cmd_seq = ID_2;
            }
            else
            {
                cmd_seq = START_2;
            }
            break;
        // Check CMD ID
        case ID_2:
            // Play 's'
            if (c == 0x73)
            {
                res = R_ACK;
                cmd_seq = START_2;
                pause_state = RE_PLAY;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            // Exit 'e'
            else if (c == 0x65)
            {
                res = R_ACK;
                cmd_seq = START_2;
                pause_state = EXIT;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            // Pause 'p'
            else if (c == 0x70)
            {
                res = R_ACK;
                cmd_seq = START_2;
                pause = 1;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            // Step 'n'
            else if (c == 0x6E)
            {
                res = R_ACK;
                cmd_seq = START_2;
                pause_state = ONE_STEP;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            else
            {
                res = R_NACK;
                cmd_seq = START_2;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            break;
        // SYNC case
        case SYN_2:
            break;
        }
        break;
    // Test mode CMD
    case TEST:
        switch (cmd_test)
        {
        // Check for backslash
        case START_3:
            if (c == 0x5C)
            {
                cmd_test = ESCAPE_3;
            }
            break;
        // Check for backslash
        case ESCAPE_3:
            if (c == 0x5C)
            {
                cmd_test = ID_3;
            }
            else
            {
                cmd_test = START_3; 
            }
            break;
        // Check CMD ID
        case ID_3:
            // Scrambled 'd'
            if (c == 0x64)
            {
                res = R_ACK;
                cmd_test = START_3;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            // Decode 'u'
            else if (c == 0x75)
            {
                res = R_ACK;
                cmd_test = START_3;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            // Exit 'e'
            else if (c == 0x65)
            {
                res = R_ACK;
                cmd_test = START_3;
                modes = SEQUENCE_SELECT;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            else
            {
                res = R_NACK;
                cmd_test = START_3;
                USART0.CTRLA |= USART_DREIE_bm; // Enable DRE interupt
            }
            break;
        default:
            cmd_test = START_3; // Set defalut value 
            break;
        }
        break;
    default:
        modes = SEQUENCE_SELECT; // Set defalut value
        break;
    }
}

ISR(USART0_DRE_vect)
{
    // CMD response Swich case
    switch (res)
    {
    // Acknowledge
    case R_ACK:
        uart_puts("#ACK\n");
        res = R_NACK;
        break;
    // Not-Acknowledge
    case R_NACK:
        uart_puts("#NACK\n");
        res = R_NACK;
        break;
    // Debug
    case R_DEBUG:
        uart_puts("? \n");
        res = R_NACK;
        break;

    default:
        res = R_NACK;
        break;
    }
    USART0.CTRLA &= ~USART_DREIE_bm; // Disable interrupt
}
