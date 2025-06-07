#include "default_mode.h"

Mode::Mode(logging* logs, estop_io* io, fans* fans_, device_struct** comm_vars){
    this->logs = logs;
    this->EstopIO = io;
    this->Fans = fans_;
    this->comm_vars = comm_vars;
}

void Mode::default_systick_handler(void){
}

void Mode::default_run(void){
}

void Mode::flagged_tim1_up_tim10(){
    EstopIO->read_inputs();

    // optional invert ext contactor feedback
    EstopIO->inputs.isolation_contactor_ext = (*comm_vars)->isolation_contactor_external_feedback_polarity ? !EstopIO->inputs.isolation_contactor_ext : EstopIO->inputs.isolation_contactor_ext;
    EstopIO->inputs.main_contactor_ext = (*comm_vars)->main_contactor_external_feedback_polarity ? !EstopIO->inputs.main_contactor_ext : EstopIO->inputs.main_contactor_ext;
    EstopIO->inputs.precharge_contactor_ext = (*comm_vars)->precharge_contactor_external_feedback_polarity ? !EstopIO->inputs.precharge_contactor_ext : EstopIO->inputs.precharge_contactor_ext;


    run_checks(); // run all validation checks

    run_logic(); // run control logic

    run_delays(); // run contact all delays

    update_feedback(); // update feedback from the system to the controller

    EstopIO->write_outputs();
}

