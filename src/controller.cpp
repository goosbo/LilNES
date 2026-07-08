#include "controller.h"

Controller::Controller() {
    state = 0;
    shiftreg = 0;
    strobe = false;
}

void Controller::update(uint8_t cstate){
    state = cstate;
}

void Controller::write(uint8_t data){
    if(strobe && ((data&1) == 0)) shiftreg = state;
    strobe = data & 1;
}

uint8_t Controller::read(){
    if(strobe)return state&1;
    uint8_t data = shiftreg&1;
    shiftreg >>=1;
    return data;
}