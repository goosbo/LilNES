#include "cpu.h"
#include "membus.h"
#include "rom.h"
#include <iostream>
int main(){
    CPU cpu;
    MemoryBus membus;
    ROM rom;

    rom.load_rom("nestest.nes");
    membus.attach_rom(rom);
    cpu.attach_membus(&membus);
    int steps = 0;
    while(steps++ <10000){
        //cpu.log_state();
        int cycles = cpu.run_instr();
        cpu.total_cycles += cycles;
    }
    
    return 0;
}