#include "uart.h"

uart::uart(logging* logs){
    this->logs = logs;
}

void uart::init(){
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;     // enable uart4 clock (50 MHz)

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;    // Enable GPIOA  Clock
	GPIOA->MODER |= GPIO_MODER_MODER0_1;	// set PA0 (TX) as alternate function
	GPIOA->MODER |= GPIO_MODER_MODER1_1;	// set PA1 (RX) as alternate function
	GPIOA->OSPEEDR |= 0b10 << GPIO_OSPEEDR_OSPEED0_Pos;	// set TX as fast speed output
	GPIOA->AFR[0] |= 8 << GPIO_AFRL_AFSEL0_Pos;	// set PA0 alternate function to 8 (UART4)
	GPIOA->AFR[0] |= 8 << GPIO_AFRL_AFSEL1_Pos;	// set PA1 alternate function to 8 (UART4)


    UART4->CR1 |= USART_CR1_UE;	// Enable USART6

    // set USARTDIV to 27.125 (baudrate of ~115200 bit/s, (115207 exact))
	UART4->BRR |= 27 << USART_BRR_DIV_Mantissa_Pos;	// Program Baud rate Mantissa
	UART4->BRR |= 2 << USART_BRR_DIV_Fraction;	// Program Baud rate fraction

	UART4->CR1 |= USART_CR1_IDLEIE;	// enable idle detect interrupt
	// UART4->CR1 |= USART_CR1_TCIE;	// enable transmission complete interrupt

	UART4->CR1 |= USART_CR1_RE;	// Enable Receiver

	// setup DMA1 stream2 for UART4 RX
	// (not implemented) setup DMA1 stream4 for UART4 TX

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; // Enable DMA clock

    DMA1_Stream2->CR &= ~(DMA_SxCR_EN); // Disable DMA stream

    while(DMA1_Stream2->CR & (DMA_SxCR_EN_Msk)){}    // Wait for stream to disable

	DMA1_Stream2->CR |= 4 << DMA_SxCR_CHSEL_Pos;	// select channel 4 for rx stream

    DMA1_Stream2->CR |= 0b00 << DMA_SxCR_PSIZE_Pos;     // set peripheral data size to 8bit (default)
    DMA1_Stream2->CR |= 0b00 << DMA_SxCR_MSIZE_Pos;     // set memory data size to 8bit (default)

    DMA1_Stream2->CR |= DMA_SxCR_MINC;      // auto-increment memory address (by set memory size)

    DMA1_Stream2->PAR = (uint32_t)&UART4->DR;        // use UART4 data register (rx)

    DMA1_Stream2->M0AR = (uint32_t)&rx_buffer;   // use rx_data as the memory for RX data

    DMA1_Stream2->CR |= 0b00 << DMA_SxCR_PL_Pos;    // low priority

    //DMA1_Stream2->FCR |= DMA_SxFCR_DMDIS;   // Disable direct transfer mode

    //DMA1_Stream2->FCR |= 0b10 << DMA_SxFCR_FTH_Pos;     // Set FIFO threshold to full

    set_rx_packet_length();

    DMA1_Stream2->CR |= DMA_SxCR_TCIE;  // Trigger interrupt when RX transfer to memory is complete

	// (not implemented) UART4->CR3 |= USART_CR3_DMAT;	// Enable DMA for tx 

    NVIC_EnableIRQ(DMA1_Stream2_IRQn);  // Configure NVIC for DMA RX Interrupt
	NVIC_SetPriority(DMA1_Stream2_IRQn, 8);
	NVIC_SetPriority(UART4_IRQn, 8);
	NVIC_EnableIRQ(UART4_IRQn);

    DMA1_Stream2->CR |= DMA_SxCR_EN; // Enable DMA RX stream

    start_receive();	// start receiving data

}

