#include "cpu.h"
#include "membus.h"
#include "rom.h"
#include <iostream>
int main(){
    PPU ppu;
    CPU cpu;
    MemoryBus membus;
    ROM rom;

    rom.load_rom("nestest.nes");
    membus.attach_ppu(&ppu);
    membus.attach_rom(&rom);
    cpu.attach_membus(&membus);
    ppu.attach_cpu(&cpu);
    int steps = 0;
    while(steps++ <10000){
        //cpu.log_state();
        int cycles = cpu.run_instr();
        cpu.total_cycles += cycles;
    }
    
    return 0;
}