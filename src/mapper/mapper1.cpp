#include "mapper1.h"

Mapper1::Mapper1(uint8_t pbank, uint8_t cbank):Mapper(pbank, cbank) {
    shiftreg = 0x10;
    shiftcnt = 0;
    control = 0x1c;
    chr_bank0 = 0;
    chr_bank1 = 0;
    prgbank_select = 0;
}

bool Mapper1::cpu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr >= 0x8000 && addr <= 0xffff){
        uint8_t prgmode = (control>>2)&3;
        if(prgmode==0|| prgmode==1)mapaddr = (((prgbank_select&0xfe)%prg_bank)*0x4000)+(addr&0x7fff);
        else if(prgmode == 2){
            if(addr>=0x8000&&addr<=0xbfff)mapaddr = addr&0x3fff;
            else mapaddr = ((prgbank_select%prg_bank)*0x4000)+(addr&0x3fff);
        }
        else if(prgmode==3){
            if(addr>=0x8000 && addr <= 0xbfff)mapaddr = (prgbank_select*0x4000)+(addr&0x3fff);
            else mapaddr = (((prg_bank-1)%prg_bank)*0x4000)+(addr&0x3fff);
        }
        return true;
    }
    return false;
}

bool Mapper1::cpu_write(uint16_t addr, uint32_t &mapaddr, uint8_t data){
    if(addr >= 0x8000 && addr <= 0xffff){
        if(data&0x80){
            shiftreg = 0x10;
            shiftcnt = 0;
            control|=0xC;
        }
        else{
            shiftreg>>=1;
            shiftreg |= (data&1)<<4;
            shiftcnt++;

            if(shiftcnt == 5){
                uint8_t reg = (addr>>13)&3;
                switch(reg){
                    case 0:
                        control = shiftreg;
                        break;
                    case 1:
                        chr_bank0 = shiftreg;
                        break;
                    case 2:
                        chr_bank1 = shiftreg;
                        break;
                    case 3:
                        prgbank_select = shiftreg&0xf;
                        break;
                }
                shiftreg=0x10;
                shiftcnt=0;   
            }
        }
        return true;
    }
    return false;
}

bool Mapper1::ppu_read(uint16_t addr, uint32_t &mapaddr){
    if(addr <= 0x1fff){
        if(chr_bank == 0){
            mapaddr = addr;
            return true;
        }
        uint8_t chrmode = (control>>4)&1;
        uint8_t mod = (chr_bank*2);
        if(chrmode == 0) mapaddr = (((chr_bank0&0xfe)%mod)*0x1000)+(addr&0x1fff);
        else{
            if(addr<=0xfff)mapaddr = ((chr_bank0%mod)*0x1000)+(addr&0xfff);
            else mapaddr = ((chr_bank1%mod)*0x1000)+(addr&0xfff);
        }
        return true;
    }
    return false;
}

bool Mapper1::ppu_write(uint16_t addr, uint32_t &mapaddr){
    if(addr <= 0x1fff){
        if(chr_bank == 0){
            mapaddr = addr;
            return true;
        }
    }
    return false;
}

bool Mapper1::get_mirroring(mirroring_type &mirroring){
    switch(control & 3){
        case 0: mirroring = ONESCREENLOW; break;
        case 1: mirroring = ONESCREENHIGH; break;
        case 2: mirroring = VERTICAL; break;
        case 3: mirroring = HORIZONTAL; break;
    }
    return true;
}