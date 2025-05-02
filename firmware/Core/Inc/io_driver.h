#pragma once

#include "stm32f413xx.h"
#include "device_descriptor.h"

/*
handle all IO related to controlling the estop board
*/

class estop_io {
    public:
        void init();

        struct inputs {
            bool isolation_contactor_int = 0;
            bool main_contactor_int = 0;
            bool precharge_contactor_int = 0;
            bool isolation_contactor_ext = 0;
            bool main_contactor_ext = 0;
            bool precharge_contactor_ext = 0;
            bool chain_A_relay = 0;
            bool chain_B_relay = 0;
            bool chain_A = 0;
            bool chain_B = 0;
            bool deadman_switch = 0;

            bool brake_A = 0;
            bool brake_B = 0;
            bool brake_1 = 0;
            bool brake_2 = 0;
            bool brake_3 = 0;
            bool brake_4 = 0;
            bool brake_5 = 0;
            bool brake_6 = 0;

            bool aux_1 = 0;
            bool aux_2 = 0;
            bool aux_3 = 0;
            bool aux_4 = 0;
            bool aux_5 = 0;
            bool aux_6 = 0;
            bool aux_7 = 0;
            bool aux_8 = 0;
            bool aux_9 = 0;
            bool aux_10 = 0;
            bool aux_11 = 0;
            bool aux_12 = 0;
        } inputs;

        struct outputs {
            bool chain_relay = 0;
            bool isolation_contactor = 0;
            bool main_contactor = 0;
            bool precharge_contactor = 0;

            bool brake_1 = 0;
            bool brake_2 = 0;
            bool brake_3 = 0;
            bool brake_4 = 0;
            bool brake_5 = 0;
            bool brake_6 = 0;
        } outputs;

        void read_inputs(void);
        void write_outputs(void);

    private:

        inline void load_input_data(void);
        inline void tick_input_clock(void);

        inline void tick_output_clock(void);
        inline void write_output_data(void);


        inline bool get_input_data(void);
        inline void set_output_data(bool data);
        inline void set_output_enable(bool enable);
        inline void set_output_clock(bool clock);
        inline void set_input_clock(bool clock);
        inline void set_input_load(bool load);
        inline void set_output_write(bool write);
};