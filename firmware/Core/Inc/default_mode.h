#include "logging.h"
#include "device_descriptor.h"
#include <stdint.h>

#pragma once

// base class that all modes inherit
class Mode {
    public:

        Mode(logging* logs, device_struct** comm_vars);

        void default_systick_handler(void);
        virtual void systick_handler(void){}

        void default_run(void);
        virtual void run(void){}

        void flagged_tim1_up_tim10();

        const uint64_t* micros = nullptr; // pointer to the global microseconds variable

    private:

    protected:

        logging* logs;
        device_struct** comm_vars;

};