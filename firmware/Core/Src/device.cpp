#include "device.h"

// extern device_struct vars;
// extern void* var_pointers[86];

device::device(){
    comm_vars = &vars;
    comm_var_pointers = var_pointers;
}

void device::init(){
    CPU_init();
    sysTick_init();

    // comment out for debugging
    //watchdog_init(); // initialize the watchdog timer

    logs.init();
    logs.comm_vars = comm_vars;
    // Initialize all the low level classes
    Comm.init();
    UserIO.init();
    EstopIO.init();
    Fans.init();


    micros = Comm.micros;
    UserIO.micros = Comm.micros;
    logs.microseconds = Comm.micros;
    Comm.comm_vars = comm_vars;
    Comm.comm_var_pointers = comm_var_pointers;

    Default_Mode.micros = micros;

    // wait until we have a valid communication address before initializing communication
    Comm.set_device_address(UserIO.get_switch_states());
    
    Comm.enable_resync = true; // enable resync

    Comm.enable(); // enable communication

    logs.clear_all(); // clear any faults from undefined startup

    // check for watchdog reset flag
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset flag is set
        RCC->CSR |= RCC_CSR_RMVF; // clear the reset flag
        logs.add((uint32_t)system_messages::watchdog_timeout);
    }
}

void device::CPU_init(){

    // 100MHz SYSCLK
    assert(SYSCLK == 100);

	FLASH->ACR |= FLASH_ACR_ICEN			// Enable intruction cache
			    | FLASH_ACR_DCEN 			// Enable Date Cache
			    | FLASH_ACR_PRFTEN 			// Enable prefetch
			    | FLASH_ACR_LATENCY_3WS;	// Set Flash latency to 3 wait states

    // attempt to use HSE (25MHz) as the clock source
    RCC->CR |= RCC_CR_HSEBYP; // Bypass HSE since we are using an external clock source (not a crystal)
    RCC->CR |= RCC_CR_HSEON; // Enable HSE clock

    for(int i = 0; i < 1000; i++){
        __NOP(); // wait for HSE to stabilize
    }

    if(!(RCC->CR & RCC_CR_HSERDY)){ // HSE not available
        RCC->CR &= ~RCC_CR_HSEON; // disable HSE clock
    }
    else{
        hse_vcxo_available = true; // HSE is available
    }

    if(!hse_vcxo_available){
    // internal HSI clock mode

    // HSI clock is used as the PLL input clock (16MHz)
    // set VCO to 2Mhz
    // set PLL_N to get SYSCLK*2
    // set PLL_P to 2 to get SYSCLK for the system
    // set PLL_Q to 5 (SYSCLK*2 / 5) for USB, SDIO, RNG, must be 48Mhz or lower
    // set PLL_R to 2 (SYSCLK*2 / 2) for I2S, DFSDM, must be 96Mhz or lower

	RCC->PLLCFGR = (8 << 0)    // Set PLL_M to 8. The input clock frequency is divided by this value.
	             | (SYSCLK << 6)  // Set PLL_N, the multiplication factor for the PLL. SYSCLK is presumably defined elsewhere, representing the desired system clock frequency.
	             | (0 << 16)   // Set PLL_P to 2 (0 in register corresponds to PLL_P = 2). The PLL output frequency is divided by this value to get the system clock.
	             | (5 << 24)  // Set PLL_Q to 5. This value is used for USB, SDIO, and random number generator clocks
                 | (2 << 28); // Set PLL_R to 2. This value is used for I2S and DFSDM clocks
    }
    else{
    // external HSE clock mode (25MHz)

    // HSE clock is used as the PLL input clock (25MHz)
    // set VCO to 1.66667 Mhz
    // set PLL_N to get SYSCLK*2
    // set PLL_P to 2 to get SYSCLK for the system
    // set PLL_Q to 5 (SYSCLK*2 / 5) for USB, SDIO, RNG, must be 48Mhz or lower
    // set PLL_R to 2 (SYSCLK*2 / 2) for I2S, DFSDM, must be 96Mhz or lower

    constexpr uint8_t PLL_N = uint8_t(float(SYSCLK*2.0) / 1.666666666666);

    RCC->PLLCFGR = (15 << 0)    // Set PLL_M to 25. The input clock frequency is divided by this value.
                 | (PLL_N << 6)  // Set PLL_N, the multiplication factor for the PLL. SYSCLK is presumably defined elsewhere, representing the desired system clock frequency.
                 | (0 << 16)   // Set PLL_P to 2 (0 in register corresponds to PLL_P = 2). The PLL output frequency is divided by this value to get the system clock.
                 | (5 << 24)  // Set PLL_Q to 5. This value is used for USB, SDIO, and random number generator clocks
                 | (2 << 28) // Set PLL_R to 2. This value is used for I2S and DFSDM clocks
                 | (RCC_PLLCFGR_PLLSRC_HSE); // Set PLL source to HSE
    }

    // Turn on the PLL and wait for it to become stable
	RCC->CR |= RCC_CR_PLLON;  // Enable the PLL
	while (!(RCC->CR & RCC_CR_PLLRDY)); // Wait for PLL to be ready (PLL ready flag)

	// Switch the system clock source to the PLL
	RCC->CFGR &= ~RCC_CFGR_SW;  // Clear the clock switch bits
	RCC->CFGR |= RCC_CFGR_SW_PLL;  // Set the clock source to PLL
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // Wait until PLL is used as the system clock source

	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;	// divide by 2 to get 50Mhz for APB1 peripherals (max allowed)

	// Update the SystemCoreClock variable to the new clock speed
	SystemCoreClockUpdate(); // Update the SystemCoreClock global variable with the new clock frequency
	
	SystemInit();	// Initialize system
}

