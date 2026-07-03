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
        case 0:{
            bool prev_nmibit = ctrl&0x80;
            ctrl = data;
            t = (t&0xf3ff)|((data&3)<<10);
            if(prev_nmibit && (ctrl&0x80) && (status&0x80)) cpu->nmi();
        }
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
        case 2:
            w = 0;
            res = status;
            status &= 0x7f;
            break;
        case 4:
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

void PPU::write_vram_mem(uint16_t addr,uint8_t data){
    if(addr < 0x1000) pattern_tables[0][addr]= data;
    else if(addr < 0x2000) pattern_tables[1][addr%0x1000] = data;
    else if(addr < 0x3f00){
        if(addr%0x800 < 0x400)nametables[0][addr%0x400] = data;
        else nametables[1][addr%0x400] = data;
    } 
    else if(addr < 0x4000) palette[addr%32] = data;
    
}

uint8_t PPU::read_vram_mem(uint16_t addr){
    if(addr < 0x1000) return pattern_tables[0][addr];
    else if(addr < 0x2000)return pattern_tables[1][addr%0x1000];
    else if(addr < 0x3f00){
        if(addr%0x800 < 0x400)return nametables[0][addr%0x400];
        else return nametables[1][addr%0x400];
    }
    else if(addr < 0x4000)return palette[addr%32];
    else return 0;
}

void PPU::handle_oamdma(const std::array<uint8_t, 256>& dma_data){
    for(int i = 0; i < 256; i++)OAM[oamaddr++] = dma_data[i];
}

void PPU::step(){
    cycle++;
    if(cycle >= 341){
        cycle = 0;
        scanline++;
        if(scanline > 2621) scanline = 0;
    }

    if(scanline == 241 && cycle == 1){
        status |= 0x80;
        if(ctrl & 0x80) cpu->nmi();
    }

    if(scanline == 261 && cycle == 1) status &= ~0x80;
}