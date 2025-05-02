#include "logging.h"
#include "io_driver.h"
#include "device_descriptor.h"
#include <stdint.h>

#pragma once

// base class that all modes inherit
class Mode {
    public:

        Mode(logging* logs, estop_io* io, device_struct** comm_vars);

        void default_systick_handler(void);
        virtual void systick_handler(void){}

        void default_run(void);
        virtual void run(void){}

        void flagged_tim1_up_tim10();

        const uint64_t* micros = nullptr; // pointer to the global microseconds variable

    private:

        void run_checks(void); // run all validation checks

        void run_logic(void); // control logic

        void run_delays(void); // run all delays

        void update_feedback(void); // update feedback from the system

        uint64_t last_brake_on_time[6] = {0, 0, 0, 0, 0, 0}; // last time a brake was on
        uint64_t last_contactor_on_time[3] = {0, 0, 0}; // last time a contactor was on

        uint8_t desired_brake_state[6] = {0, 0, 0, 0, 0, 0}; // desired state of the brakes
        uint8_t desired_contactor_state[3] = {0, 0, 0}; // desired state of the contactors

        bool compare_aux_inputs(uint16_t required_inputs);

        uint64_t precharge_time = -1; // target power on time

    protected:

        logging* logs;
        estop_io* EstopIO;
        device_struct** comm_vars;

};