void Mode::run_checks(void){
    // run all validation checks

    // check for stuck on brake outputs
    if(EstopIO->inputs.brake_1 && !EstopIO->outputs.brake_1){
        logs->log_persistent_active((uint32_t)checks_messages::brake_1_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_1_powered_when_off);
    }
    if(EstopIO->inputs.brake_2 && !EstopIO->outputs.brake_2){
        logs->log_persistent_active((uint32_t)checks_messages::brake_2_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_2_powered_when_off);
    }
    if(EstopIO->inputs.brake_3 && !EstopIO->outputs.brake_3){
        logs->log_persistent_active((uint32_t)checks_messages::brake_3_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_3_powered_when_off);
    }
    if(EstopIO->inputs.brake_4 && !EstopIO->outputs.brake_4){
        logs->log_persistent_active((uint32_t)checks_messages::brake_4_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_4_powered_when_off);
    }
    if(EstopIO->inputs.brake_5 && !EstopIO->outputs.brake_5){
        logs->log_persistent_active((uint32_t)checks_messages::brake_5_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_5_powered_when_off);
    }
    if(EstopIO->inputs.brake_6 && !EstopIO->outputs.brake_6){
        logs->log_persistent_active((uint32_t)checks_messages::brake_6_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_6_powered_when_off);
    }

    // stuck off brake outputs
    if(!EstopIO->inputs.brake_1 && EstopIO->outputs.brake_1){
        logs->log_persistent_active((uint32_t)checks_messages::brake_1_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_1_not_powered_when_on);
    }
    if(!EstopIO->inputs.brake_2 && EstopIO->outputs.brake_2){
        logs->log_persistent_active((uint32_t)checks_messages::brake_2_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_2_not_powered_when_on);
    }
    if(!EstopIO->inputs.brake_3 && EstopIO->outputs.brake_3){
        logs->log_persistent_active((uint32_t)checks_messages::brake_3_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_3_not_powered_when_on);
    }
    if(!EstopIO->inputs.brake_4 && EstopIO->outputs.brake_4){
        logs->log_persistent_active((uint32_t)checks_messages::brake_4_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_4_not_powered_when_on);
    }
    if(!EstopIO->inputs.brake_5 && EstopIO->outputs.brake_5){
        logs->log_persistent_active((uint32_t)checks_messages::brake_5_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_5_not_powered_when_on);
    }
    if(!EstopIO->inputs.brake_6 && EstopIO->outputs.brake_6){
        logs->log_persistent_active((uint32_t)checks_messages::brake_6_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_6_not_powered_when_on);
    }

    // main brake relay stuck on
    if((EstopIO->inputs.brake_A && !EstopIO->outputs.chain_relay) || (EstopIO->inputs.brake_B && !EstopIO->outputs.chain_relay)){
        logs->log_persistent_active((uint32_t)checks_messages::brake_main_relay_stuck_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_main_relay_stuck_on);
    }

    // brake supply missing
    if(!(EstopIO->inputs.brake_A && EstopIO->inputs.brake_B) && EstopIO->outputs.chain_relay && EstopIO->inputs.chain_A && EstopIO->inputs.chain_B){
        logs->log_persistent_active((uint32_t)checks_messages::brake_supply_missing);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::brake_supply_missing);
    }

    // estop chain mismatch
    if(EstopIO->inputs.chain_A != EstopIO->inputs.chain_B){
        logs->log_persistent_active((uint32_t)checks_messages::estop_chain_mismatch);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::estop_chain_mismatch);
    }

    // estop chain relay stuck on
    if((EstopIO->inputs.chain_A_relay || EstopIO->inputs.chain_B_relay) && !EstopIO->outputs.chain_relay && !(*comm_vars)->STO_only){
        logs->log_persistent_active((uint32_t)checks_messages::chain_relay_stuck_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::chain_relay_stuck_on);
    }

    // 24v supply missing
    if(EstopIO->outputs.chain_relay && (EstopIO->inputs.chain_A && EstopIO->inputs.chain_B) && (!EstopIO->inputs.chain_A_relay || !EstopIO->inputs.chain_B_relay)){
        logs->log_persistent_active((uint32_t)checks_messages::contactor_supply_missing);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::contactor_supply_missing);
    }

    // contactors powered when off
    if(EstopIO->inputs.isolation_contactor_int && !EstopIO->outputs.isolation_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::isolation_contactor_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::isolation_contactor_powered_when_off);
    }
    if(EstopIO->inputs.main_contactor_int && !EstopIO->outputs.main_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::main_contactor_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::main_contactor_powered_when_off);
    }
    if(EstopIO->inputs.precharge_contactor_int && !EstopIO->outputs.precharge_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::precharge_contactor_powered_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::precharge_contactor_powered_when_off);
    }

    // contactors not powered when on
    if(!EstopIO->inputs.isolation_contactor_int && EstopIO->outputs.isolation_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::isolation_contactor_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::isolation_contactor_not_powered_when_on);
    }
    if(!EstopIO->inputs.main_contactor_int && EstopIO->outputs.main_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::main_contactor_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::main_contactor_not_powered_when_on);
    }
    if(!EstopIO->inputs.precharge_contactor_int && EstopIO->outputs.precharge_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::precharge_contactor_not_powered_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::precharge_contactor_not_powered_when_on);
    }

    // external contactors closed when off
    if(EstopIO->inputs.isolation_contactor_ext && !EstopIO->outputs.isolation_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::isolation_contactor_feedback_preset_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::isolation_contactor_feedback_preset_when_off);
    }
    if(EstopIO->inputs.main_contactor_ext && !EstopIO->outputs.main_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::main_contactor_feedback_preset_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::main_contactor_feedback_preset_when_off);
    }
    if(EstopIO->inputs.precharge_contactor_ext && !EstopIO->outputs.precharge_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::precharge_contactor_feedback_preset_when_off);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::precharge_contactor_feedback_preset_when_off);
    }

    // external contactors open when on
    if(!EstopIO->inputs.isolation_contactor_ext && EstopIO->outputs.isolation_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::isolation_contactor_feedback_missing_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::isolation_contactor_feedback_missing_when_on);
    }
    if(!EstopIO->inputs.main_contactor_ext && EstopIO->outputs.main_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::main_contactor_feedback_missing_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::main_contactor_feedback_missing_when_on);
    }
    if(!EstopIO->inputs.precharge_contactor_ext && EstopIO->outputs.precharge_contactor){
        logs->log_persistent_active((uint32_t)checks_messages::precharge_contactor_feedback_missing_when_on);
    }
    else{
        logs->log_persistent_inactive((uint32_t)checks_messages::precharge_contactor_feedback_missing_when_on);
    }
}

