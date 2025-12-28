#include <cpu.h>
#include <membus.h>

int main(){
    CPU cpu;
    MemoryBus membus;

    cpu.attach_membus(&membus);
    return 0;
}