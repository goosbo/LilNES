#include <cpu.h>
#include <membus.h>

int main(){
    CPU cpu;
    MemoryBus membus;

    cpu.attach_membus(&membus);

    while(true){
        int cycles = cpu.run_instr();
    }
    
    return 0;
}