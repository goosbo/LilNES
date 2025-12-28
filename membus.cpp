#include <membus.h>

MemoryBus::MemoryBus(){
    RAM.fill(0x00);
    stk.fill(0x00);
}

void MemoryBus::write_mem(uint16_t addr, uint8_t data){
    if(addr >= 0x0000 && addr <= 0x1FFF) RAM[addr&0x7ff] = data;;
}

uint8_t MemoryBus::read_mem(uint16_t addr){
    if(addr >= 0x0000 && addr <= 0x1FFF) return RAM[addr&0x7ff];
    return 0x00;
}