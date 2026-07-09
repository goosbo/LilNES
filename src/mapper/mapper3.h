#pragma once
#include "mapper.h"

class Mapper3 : public Mapper {
    private:
    uint8_t chrbank_select;
    
    public:
    Mapper3(uint8_t prg_banks, uint8_t chr_banks);

    bool cpu_read(uint16_t addr, uint32_t &mapaddr) override;
    bool cpu_write(uint16_t addr, uint32_t &mapaddr,uint8_t data) override;
    bool ppu_read(uint16_t addr, uint32_t &mapaddr) override;
    bool ppu_write(uint16_t addr, uint32_t &mapaddr) override;
};