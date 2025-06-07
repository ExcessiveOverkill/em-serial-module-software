/**
  ******************************************************************************
  * @file           : uart.h
  * @brief          : Header for uart.cpp file.
  *                   uart files allow the module to communicate with the 3d mouse stm32H7
  ******************************************************************************
**/

#pragma once

#include "stm32f413xx.h"
#include <memory.h>
#include "device_descriptor.h"
#include "logging.h"

class uart{
    public:
        struct data{
            int32_t axis_0 = 0;
            int32_t axis_1 = 0;
            int32_t axis_2 = 0;
            int32_t axis_3 = 0;
            int32_t axis_4 = 0;
            int32_t axis_5 = 0;
            bool button = 0;
        };

    private:
        logging* logs;
        const uint64_t* micros = nullptr; // pointer to the global microseconds variable

        #define PACKET_LENGTH 24+1+16

        uint8_t rx_buffer[PACKET_LENGTH];
        data unpacked_data;
        bool new_data = false;

        bool receive_complete = false;
        bool receive_started = false;

        uint64_t last_update_time_us = 0;

        void set_rx_packet_length(void);

        void start_receive(void);

        inline void restart_rx_dma(void);
        inline void clear_rx_idle_flag(void);
        inline void reset_timeout(void);

        void interpret_rx_packet(void);

        int8_t verify_rx_packet(void);

    public:
        uart(logging* logs);
        void init(void);

        bool new_data_available(void);
        bool timed_out(void);

        data get_data(void);

        void flagged_uart4_interrupt_handler(void);
        void flagged_dma1_stream2_interrupt_handler(void);

};