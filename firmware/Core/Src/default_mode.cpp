#include "default_mode.h"

Mode::Mode(logging* logs, device_struct** comm_vars){
    this->logs = logs;
    this->comm_vars = comm_vars;
}

void Mode::default_systick_handler(void){
}

void Mode::default_run(void){
}

void Mode::flagged_tim1_up_tim10(){
    
}