void Mode::run_logic(void){
    // run all control logic

    EstopIO->outputs.chain_relay = 0; // default to off, this will be turned on if it is needed by the logic

    // if in error state, disable all outputs
    if(
        logs->get_active_severity() == message_severities::error ||
        logs->get_active_severity() == message_severities::critical
    ){
        EstopIO->outputs.chain_relay = 0;
        desired_brake_state[0] = 0;
        desired_brake_state[1] = 0;
        desired_brake_state[2] = 0;
        desired_brake_state[3] = 0;
        desired_brake_state[4] = 0;
        desired_brake_state[5] = 0;
        desired_contactor_state[0] = 0;
        desired_contactor_state[1] = 0;
        desired_contactor_state[2] = 0;
        return;
    }

    bool ext_chain_ok = EstopIO->inputs.chain_A && EstopIO->inputs.chain_B;

    // if chain is broken, disable according to the STO config
    if(!ext_chain_ok){
        EstopIO->outputs.chain_relay = 0;
        desired_brake_state[0] = 0;
        desired_brake_state[1] = 0;
        desired_brake_state[2] = 0;
        desired_brake_state[3] = 0;
        desired_brake_state[4] = 0;
        desired_brake_state[5] = 0;

        if((*comm_vars)->STO_only){
            // ok to leave the contactors on if in STO only mode
        }
        else{
            // disable all contactors if in normal mode
            desired_contactor_state[0] = 0; // isolation contactor
            desired_contactor_state[1] = 0; // main contactor
            desired_contactor_state[2] = 0; // precharge contactor
            return; // absolutly nothing can be powered in this case
        }
    }
    else{   // chain ok

        // if we make it to here, we are in a valid state where we can apply power if desired

        if((*comm_vars)->enable_power && compare_aux_inputs((*comm_vars)->enable_required_inputs)){
            // allowed to enable power
            if(!(*comm_vars)->STO_only){    // we require the chain for contactor power
                EstopIO->outputs.chain_relay = 1; // enable the chain relay
                if(EstopIO->inputs.chain_A_relay && EstopIO->inputs.chain_B_relay){ // chain fully powered
                    desired_contactor_state[0] = 1; // isolation contactor on
                }
            }
            else{   // we are in STO only mode, we can power the contactors directly
                desired_contactor_state[0] = 1; // isolation contactor on
            }
            
            if(EstopIO->inputs.isolation_contactor_ext && EstopIO->inputs.isolation_contactor_int){ // isolation contactor powered and closed
                if(desired_contactor_state[2] == 0 && desired_contactor_state[1] == 0){ // precharge and main contactors are off
                    desired_contactor_state[2] = 1; // start precharge
                    precharge_time_min = -1; // reset precharge time
                    precharge_time_max = -1; // reset precharge time
                }

                if(EstopIO->inputs.main_contactor_ext && EstopIO->inputs.main_contactor_int){ // main contactor powered and closed
                    if(*micros > precharge_overlap_time){ // precharge overlap time has passed
                        desired_contactor_state[2] = 0; // disable precharge contactor (don't need it on once main is on)
                    }
                }
                else{
                    precharge_overlap_time = *micros + PRECHARGE_OVERLAP_TIME*1000;
                    if(EstopIO->inputs.precharge_contactor_ext && EstopIO->inputs.precharge_contactor_int){ // precharge contactor powered and closed
                        if(*micros > precharge_time_min){ // precharge time has passed
                            if((*comm_vars)->precharge_done){   // controller signaled precharge is done
                                desired_contactor_state[1] = 1; // enable main contactor
                            }
                            else if(*micros > precharge_time_max){
                                logs->add((uint32_t)checks_messages::precharge_timeout); // precharge timeout
                            }
                        }
                    }
                    else{
                        precharge_time_min = *micros + (*comm_vars)->precharge_delay_min*1000;
                        precharge_time_max = *micros + (*comm_vars)->precharge_delay_max*1000;
                    }
                }   
            }
        }
        else{
            // disable all contactors
            desired_contactor_state[0] = 0; // isolation contactor
            desired_contactor_state[1] = 0; // main contactor
            desired_contactor_state[2] = 0; // precharge contactor
        }


        // enable brakes if required

        // if any are on, enable the chain relay and brake power supply (not yet the actual brakes)
        if((*comm_vars)->enable_brakes){
            EstopIO->outputs.chain_relay = 1;
            if(EstopIO->inputs.chain_A_relay && EstopIO->inputs.chain_B_relay && EstopIO->inputs.brake_A && EstopIO->inputs.brake_B){ // chain and brake supply powered
                
                // enable the brakes according to the requested
                bool enables[6] = {0, 0, 0, 0, 0, 0};
                enables[0] = (*comm_vars)->enable_brakes & (1<<0);
                enables[1] = (*comm_vars)->enable_brakes & (1<<1);
                enables[2] = (*comm_vars)->enable_brakes & (1<<2);
                enables[3] = (*comm_vars)->enable_brakes & (1<<3);
                enables[4] = (*comm_vars)->enable_brakes & (1<<4);
                enables[5] = (*comm_vars)->enable_brakes & (1<<5);

                desired_brake_state[0] = 0; // disable all brakes
                desired_brake_state[1] = 0;
                desired_brake_state[2] = 0;
                desired_brake_state[3] = 0;
                desired_brake_state[4] = 0;
                desired_brake_state[5] = 0;

                if(enables[0] && compare_aux_inputs((*comm_vars)->brake_1_required_inputs)){
                    desired_brake_state[0] = 1; // enable brake 1
                }
                if(enables[1] && compare_aux_inputs((*comm_vars)->brake_2_required_inputs)){
                    desired_brake_state[1] = 1; // enable brake 2
                }
                if(enables[2] && compare_aux_inputs((*comm_vars)->brake_3_required_inputs)){
                    desired_brake_state[2] = 1; // enable brake 3
                }
                if(enables[3] && compare_aux_inputs((*comm_vars)->brake_4_required_inputs)){
                    desired_brake_state[3] = 1; // enable brake 4
                }
                if(enables[4] && compare_aux_inputs((*comm_vars)->brake_5_required_inputs)){
                    desired_brake_state[4] = 1; // enable brake 5
                }
                if(enables[5] && compare_aux_inputs((*comm_vars)->brake_6_required_inputs)){
                    desired_brake_state[5] = 1; // enable brake 6
                }
            }
        }
    }
}

