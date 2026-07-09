#include "mapper3.h"

Mapper3::Mapper3(uint8_t pbank, uint8_t cbank):Mapper(pbank, cbank) {
    chrbank_select = 0;
}

bool Mapper3::cpu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0x8000 && addr <=0xffff){
        mapaddr = addr & (prg_bank>1?0x7fff:0x3fff);
        return true;
    }
    return false;
}

bool Mapper3::cpu_write(uint16_t addr, uint32_t &mapaddr, uint8_t data){
    if(addr >= 0x8000 && addr <= 0xffff){
        chrbank_select = data&3;
        return true;
    }
    return false;
}

bool Mapper3::ppu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr <= 0x1fff){
        mapaddr = (chrbank_select*0x2000)+addr;
        return true;
    }
    return false;
}

bool Mapper3::ppu_write(uint16_t addr,uint32_t &mapaddr){
    return false;
}