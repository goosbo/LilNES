#pragma once
#include <cstdint>
#include <array>
#include <rom.h>
class MemoryBus{
    private:
    std::array<uint8_t,2048> RAM;
    std::array<uint8_t,256> stk;
    ROM *rom;

    public:
    MemoryBus();
    ~MemoryBus();

    void attach_rom(ROM &rom);
    void write_mem(uint16_t addr, uint8_t data);
    uint8_t read_mem(uint16_t addr);

    uint8_t pop_stk(uint8_t sp);
    void push_stk(uint8_t sp, uint8_t data);
};