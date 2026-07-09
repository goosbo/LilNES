#include "mapper2.h"

Mapper2::Mapper2(uint8_t pbank, uint8_t cbank):Mapper(pbank, cbank) {
    prgbank_select = 0;
}

bool Mapper2::cpu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0x8000 && addr <=0xbfff){
        mapaddr = (prgbank_select%prg_bank)*0x4000 + (addr&0x3fff);
        return true;
    }
    if(addr >= 0xc000 && addr <= 0xffff){
        mapaddr = (prg_bank-1)*0x4000 + (addr&0x3fff);
        return true;
    }
    return false;
}

bool Mapper2::cpu_write(uint16_t addr, uint32_t &mapaddr, uint8_t data){
    if(addr >= 0x8000 && addr <= 0xffff){
        prgbank_select = data&0xf;
        return true;
    }
    return false;
}

bool Mapper2::ppu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr<=0x1fff){
        mapaddr = addr;
        return true;
    }
    return false;
}

bool Mapper2::ppu_write(uint16_t addr, uint32_t &mapaddr){
    if(addr <= 0x1fff && chr_bank == 0){
        mapaddr = addr;
        return true;
    }
    return false;
}