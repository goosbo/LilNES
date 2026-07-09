#include "mapper0.h"

Mapper0::Mapper0(uint8_t pbank, uint8_t cbank):Mapper(pbank, cbank) {}

bool Mapper0::cpu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0x8000 & addr <=0xffff){
        mapaddr = addr & (prg_bank>1?0x7fff:0x3fff);
        return true;
    }
    return false;
}

bool Mapper0::cpu_write(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0x8000 & addr <=0xffff){
        mapaddr = addr & (prg_bank>1?0x7fff:0x3fff);
        return true;
    }
    return false;
}

bool Mapper0::ppu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0 && addr <= 0x1fff){
        mapaddr = addr;
        return true;
    }
    return false;
}

bool Mapper0::ppu_write(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0 && addr <= 0x1fff){
        if(chr_bank == 0){
            mapaddr = addr;
            return true;
        }
    }
    return false;
}