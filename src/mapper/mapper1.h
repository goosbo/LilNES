#pragma once
#include "mapper.h"

class Mapper1 : public Mapper {
private:
    uint8_t shiftreg;
    uint8_t shiftcnt;
    uint8_t control;
    uint8_t chr_bank0;
    uint8_t chr_bank1;
    uint8_t prgbank_select;

public:
    Mapper1(uint8_t pbank, uint8_t cbank);

    bool cpu_read(uint16_t addr, uint32_t &mapaddr) override;
    bool cpu_write(uint16_t addr, uint32_t &mapaddr, uint8_t data) override;
    bool ppu_read(uint16_t addr, uint32_t &mapaddr) override;
    bool ppu_write(uint16_t addr, uint32_t &mapaddr) override;
};