void device::delay_us(uint32_t time_us){
    uint64_t end_time = Comm.get_microseconds() + time_us;
    while (Comm.get_microseconds() < end_time){    // Wait until the desired time has passed
        watchdog_reload();
    }
}

void device::delay_ms(uint32_t time_ms){
    for (uint32_t i = 0; i < time_ms; i++){
        delay_us(1e3); // delay 1ms
    }
}

void device::sysTick_init(){
    SysTick->LOAD = ((SYSCLK*1e6) / SYSTICK_FREQUENCY) - 1; // Set the SysTick timer to count at the desired frequency
    NVIC_SetPriority(USART6_IRQn, 15);  // low priority
    NVIC_EnableIRQ(SysTick_IRQn); // Enable the SysTick interrupt
    SysTick->CTRL = 0b111;	// Enable counter, interrupt, and set clock source to system clock
}

void device::watchdog_init(){
    IWDG->KR = 0x5555; // Enable write access to the IWDG_PR and IWDG_RLR registers
    IWDG->PR = 0b000; // Set the prescaler to 4 (LSI at 32khz / 4)
    IWDG->RLR = 8000 / 1000; // Set the reload value to 1ms
    IWDG->KR = 0xAAAA; // Reload the watchdog timer
    IWDG->KR = 0xCCCC; // Start the watchdog timer
}

void device::watchdog_reload(){
    IWDG->KR = 0xAAAA; // Reload the watchdog timer
}

void device::run(){
    tim1_update_missed = false; // reset missed update flag
    tim1_up_tim10_flag = false;

    while(1){   // main loop

        // run flag interrupt handlers
        if(sysTick_flag){
            flagged_sysTick();
        }
        if(tim2_flag){
            flagged_tim2();
        }
        if(tim5_flag){
            flagged_tim5();
        }
        if(dma2_stream1_flag){
            flagged_dma2_stream1();
        }
        if(tim1_up_tim10_flag){
            flagged_tim1_up_tim10();
            watchdog_reload(); // reload the watchdog timer
        }
        if(tim1_update_missed){
            logs.add((uint32_t)system_messages::control_deadline_missed);
        }

        // handle requested state changes from controller
        if(vars.requested_state != last_controller_requested_state){    // new requested state
            switch(vars.requested_state){
                case 0:
                    break; // nothing
                case 3:
                    logs.clear_all(); // clear all faults
                    break;
                default:
                    logs.add((uint32_t)system_messages::invalid_state); // invalid state requested
                    break;
            }
            last_controller_requested_state = vars.requested_state;
        }

        // handle error states
        if(logs.get_active_severity() == message_severities::error){
            //current_mode->request_state(Mode::States::IDLE); // stop the current mode
        }
        else if(logs.get_active_severity() == message_severities::critical){
            //current_mode->request_state(Mode::States::IDLE); // stop the current mode
            critical_shutdown();
        }

        // run additional mode functions
        current_mode->default_run();
        current_mode->run();
    }
}

void device::update(){

    if(!Comm.is_ok()){
        logs.add((uint32_t)communication_messages::timeout_error); // communication timeout error, this will trigger the entire system to shutdown
    }

    update_leds();
}

void device::update_leds(){
    if(Comm.is_ok()){
        UserIO.set_led_state(UserIO.blink_fast);
    }
    else{
        UserIO.set_led_state(UserIO.blink_slow);
    }
}

void device::critical_shutdown(){
}

void device::SysTick_Handler(void){
    sysTick_flag = true;
}

void device::flagged_sysTick(void){
    UserIO.SysTick_Handler();
    Fans.SysTick_Handler();

    update();

    current_mode->default_systick_handler();
    current_mode->systick_handler();

    sysTick_flag = false;
}

void device::TIM2_IRQHandler(void){
    TIM2->SR &= ~TIM_SR_UIF; // Clear the update interrupt flag
    tim2_flag = true;
}

void device::TIM5_IRQHandler(void){
    TIM5->SR &= ~TIM_SR_UIF; // Clear the update interrupt flag
    tim5_flag = true;
}

void device::flagged_tim2(void){
    Comm.TIM2_IRQHandler();
    tim2_flag = false;
}

void device::flagged_tim5(void){
    Comm.TIM5_IRQHandler();
    tim5_flag = false;
}


void device::DMA2_Stream1_IRQHandler(void){
    Comm.dma_stream1_interrupt_handler();
    dma2_stream1_flag = true;
}

void device::flagged_dma2_stream1(void){
    dma2_stream1_flag = false;
}

void device::TIM1_UP_TIM10_IRQHandler(void){
    if (TIM1->SR & TIM_SR_UIF) { // Check if update interrupt flag is set
        TIM1->SR &= ~TIM_SR_UIF; // Clear update interrupt flag
        tim1_update_missed |= tim1_up_tim10_flag;
        tim1_up_tim10_flag = true;
    }
}

void device::flagged_tim1_up_tim10(void){
    Comm.update_timeout();
    current_mode->flagged_tim1_up_tim10();
    watchdog_reload();
    tim1_up_tim10_flag = false;
    tim1_update_missed = false;
}

void device::USART6_IRQHandler(void){   // this handler is not flagged since it needs to be called immediately to ensure lowest jitter
    Comm.usart6_interrupt_handler();
}

device::IRQ device::missed_irq = IRQ::NONE;
void device::missed_irq_handler(IRQ irq){
    missed_irq = irq;
}