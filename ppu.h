#pragma once
#include <cstdint>
#include <array>

class CPU;
class ROM;

struct Sprite {
    uint8_t x;
    uint8_t y;
    uint8_t id;
    uint8_t attrib;
};

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
    uint8_t tileid;
    uint8_t tileattrib;
    uint8_t tilelsb;
    uint8_t tilemsb;
    uint16_t patlow;
    uint16_t pathigh;
    uint16_t attriblow;
    uint16_t attribhigh;

    Sprite sprites[8];
    uint8_t sprite_count;
    uint8_t sprite_patlow[8];
    uint8_t sprite_pathigh[8];
    bool zero_hit;

    bool w;
    CPU *cpu;
    ROM *rom;
    
    std::array<std::array<uint8_t,0x400>,2> nametables;
    std::array<uint8_t,32> palette;

    std::array<uint8_t,256> OAM;

    void write_oam_mem(uint8_t addr,uint8_t data);
    uint8_t read_oam_mem(uint8_t addr);
    void write_vram_mem(uint16_t addr,uint8_t data);
    uint8_t read_vram_mem(uint16_t addr);
    void inc_scroll_x();
    void inc_scroll_y();
    void transfer_addr_x();
    void transfer_addr_y();
    void load_shifters();
    void update_shifters();
    void render();
        
    public:
    int scanline;
    int cycle;
    bool framedone;
    uint32_t screen[256*240];

    PPU();
    void cpu_write(uint16_t addr,uint8_t data);
    uint8_t cpu_read(uint16_t addr);
    void attach_cpu(CPU *c);
    void attach_rom(ROM *r);
    void handle_oamdma(const std::array<uint8_t, 256>& dma_data);
    void step();
};