#include "ppu.h"
#include "cpu.h"
#include "rom.h"
#include <iostream>

const uint32_t colors[64] = {
    0x666666FF, 0x002A88FF, 0x1412A7FF, 0x3B00A4FF, 0x5C007EFF, 0x6E0040FF, 0x6C0600FF, 0x561D00FF,
    0x333500FF, 0x0B4800FF, 0x005200FF, 0x004F08FF, 0x00404DFF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xADADADFF, 0x155FD9FF, 0x4240FFFF, 0x7527FEFF, 0xA01ACCFF, 0xB71E7BFF, 0xB53120FF, 0x994E00FF,
    0x6B6D00FF, 0x388700FF, 0x0C9300FF, 0x008F32FF, 0x007C8DFF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFFFFFFFF, 0x64B0FFFF, 0x9290FFFF, 0xC676FFFF, 0xF36AFFFF, 0xFE6ECCFF, 0xFE8170FF, 0xEA9E22FF,
    0xBCBE00FF, 0x88D800FF, 0x5CE430FF, 0x45E082FF, 0x48CDDEFF, 0x4F4F4FFF, 0x000000FF, 0x000000FF,
    0xFFFFFFFF, 0xC0DFFFFF, 0xD3D2FFFF, 0xE8C8FFFF, 0xFBC2FFFF, 0xFEC4EAFF, 0xFECCC5FF, 0xF7D8A5FF,
    0xE4E594FF, 0xCFEF96FF, 0xBDF4ABFF, 0xB3F3CCFF, 0xB5EBF2FF, 0xB8B8B8FF, 0x000000FF, 0x000000FF
};

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
    framedone = false;
    scanline = 0;
    cycle = 0;
    for(int i = 0; i < 256*240; i++) screen[i] = 0xFF000000;
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
            mask = data;
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            if(w == 0){
                x = data & 7;
                t = (t&0xffe0)|(data>>3);
                w = 1;
            }
            else {
                t = (t&0x8fff)|((data&7)<<12);
                t = (t&0xfc1f)|((data&0xf8)<<2);
            }
            break;
        case 6:
            if(w == 0){
                t = (t&0x80ff)|((uint16_t)(data &0x3f) <<8);
                w = 1;
            }
            else{
                t = (t&0xff00)|data;
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

void PPU::write_vram_mem(uint16_t addr, uint8_t data) {
    if (rom->ppu_write(addr, data)) return; 
    
    if (addr >= 0x2000 && addr < 0x3F00) {
        uint16_t mirrored_addr = addr & 0x2FFF;
        if (mirrored_addr < 0x2400) nametables[0][mirrored_addr & 0x03FF] = data;
        else nametables[1][mirrored_addr & 0x03FF] = data;
    } 
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        uint16_t palette_addr = addr & 0x001F;
        if (palette_addr == 0x0010) palette_addr = 0x0000;
        if (palette_addr == 0x0014) palette_addr = 0x0004;
        if (palette_addr == 0x0018) palette_addr = 0x0008;
        if (palette_addr == 0x001C) palette_addr = 0x000C;
        
        palette[palette_addr] = data;
    }
}

uint8_t PPU::read_vram_mem(uint16_t addr){
    uint8_t data = 0;
    if (rom->ppu_read(addr, data)) return data;
    
    if(addr >= 0x2000 && addr < 0x3f00){
        uint16_t mirrored_addr = addr & 0x2FFF;
        if(mirrored_addr < 0x2400) return nametables[0][mirrored_addr & 0x03FF];
        else return nametables[1][mirrored_addr & 0x03FF]; 
    }
    else if(addr >= 0x3f00 && addr <= 0x3FFF) {
        uint16_t palette_addr = addr & 0x001F;
        if (palette_addr == 0x0014) palette_addr = 0x0004;
        if (palette_addr == 0x0018) palette_addr = 0x0008;
        if (palette_addr == 0x001C) palette_addr = 0x000C;
        return palette[palette_addr];
    }
    return 0;
}

void PPU::handle_oamdma(const std::array<uint8_t, 256>& dma_data){
    for(int i = 0; i < 256; i++)OAM[oamaddr++] = dma_data[i];
}

