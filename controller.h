#pragma once
#include <cstdint>

class Controller{
    private:
    uint8_t state;
    uint8_t shiftreg;
    bool strobe;

    public:
    Controller();
    void update(uint8_t cstate);
    void write(uint8_t data);
    uint8_t read();   
};