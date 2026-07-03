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
    membus.attach_cpu(&cpu);
    cpu.attach_membus(&membus);
    ppu.attach_cpu(&cpu);
    bool running = true;
    int ins_run = 0;
    for(int i = 0; i < 21; i++) {
        ppu.step();
    }
    while (running && ins_run < 8991){
        if (cpu.suspended == 0) {
            cpu.log_state(); 
            ins_run++;
        }
        int cpu_cycles = 0;
        if (cpu.suspended > 0){
            cpu.suspended--;
            cpu_cycles = 1;
        }
        else cpu_cycles = cpu.run_instr();
        cpu.total_cycles += cpu_cycles;

        for(int i = 0; i < cpu_cycles * 3; i++) ppu.step();
    }

    return 0;
}