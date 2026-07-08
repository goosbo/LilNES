#pragma once
#include <cstdint>

enum key{
    keyA = 1,
    keyB = 2,
    Select = 4,
    Start = 8,
    Up = 0x10,
    Down = 0x20,
    Left = 0x40,
    Right = 0x80
};

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