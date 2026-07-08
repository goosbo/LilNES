#include "membus.h"
#include "cpu.h"

MemoryBus::MemoryBus(){
    RAM.fill(0x00);
}

void MemoryBus::write_mem(uint16_t addr, uint8_t data){
    if(addr >= 0x0000 && addr <= 0x1FFF) RAM[addr&0x7ff] = data;
    else if(addr >= 0x2000 && addr <= 0x3fff)ppu->cpu_write(addr,data);
    else if(addr == 0x4014){
        std::array<uint8_t,256> dma_data;
        uint16_t base = data << 8;
        for(int i = 0; i < 256; i++) dma_data[i] = read_mem(base + i);
        ppu->handle_oamdma(dma_data);
        if(cpu){
            cpu->suspended += 513;
        }
    }
    else if(addr == 0x4016 && controller)controller->write(data);
    else if(addr >= 0x6000)rom->cpu_write(addr,data);
    
}

uint8_t MemoryBus::read_mem(uint16_t addr){
    uint8_t data = 0x00;
    if(rom->cpu_read(addr,data))return data;
    if(addr >= 0x0000 && addr <= 0x1FFF) return RAM[addr&0x7ff];
    else if(addr >= 0x2000 && addr <= 0x3fff)return ppu->cpu_read(addr);
    else if(addr == 0x4016 && controller)return controller->read();
    return data;
}

void MemoryBus::push_stk(uint8_t sp, uint8_t data){
    write_mem(0x100+sp,data);
}

uint8_t MemoryBus::pop_stk(uint8_t sp){
    return read_mem(0x100+sp);
}

void MemoryBus::attach_rom(ROM *r){
    rom = r;
}

void MemoryBus::attach_ppu(PPU *p){
    ppu = p;
}

void MemoryBus::attach_cpu(CPU *c) {
    cpu = c;
}

void MemoryBus::attach_controller(Controller *c){
    controller = c;
}