void Mode::run_delays(void){
    // run all delays for contactors and brakes

    // update last on time
    if(EstopIO->outputs.isolation_contactor){
        last_contactor_on_time[0] = *micros;
    }
    if(EstopIO->outputs.main_contactor){
        last_contactor_on_time[1] = *micros;
    }
    if(EstopIO->outputs.precharge_contactor){
        last_contactor_on_time[2] = *micros;
    }
    if(EstopIO->outputs.brake_1){
        last_brake_on_time[0] = *micros;
    }
    if(EstopIO->outputs.brake_2){
        last_brake_on_time[1] = *micros;
    }
    if(EstopIO->outputs.brake_3){
        last_brake_on_time[2] = *micros;
    }
    if(EstopIO->outputs.brake_4){
        last_brake_on_time[3] = *micros;
    }
    if(EstopIO->outputs.brake_5){
        last_brake_on_time[4] = *micros;
    }
    if(EstopIO->outputs.brake_6){
        last_brake_on_time[5] = *micros;
    }

    // check if the requested state is different from the last state
    // if swithing off, there is no delay
    // if switching on, the delay must have passed
    if(EstopIO->outputs.isolation_contactor != desired_contactor_state[0]){
        if(desired_contactor_state[0] == 1){
            if((*micros - last_contactor_on_time[0]) > CONTACTOR_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.isolation_contactor = desired_contactor_state[0];
            }
        }
        else{
            EstopIO->outputs.isolation_contactor = desired_contactor_state[0];
        }
    }
    if(EstopIO->outputs.main_contactor != desired_contactor_state[1]){
        if(desired_contactor_state[1] == 1){
            if((*micros - last_contactor_on_time[1]) > CONTACTOR_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.main_contactor = desired_contactor_state[1];
            }
        }
        else{
            EstopIO->outputs.main_contactor = desired_contactor_state[1];
        }
    }
    if(EstopIO->outputs.precharge_contactor != desired_contactor_state[2]){
        if(desired_contactor_state[2] == 1){
            if((*micros - last_contactor_on_time[2]) > CONTACTOR_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.precharge_contactor = desired_contactor_state[2];
            }
        }
        else{
            EstopIO->outputs.precharge_contactor = desired_contactor_state[2];
        }
    }

    // brakes
    if(EstopIO->outputs.brake_1 != desired_brake_state[0]){
        if(desired_brake_state[0] == 1){
            if((*micros - last_brake_on_time[0]) > BRAKE_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.brake_1 = desired_brake_state[0];
            }
        }
        else{
            EstopIO->outputs.brake_1 = desired_brake_state[0];
        }
    }
    if(EstopIO->outputs.brake_2 != desired_brake_state[1]){
        if(desired_brake_state[1] == 1){
            if((*micros - last_brake_on_time[1]) > BRAKE_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.brake_2 = desired_brake_state[1];
            }
        }
        else{
            EstopIO->outputs.brake_2 = desired_brake_state[1];
        }
    }
    if(EstopIO->outputs.brake_3 != desired_brake_state[2]){
        if(desired_brake_state[2] == 1){
            if((*micros - last_brake_on_time[2]) > BRAKE_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.brake_3 = desired_brake_state[2];
            }
        }
        else{
            EstopIO->outputs.brake_3 = desired_brake_state[2];
        }
    }
    if(EstopIO->outputs.brake_4 != desired_brake_state[3]){
        if(desired_brake_state[3] == 1){
            if((*micros - last_brake_on_time[3]) > BRAKE_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.brake_4 = desired_brake_state[3];
            }
        }
        else{
            EstopIO->outputs.brake_4 = desired_brake_state[3];
        }
    }
    if(EstopIO->outputs.brake_5 != desired_brake_state[4]){
        if(desired_brake_state[4] == 1){
            if((*micros - last_brake_on_time[4]) > BRAKE_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.brake_5 = desired_brake_state[4];
            }
        }
        else{
            EstopIO->outputs.brake_5 = desired_brake_state[4];
        }
    }
    if(EstopIO->outputs.brake_6 != desired_brake_state[5]){
        if(desired_brake_state[5] == 1){
            if((*micros - last_brake_on_time[5]) > BRAKE_MINIMUM_OFF_TIME*1000){
                EstopIO->outputs.brake_6 = desired_brake_state[5];
            }
        }
        else{
            EstopIO->outputs.brake_6 = desired_brake_state[5];
        }
    }
}

