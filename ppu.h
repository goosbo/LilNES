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
    
    std::array<uint8_t,16384> VRAM;
    void write_vram_mem(uint16_t addr,uint8_t data);
    uint8_t read_vram_mem(uint16_t addr);

    public:
    PPU();
    void cpu_write(uint16_t addr,uint8_t data);
    uint8_t cpu_read(uint16_t addr);
    void attach_cpu(CPU *c);
    void handle_oamdma(uint8_t data);
};