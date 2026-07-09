#pragma once
#include "mapper.h"

class Mapper2 : public Mapper {
    private:
    uint8_t prgbank_select;
    
    public:
    Mapper2(uint8_t pbanks, uint8_t cbanks);

    bool cpu_read(uint16_t addr, uint32_t &mapaddr) override;
    bool cpu_write(uint16_t addr, uint32_t &mapaddr,uint8_t data) override;
    bool ppu_read(uint16_t addr, uint32_t &mapaddr) override;
    bool ppu_write(uint16_t addr, uint32_t &mapaddr) override;
};