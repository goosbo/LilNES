#pragma once
#include <cstdint>
#include <vector>
#include <fstream>
#include <string>

struct ROM_header{
    char magic_bytes[4];
    uint8_t prg_size;
    uint8_t chr_size;
    uint8_t mapper1;
    uint8_t mapper2;
    uint8_t prg_ram_size;
    uint8_t tv_system1;
    uint8_t tv_system2;
    char padding[5];
};

enum mirroring_type{
    HORIZONTAL,
    VERTICAL
};

class ROM{
    private:
        std::vector<uint8_t> prg_rom;
        std::vector<uint8_t> chr_rom;
        std::vector<uint8_t> prg_ram;
        uint8_t mapper_id;
        uint8_t prg_size, chr_size;
        
    public:
        ROM();
        mirroring_type mirroring;
        bool load_rom(const std::string& rom_path);
        bool cpu_read(uint16_t addr, uint8_t &data);
        bool cpu_write(uint16_t addr, uint8_t data);
        bool ppu_read(uint16_t addr, uint8_t &data);
        bool ppu_write(uint16_t addr, uint8_t data);
};