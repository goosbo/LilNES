#pragma once
#include <cstdint>

enum mirroring_type{
    HORIZONTAL,
    VERTICAL,
    ONESCREENLOW,
    ONESCREENHIGH
};
class Mapper{
    protected:
    uint8_t prg_bank;
    uint8_t chr_bank;

    public:
    Mapper(uint8_t pbank,uint8_t cbank);
    virtual bool cpu_read(uint16_t addr, uint32_t &mapaddr) = 0;
    virtual bool cpu_write(uint16_t addr, uint32_t &mapaddr,uint8_t data) = 0;
    virtual bool ppu_read(uint16_t addr, uint32_t &mapaddr) = 0;
    virtual bool ppu_write(uint16_t addr, uint32_t &mapaddr) = 0;
    virtual bool get_mirroring(mirroring_type &mirroring) { return false; }
};