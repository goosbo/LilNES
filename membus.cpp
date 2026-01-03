#include <membus.h>

MemoryBus::MemoryBus(){
    RAM.fill(0x00);
    stk.fill(0x00);
}

void MemoryBus::write_mem(uint16_t addr, uint8_t data){
    if(addr >= 0x0000 && addr <= 0x1FFF) RAM[addr&0x7ff] = data;
    else if(addr >= 0x6000)rom->cpu_write(addr,data);
}

uint8_t MemoryBus::read_mem(uint16_t addr){
    uint8_t data = 0x00;
    if(rom->cpu_read(addr,data))return data;
    if(addr >= 0x0000 && addr <= 0x1FFF) return RAM[addr&0x7ff];
    return data;
}

void MemoryBus::push_stk(uint8_t sp, uint8_t data){
    write_mem(0x100+sp,data);
}

uint8_t MemoryBus::pop_stk(uint8_t sp){
    return read_mem(0x100+sp);
}