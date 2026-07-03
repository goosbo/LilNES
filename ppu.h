#pragma once
#include <cstdint>
#include <array>

class CPU;

class PPU{
    private:
    uint8_t ctrl;
    uint8_t mask;
    uint8_t status;
    uint8_t oamaddr;
    uint8_t oamdata;
    uint8_t scroll;
    uint8_t ppuaddr;
    uint8_t ppudata;
    uint8_t oamdma;
    uint16_t v;
    uint16_t t;
    uint8_t x;
    uint8_t databuffer;


    bool w;
    CPU *cpu;
    
    std::array<std::array<uint8_t,0x1000>,2> pattern_tables;
    std::array<std::array<uint8_t,0x400>,2> nametables;
    std::array<uint8_t,32> palette;

    std::array<uint8_t,256> OAM;

    void write_oam_mem(uint8_t addr,uint8_t data);
    uint8_t read_oam_mem(uint8_t addr);
    void write_vram_mem(uint16_t addr,uint8_t data);
    uint8_t read_vram_mem(uint16_t addr);

    public:
    int scanline = 0;
    int cycle = 0;
    PPU();
    void cpu_write(uint16_t addr,uint8_t data);
    uint8_t cpu_read(uint16_t addr);
    void attach_cpu(CPU *c);
    void handle_oamdma(const std::array<uint8_t, 256>& dma_data);
    void step();
};