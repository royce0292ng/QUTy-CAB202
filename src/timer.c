#include <avr/io.h>
#include <avr/interrupt.h>

void timer_init() {

    // Configure timer for PB sampling
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;    // Configure TCB0 in periodic interrupt mode
    TCB0.CCMP = 0;                      // Set interval for 16 bit overflow 100ns * 2^17 = 1.31ms ( 131071 clocks @ 10 MHz)
    TCB0.INTCTRL = TCB_OVF_bm;          // OVF interrupt enable
    TCB0.CTRLA = TCB_ENABLE_bm;         // Enable

    TCB1.CTRLB = TCB_CNTMODE_INT_gc;    // Configure TCB0 in periodic interrupt mode
    TCB1.CCMP = 10000;                  // Set interval for 1ms ( 10000 clocks @ 10 MHz)
    TCB1.INTCTRL = TCB_CAPT_bm;         // CAPT interrupt enable
    TCB1.CTRLA = TCB_ENABLE_bm;         // Enable
     
}

void pwm_init(void)
{
    PORTMUX.TCAROUTEA = PORTMUX_TCA02_ALT1_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc; // presaler = /1 (10 MHz @ CLK)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_CMP1EN_bm;
    TCA0.SINGLE.PER = 250;                          // Default PER value 
    TCA0.SINGLE.CMP0 = 250;                         // 100% duty
    TCA0.SINGLE.CMP1 = 250;                         // 100% duty
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;      // Enable TCA
}

void adc_init(void)
{
    // Enable ADC
    ADC0.CTRLA = ADC_ENABLE_bm;
    // /2 clock prescaler
    ADC0.CTRLB = ADC_PRESC_DIV2_gc;
    // Need 10 CLK_PER cycles @ 10 MHz for 1us, select VDD as ref
    ADC0.CTRLC = (10 << ADC_TIMEBASE_gp) | ADC_REFSEL_VDD_gc;
    // Sample duration of 64
    ADC0.CTRLE = 64;
    // Free running, left adjust result
    ADC0.CTRLF = ADC_FREERUN_bm | ADC_LEFTADJ_bm;
    // Select AIN2 (potentiomenter R1)
    ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;
    // Select 12-bit resolution, single-ended
    ADC0.COMMAND = ADC_MODE_SINGLE_8BIT_gc | ADC_START_IMMEDIATE_gc;
}