void uart::set_rx_packet_length(){

	bool stream_enabled = DMA1_Stream2->CR & (DMA_SxCR_EN_Msk);

	if(stream_enabled){
		DMA1_Stream2->CR &= ~(DMA_SxCR_EN); // Disable DMA stream
		while(DMA1_Stream2->CR & (DMA_SxCR_EN_Msk));    // Wait for stream to disable
	}

	DMA1_Stream2->NDTR = PACKET_LENGTH;	// set number of 8 bit transfer cycles

	if(stream_enabled){
		DMA1_Stream2->CR |= DMA_SxCR_EN; // Enable DMA stream
	}
}

void uart::clear_rx_idle_flag(){
	[[maybe_unused]] volatile uint32_t temp = UART4->SR;	// read SR register
	temp = UART4->DR;	// read DR register
}

void uart::restart_rx_dma(){
	receive_started = false;
	receive_complete = false;
	DMA1_Stream2->CR &= ~(DMA_SxCR_EN); // Disable DMA stream
	while(DMA1_Stream2->CR & (DMA_SxCR_EN_Msk));    // Wait for stream to disable
	DMA1->LIFCR |= DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;	// clear any interrupt flags
	DMA1_Stream2->NDTR = PACKET_LENGTH;	// set number of 8 bit transfer cycles
	DMA1_Stream2->CR |= DMA_SxCR_EN; // Enable DMA stream
}

void uart::start_receive(){
    clear_rx_idle_flag();
	restart_rx_dma();
	receive_complete = false;
	receive_started = true;
	UART4->CR3 |= USART_CR3_DMAR;	// Enable DMA for rx
}

int8_t uart::verify_rx_packet(){

    // check header
    const uint8_t header = 0xff;    // header byte
    for(uint8_t i = 0; i < 16; i++){
        if(rx_buffer[i] != header){    // check if the header is correct
            return -1;    // header bad
        }
    }

    // check button byte
    if(rx_buffer[PACKET_LENGTH-1] & 0xfe){
        return -1;    // button byte bad
    }

    return 0;    // packet good

}

void uart::flagged_uart4_interrupt_handler(){
    if(UART4->SR & USART_SR_IDLE_Msk){		// RX IDLE state detected (incomming transmission over)
        bool receive_complete_ = receive_complete;
        
		int8_t result = verify_rx_packet();
		if(!receive_complete_){	// if not a complete packet, ignore
			// do nothing
		}
        else if(result == 0){	// valid packet
            interpret_rx_packet();
            reset_timeout();
        }

        // clear buffer so it can't be used again
        memset(rx_buffer, 0, PACKET_LENGTH);

        start_receive();
    }
}

void uart::flagged_dma1_stream2_interrupt_handler(){
    if(DMA1->LISR & DMA_LISR_TCIF2){
        UART4->CR3 &= ~USART_CR3_DMAR;	// Disable DMA for rx
		receive_complete = true;
		receive_started = false;
        DMA1->LIFCR |= DMA_LIFCR_CTCIF2;	// clear trasfer finished flag
    }
}

void uart::interpret_rx_packet(){

    unpacked_data.axis_0 = *(int32_t*)&rx_buffer[16+0];
    unpacked_data.axis_1 = *(int32_t*)&rx_buffer[16+4];
    unpacked_data.axis_2 = *(int32_t*)&rx_buffer[16+8];
    unpacked_data.axis_3 = *(int32_t*)&rx_buffer[16+12];
    unpacked_data.axis_4 = *(int32_t*)&rx_buffer[16+16];
    unpacked_data.axis_5 = *(int32_t*)&rx_buffer[16+20];
    unpacked_data.button = rx_buffer[16+24] & 0x01;

    new_data = true;

}

void uart::reset_timeout(){
    last_update_time_us = *micros;
}

bool uart::new_data_available(){
    auto new_data_ = new_data;
    new_data = false;
    return new_data_;
}

bool uart::timed_out(){
    return (*micros - last_update_time_us) > 50000;    // 50ms timeout
}

uart::data uart::get_data(){
    new_data = false;
    return unpacked_data;
}