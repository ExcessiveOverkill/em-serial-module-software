#include "user_io.h"
#include "device_descriptor.h"

user_io::user_io(){
}

/*!
    \brief Initialize hardware and configure IO expander
    
    \note Run this after clocks are configured but before the main loop is started
*/
void user_io::init(void){

    // LED active high: PC13
    // SW1 active low: PC14
    // SW2 active low: PC15

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;	// Enable GPIOC Peripheral Clock

    // configure LED for output
    GPIOC->MODER &= ~(GPIO_MODER_MODER13);	// Clear mode bits for PC13
    GPIOC->MODER |= GPIO_MODER_MODER13_0;	// Set mode to output (01)
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT13);	// Set output type to push-pull

    // configure switches for input pull-up
    GPIOC->MODER &= ~(GPIO_MODER_MODER14 | GPIO_MODER_MODER15);	// Clear mode bits for PC14 and PC15
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR14 | GPIO_PUPDR_PUPDR15);	// Clear pull-up/pull-down bits for PC14 and PC15
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR14_0 | GPIO_PUPDR_PUPDR15_0;	// Set pull-up (01) for PC14 and PC15
}


/*!
    \brief Get states of the DIP switches
    
    \return Binary representation of the switch states
*/
uint8_t user_io::get_switch_states(void){
    uint8_t switch_states = (GPIOC->IDR & (GPIO_IDR_ID14 | GPIO_IDR_ID15)) >> 14;	// Read the input data register for PC14 and PC15
    switch_states = ~switch_states & 0b11;	// Invert and mask the switch states (active low)
    return switch_states;
}


/*!
    \brief Set state of LED
    
    \param led_mode_ Mode to set the LED to given by led_mode_enum

    \note
    Led mode options: {off, on, blink_slow, blink_medium, blink_fast}

    \note
    Example:
    `userIO.set_led_state(userIO.blink_medium)  // blink at medium speed`
*/
void user_io::set_led_state(uint32_t led_mode_){
    led_mode = led_mode_;
}


/*!
    \brief Update IO states
    
    \note Run this as whatever period was set when user_IO was created
    
    \note a call to this function STARTS the update cycle, it is non-blocking so the actual states will be done updating at some later time
*/
void user_io::SysTick_Handler(void){

    volatile uint64_t t = *micros;	// get the current microseconds value

    bool slow_blink = (*micros & (0b1 << 20)) != 0;    // ~0.25Hz
    bool medium_blink = (*micros & (0b1 << 19)) != 0;    // ~1Hz
    bool fast_blink = (*micros & (0b1 << 17)) != 0;    // ~4Hz

    blink_state = (slow_blink << 1) | (medium_blink << 2) | (fast_blink << 3) | (1 << 4);

    led_state = (blink_state & led_mode) ? 1 : 0;

    if(led_state){
        GPIOC->ODR |= GPIO_ODR_OD13;	// Set PC13 high (LED on)
    }
    else{
        GPIOC->ODR &= ~GPIO_ODR_OD13;	// Set PC13 low (LED off)
    }
}