void Mode::update_feedback(void){
    (*comm_vars)->dms_ok = EstopIO->inputs.deadman_switch;

    uint8_t temp = 0;
    temp |= EstopIO->inputs.brake_1 << 0;
    temp |= EstopIO->inputs.brake_2 << 1;
    temp |= EstopIO->inputs.brake_3 << 2;
    temp |= EstopIO->inputs.brake_4 << 3;
    temp |= EstopIO->inputs.brake_5 << 4;
    temp |= EstopIO->inputs.brake_6 << 5;
    (*comm_vars)->brake_status = temp;

    (*comm_vars)->estop_chain_ok = EstopIO->inputs.chain_A && EstopIO->inputs.chain_B;

    (*comm_vars)->precharge_active = EstopIO->inputs.precharge_contactor_int;
    (*comm_vars)->main_power_active = EstopIO->inputs.main_contactor_int;
}

bool Mode::compare_aux_inputs(uint16_t required_inputs){
    // compare the inputs to the required inputs
    // returns true if all of the required inputs are present

    uint16_t inputs = 0;
    inputs |= EstopIO->inputs.aux_1 << 0;
    inputs |= EstopIO->inputs.aux_2 << 1;
    inputs |= EstopIO->inputs.aux_3 << 2;
    inputs |= EstopIO->inputs.aux_4 << 3;
    inputs |= EstopIO->inputs.aux_5 << 4;
    inputs |= EstopIO->inputs.aux_6 << 5;
    inputs |= EstopIO->inputs.aux_7 << 6;
    inputs |= EstopIO->inputs.aux_8 << 7;
    inputs |= EstopIO->inputs.aux_9 << 8;
    inputs |= EstopIO->inputs.aux_10 << 9;
    inputs |= EstopIO->inputs.aux_11 << 10;
    inputs |= EstopIO->inputs.aux_12 << 11;

    // ensure all required bits are set
    if((inputs & required_inputs) == required_inputs){
        return true; // all required inputs are present
    }
    else{
        return false; // not all required inputs are present
    }
}