#include "cpu.h"
#include "membus.h"
#include "rom.h"
int main(){
    CPU cpu;
    MemoryBus membus;
    ROM rom;

    rom.load_rom("nestest.nes");
    membus.attach_rom(rom);
    cpu.attach_membus(&membus);

    while(true){
        int cycles = cpu.run_instr();
    }
    
    return 0;
}