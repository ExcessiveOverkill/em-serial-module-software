/**
  ******************************************************************************
  * @file           : user_io.h
  * @brief          : Header for user_io.cpp file.
  *                   user_io files contain the requred functions to setup and use the indicator LED and DIP switches.
  ******************************************************************************
**/

#pragma once

#include "stm32f413xx.h"

class user_io{
    private:
        uint8_t blink_state = 0b10000;  //states of each led mode  (bit 0:off 1:slow, 2:medium, 3:fast 4:on)

        uint32_t led_state = 0;
        uint8_t led_mode = 0;


    public:

        enum led_mode_enum{
          off = 1,
          blink_slow = 2,
          blink_medium = 4,
          blink_fast = 8,
          on = 16
        };
        
        user_io();

        const uint64_t* micros = nullptr; // pointer to the global microseconds variable

        void init(void);
        
        uint8_t get_switch_states(void);

        void set_led_state(uint32_t led_mode_);

        void SysTick_Handler(void);

};