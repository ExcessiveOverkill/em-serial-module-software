#include "fans.h"

#define APB1_Timer_Clock_Frequency 100000000    // TODO: get this from some higher level configuration

fans::fans(logging* logs){
    this->logs = logs;
}

void fans::configure_GPIOB6_for_PWM(void){

    // Enable clock for GPIOB
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // Configure PB6 as alternate function (AF2 for TIM4_CH1)
    GPIOB->MODER &= ~(GPIO_MODER_MODER6);
    GPIOB->MODER |= (GPIO_MODER_MODER6_1);
    GPIOB->AFR[0] |= (2 << GPIO_AFRL_AFSEL0_Pos);
}

void fans::configure_TIM4_for_PWM(void){

    // Enable clock for TIM4
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    // Set prescaler to get 25kHz PWM frequency
    TIM4->PSC = (APB1_Timer_Clock_Frequency / 25000 / 1000) - 1;

    // Set auto-reload register for 1000 ticks (for 25kHz PWM)
    TIM4->ARR = 1000 - 1;

    // Set compare value to 0 (0% duty cycle initially)
    TIM4->CCR1 = 0;

    // Configure TIM4 channel 1 in PWM mode 1
    TIM4->CCMR1 &= ~(TIM_CCMR1_OC1M);
    TIM4->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);
    TIM4->CCMR1 |= TIM_CCMR1_OC1PE;

    // Enable capture/compare channel 1
    TIM4->CCER |= TIM_CCER_CC1E;

    // Enable counter
    TIM4->CR1 |= TIM_CR1_CEN;
}


void fans::configure_GPIOA6_for_tachometer(void){

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // Enable clock for GPIOA
    GPIOA->MODER &= ~(0x3 << (6 * 2));     // Clear MODER6[1:0]
    GPIOA->MODER |= (0x2 << (6 * 2));      // Set MODER6[1:0] to '10' (AF mode)
    GPIOA->PUPDR &= ~(0x3 << (6 * 2));     // No pull-up, no pull-down
    GPIOA->OSPEEDR &= ~(0x3 << (6 * 2));   // Clear speed bits
    GPIOA->OSPEEDR |= (0x1 << (6 * 2));    // Medium speed (optional)
    GPIOA->AFR[0] &= ~(0xF << (6 * 4));    // Clear AFRL6[3:0]
    GPIOA->AFR[0] |= (0x2 << (6 * 4));     // Set AFRL6[3:0] to AF2 (TIM3_CH1)
}
void fans::configure_TIM3_for_tachometer(void){

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;    // Enable clock for TIM3
    TIM3->CR1 &= ~TIM_CR1_CEN;             // Disable TIM3
    TIM3->PSC = 0;                         // Set prescaler to 0
    TIM3->ARR = 0xFFFF;                    // Set auto-reload value to maximum
    TIM3->CR1 &= ~TIM_CR1_DIR;             // Up-counting mode

    TIM3->CCMR1 &= ~TIM_CCMR1_CC1S;        // Clear CC1S bits
    TIM3->CCMR1 |= TIM_CCMR1_CC1S_0;       // CC1S = '01' (TI1)
    TIM3->CR1 |= 0b10 << TIM_CR1_CKD_Pos;   // Set input clock division to 4
    TIM3->CCMR1 |= 0b1111 << TIM_CCMR1_IC1F_Pos;        // maximum filter

    TIM3->SMCR &= ~(TIM_SMCR_SMS | TIM_SMCR_TS); // Clear SMS and TS bits
    TIM3->SMCR |= TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0; // SMS = '111'
    TIM3->SMCR |= TIM_SMCR_TS_2 | TIM_SMCR_TS_0; // TS = '101' (TI1FP1)

    TIM3->CR1 |= TIM_CR1_CEN;              // Enable TIM3 counter
}


void fans::init()
{
    configure_GPIOB6_for_PWM();
    configure_TIM4_for_PWM();

    configure_GPIOA6_for_tachometer();
    configure_TIM3_for_tachometer();
}

uint32_t fans::set_speed(uint32_t speed_rpm)
{
    // Limit speed
    if (speed_rpm > MAX_FAN_SPEED_RPM) speed_rpm = MAX_FAN_SPEED_RPM;

    set_speed_rpm = speed_rpm;

    return 0;
}

uint32_t fans::get_fan_speed_rpm(void)
{
    return tachometer_rpm;
}

void fans::SysTick_Handler()
{   
    if(update_cycle_count >= tach_sample_count)
    {
        // Set fan speed
        uint32_t duty_cycle = (set_speed_rpm * 1000) / MAX_FAN_SPEED_RPM;
        TIM5->CCR1 = duty_cycle;

        // Read tachometer 1 values
        tachometer_rpm = TIM3->CNT * SYSTICK_FREQUENCY * (60/2) / tach_sample_count;  // 2 pulses per revolution, convert to RPM
        TIM3->CNT = 0;

        update_cycle_count = 0;
    }
    else{
        update_cycle_count++;
    }
}