void PPU::step(){
    
    if (scanline >= 0 && scanline < 240 || scanline == 261){
        if(scanline == 261 && cycle == 1) status &= ~0xe0;
        if((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336)) {
            update_shifters();
            switch((cycle-1)%8){
                case 0:
                    load_shifters();
                    tileid = read_vram_mem(0x2000 | (v&0xfff));
                    break;
                case 2:
                    tileattrib = read_vram_mem(0x23c0 | (v & 0xc00) | ((v >> 4) & 0x38) | ((v >> 2) & 7));
                    if (v & 0x40) tileattrib >>= 4;
                    if (v & 2) tileattrib >>= 2;
                    tileattrib &= 3;
                    break;
                case 4:
                    tilelsb = read_vram_mem(((ctrl&0x10)?0x1000:0)+((uint16_t)tileid<<4)+((v>>12)&7));
                    break;
                case 6:
                    tilemsb = read_vram_mem(((ctrl&0x10)?0x1000:0)+((uint16_t)tileid<<4)+(((v>>12)&7)+8));
                    break;
                case 7:
                    inc_scroll_x();
                    break;
            }
        }
        if(cycle == 256)inc_scroll_y();
        if(cycle == 257){
            load_shifters();
            transfer_addr_x();
        }
        if (scanline == 261 && cycle >= 280 && cycle <= 304)transfer_addr_y();
        if (scanline >= 0 && scanline <= 239 && cycle >= 1 && cycle <= 256) render();
    }

    if(scanline == 241 && cycle == 1){
        status |= 0x80;
        if(ctrl & 0x80) cpu->nmi();
    }
    cycle++;
    if(cycle >= 341){
        cycle = 0;
        scanline++;
        if(scanline > 261){
            scanline = 0;
            framedone = true;
        } 
    }

}

void PPU::inc_scroll_x(){
    if((mask&0x18) == 0)return;
    if((v&0x1f) == 31){
        v &= ~0x1f;
        v &= 0x400;
    }
    else v += 1;
}

void PPU::inc_scroll_y(){
    if((mask & 0x18) == 0) return;
    if ((v & 0x7000) != 0x7000)v += 0x1000;
    else {
        v &= ~0x7000;
        int y = (v & 0x03E0) >> 5;  
        if (y == 29) {
            y = 0;
            v ^= 0x0800;
        } else if (y == 31)y = 0;
        else y += 1;
        v = (v & ~0x3e0) | (y << 5);
    }
}

void PPU::transfer_addr_x() {
    if ((mask & 0x18) == 0) return;
    v = (v & ~0x41f) | (t & 0x41f);
}

void PPU::transfer_addr_y() {
    if ((mask & 0x18) == 0) return;
    v = (v & ~0x7be0) | (t & 0x7be0);
}

void PPU::load_shifters(){
    patlow = (patlow&0xff00)|tilelsb;
    pathigh = (pathigh&0xff00)|tilemsb;
    attriblow = (attriblow&0xff00)|((tileattrib&1)?0xff:0);
    attribhigh = (attribhigh&0xff00)|((tileattrib&2)?0xff:0);
}

void PPU::update_shifters(){
    if ((mask & 0x18) == 0) return;
    patlow <<= 1;
    pathigh <<= 1;
    attriblow <<= 1;
    attribhigh <<= 1;
}

void PPU::render(){
    if((mask &8) == 0)return;
    uint16_t bit = 0x8000>>x;
    uint8_t bgpixel = (((pathigh&bit)>0)<<1)|((patlow&bit)>0);
    uint8_t bgpal = (((attribhigh&bit)>0)<<1)|((attriblow&bit)>0);
    uint8_t pixel = 0, pal = 0;
    if (bgpixel != 0){
        pixel = bgpixel;
        pal = bgpal;
    }
    uint16_t paladdr = 0x3F00 + (pal << 2) + pixel;

    uint8_t coloridx = read_vram_mem(paladdr) & 0x3f;
    uint32_t c = colors[coloridx];
    uint8_t r = (c >> 24) & 0xFF;
    uint8_t g = (c >> 16) & 0xFF;
    uint8_t b = (c >> 8) & 0xFF;
    uint8_t a = c & 0xFF;
    
    screen[scanline * 256 + (cycle - 1)] = (a << 24) | (b << 16) | (g << 8) | r;
}

void PPU::attach_rom(ROM *r){
    rom = r;
}

