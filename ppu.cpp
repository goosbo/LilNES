#include "ppu.h"
#include "cpu.h"

PPU::PPU(){
    ctrl = 0;
    mask = 0;
    status = 0;
    oamaddr = 0;
    v = 0;
    t = 0;
    x = 0;
    w = 0;
    cpu = nullptr;
    databuffer = 0;
}

void PPU::cpu_write(uint16_t addr,uint8_t data){
    switch(addr % 8){
        case 0:
            bool prev_nmibit = ctrl&0x80;
            ctrl = data;
            t = (t&0xf3ff)|((data&3)<<10);
            if(prev_nmibit && (ctrl&0x80) && (status&0x80)) cpu->nmi();
            break;
        case 1:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            if(w == 0){
                t = (t&0xff)|((uint16_t)(data &0x3f) <<8);
                w = 1;
            }
            else{
                t = (t&0xff)|data;
                v = t;
                w = 0;
            }
            break;
        case 7:
            write_vram_mem(v,data);
            if(ctrl&4)v += 32;
            else v += 1;
            break;
    }
}

uint8_t PPU::cpu_read(uint16_t addr){
    uint8_t res = 0;
    switch (addr%8){
        case 1:
            break;
        case 2:
            w = 0;
            res = status;
            status &= 0x7f;
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        case 7:
            res = databuffer;
            databuffer = read_vram_mem(v);
            if(v >= 0x3f00)res = databuffer;

            if(ctrl&4)v += 32;
            else v += 1;
            break;
        }
    return res;
}

void PPU::attach_cpu(CPU *c){
    cpu = c;
}