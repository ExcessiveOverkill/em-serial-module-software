#include "io_driver.h"

void estop_io::init(){
    // setup TIM1 for system update flag

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;		// Enable TIM1   Clock
    TIM1->CR1 |= TIM_CR1_ARPE;		// Enable Auto-reload preload

    TIM1->ARR = (SYSCLK * 1e6) / UPDATE_FREQUENCY;

    TIM1->DIER |= TIM_DIER_UIE; // Enable Update Interrupt

    NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 3); // right below communication tx priority
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

    TIM1->CR1 |= TIM_CR1_CEN;		// Enable counting


    // setup GPIO for shift register IO

    /*
    PB0: input data
    PB1: output enable
    PB2: output clock
    PB7: input clock
    PB9: input load
    PB10: output write
    PB13: output data
    */

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;	// Enable GPIOB Peripheral Clock
    // Clear mode bits for PB0, PB1, PB2, PB7, PB9, PB10 and PB13
    GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER7 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10 | GPIO_MODER_MODER13);
    // Set mode to input (00) for 0 and output (01) for 1, 2, 7, 9, 10 and 13
    GPIOB->MODER |= (0b00 << GPIO_MODER_MODER0_Pos) | (0b01 << GPIO_MODER_MODER1_Pos) | (0b01 << GPIO_MODER_MODER2_Pos) | (0b01 << GPIO_MODER_MODER7_Pos) | (0b01 << GPIO_MODER_MODER9_Pos) | (0b01 << GPIO_MODER_MODER10_Pos) | (0b01 << GPIO_MODER_MODER13_Pos);

    // Set output type to push-pull for 1, 2, 7, 9, 10 and 13
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT7 | GPIO_OTYPER_OT9 | GPIO_OTYPER_OT10 | GPIO_OTYPER_OT13);


    set_input_load(1);
    set_input_clock(0);

    set_output_enable(0);   // active low (enable)
    set_output_clock(0);
    set_output_write(0);
    set_output_data(0);

    write_output_data(); // set output data to defaults
}

void estop_io::read_inputs(void){
    // shift in data from the input shift registers

    load_input_data();

    for(uint8_t i = 0; i < 32; i++){

        bool data = get_input_data();

        tick_input_clock();

        data = !data; // all inputs are active low

        switch (i){
            case 0:
                inputs.brake_B = data;
                break;
            case 1:
                inputs.brake_6 = data;
                break;
            case 2:
                inputs.brake_5 = data;
                break;
            case 3:
                inputs.brake_4 = data;
                break;
            case 4:
                inputs.brake_3 = data;
                break;
            case 5:
                inputs.brake_2 = data;
                break;
            case 6:
                inputs.brake_1 = data;
                break;
            case 7:
                inputs.brake_A = data;
                break;
            case 8:
                inputs.chain_A_relay = data;
                break;
            case 9:
                inputs.chain_B = data;
                break;
            case 10:
                inputs.chain_A = data;
                break;
            case 11:
                inputs.deadman_switch = data;
                break;
            case 12:
                inputs.chain_B_relay = data;
                break;
            case 13:
                inputs.isolation_contactor_ext = data;
                break;
            case 14:
                inputs.precharge_contactor_ext = data;
                break;
            case 15:
                inputs.main_contactor_ext = data;
                case 16:
                inputs.aux_1 = data;
                break;
            case 17:
                inputs.aux_2 = data;
                break;
            case 18:
                inputs.aux_3 = data;
                break;
            case 19:
                inputs.aux_12 = data;
                break;
            case 20:
                inputs.precharge_contactor_int = data;
                break;
            case 21:
                inputs.main_contactor_int = data;
                break;
            case 22:
                inputs.isolation_contactor_int = data;
                break;
            case 23:
                // unconnected
            case 24:
                inputs.aux_11 = data;
                break;
            case 25:
                inputs.aux_10 = data;
                break;
            case 26:
                inputs.aux_9 = data;
                break;
            case 27:
                inputs.aux_8 = data;
                break;
            case 28:
                inputs.aux_7 = data;
                break;
            case 29:
                inputs.aux_6 = data;
                break;
            case 30:
                inputs.aux_5 = data;
                break;
            case 31:
                inputs.aux_4 = data;
                break;
            default:
                break;
        };
    }

}

void estop_io::write_outputs(void){
    // shift out data to the output shift registers

    for(uint8_t i = 0; i < 16; i++){
        bool data = 0;

        switch (i){
            case 0:
                data = outputs.isolation_contactor;
                break;
            case 1:
                data = outputs.main_contactor;
                break;
            case 2:
                data = outputs.precharge_contactor;
                break;
            case 3:
                // unconnected
                break;
            case 4:
                // unconnected
                break;
            case 5:
                // unconnected
                break;
            case 6:
                // unconnected
                break;
            case 7:
                // unconnected
                break;
            case 8:
                data = outputs.chain_relay;
                break;
            case 9:
                data = outputs.brake_1;
                break;
            case 10:
                data = outputs.brake_2;
                break;
            case 11:
                data = outputs.brake_3;
                break;
            case 12:
                data = outputs.brake_4;
                break;
            case 13:
                data = outputs.brake_5;
                break;
            case 14:
                data = outputs.brake_6;
                break;
            case 15:
                // unconnected
                break;
            default:
                break;
        };

        set_output_data(data);
        tick_output_clock();
    }

    write_output_data(); // output the data
}

void estop_io::tick_output_clock(void){
    set_output_clock(1); // data moves on the rising edge of the clock
    __NOP();
    set_output_clock(0);
}

void estop_io::write_output_data(void){
    set_output_write(1);
    __NOP();
    set_output_write(0);
}

void estop_io::load_input_data(void){
    set_input_load(0); // Load the input data into the shift register (active low)
    __NOP();
    set_input_load(1);
}

void estop_io::tick_input_clock(void){
    set_input_clock(1); // data moves on the rising edge of the clock
    __NOP();
    set_input_clock(0);
}

bool estop_io::get_input_data(void){
    return (GPIOB->IDR & GPIO_IDR_ID0) >> 0; // Read the input data register for PB0
}

void estop_io::set_output_data(bool data){
    if(data){
        GPIOB->ODR |= GPIO_ODR_OD13; // Set PB13 high
    }
    else{
        GPIOB->ODR &= ~GPIO_ODR_OD13; // Set PB13 low
    }
}

void estop_io::set_output_enable(bool enable){
    if(enable){
        GPIOB->ODR |= GPIO_ODR_OD1; // Set PB1 high
    }
    else{
        GPIOB->ODR &= ~GPIO_ODR_OD1; // Set PB1 low
    }
}

void estop_io::set_output_clock(bool clock){
    if(clock){
        GPIOB->ODR |= GPIO_ODR_OD2; // Set PB2 high
    }
    else{
        GPIOB->ODR &= ~GPIO_ODR_OD2; // Set PB2 low
    }
}

void estop_io::set_input_clock(bool clock){
    if(clock){
        GPIOB->ODR |= GPIO_ODR_OD7; // Set PB7 high
    }
    else{
        GPIOB->ODR &= ~GPIO_ODR_OD7; // Set PB7 low
    }
}

void estop_io::set_input_load(bool load){
    if(load){
        GPIOB->ODR |= GPIO_ODR_OD9; // Set PB9 high
    }
    else{
        GPIOB->ODR &= ~GPIO_ODR_OD9; // Set PB9 low
    }
}

void estop_io::set_output_write(bool write){
    if(write){
        GPIOB->ODR |= GPIO_ODR_OD10; // Set PB10 high
    }
    else{
        GPIOB->ODR &= ~GPIO_ODR_OD10; // Set PB10 low
    }
}