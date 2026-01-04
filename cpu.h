#pragma once
#include <cstdint>
#include <array>
#include "membus.h"

enum addr_mode{
    imm,//immediate
    zp,//zeropage
    zpx,//zeropage + xindexed
    zpy,//zeropage + yindexed
    indx,//indirect + xindexed
    indy,//indirect + yindexed
    _abs,//absolute
    absx,//absolute + xindexed
    absy, //absolute + yindexed
    acc,//accumulator
    imp,//implied,
    rel,//relative,
    ind//indirect
};

enum flag{
    C = 1,
    Z = 2,
    I = 4,
    D = 8,
    B = 16,
    U = 32,
    V = 64,
    N = 128
};

class CPU{

    private:
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint16_t pc;
    uint8_t sp;
    uint8_t status;
    MemoryBus* bus;
    bool page_crossed;
    
    uint8_t get_flag(flag f);
    void set_flag(flag f,bool val);
    uint16_t get_addr(addr_mode mode);
    void adc(uint16_t addr);
    void _and(uint16_t addr);
    void asl(uint16_t addr,addr_mode mode);
    void bit(uint16_t addr);
    void brk();
    void cmp(uint16_t addr);
    void cpx(uint16_t addr);
    void cpy(uint16_t addr);
    void dec(uint16_t addr);
    void dex();
    void dey();
    void eor(uint16_t addr);
    void inc(uint16_t addr);
    void inx();
    void iny();
    void lda(uint16_t addr);
    void ldx(uint16_t addr);
    void ldy(uint16_t addr);
    void lsr(uint16_t addr,addr_mode mode);
    void ora(uint16_t addr);
    void pla();
    void rol(uint16_t addr,addr_mode mode);
    void ror(uint16_t addr,addr_mode mode);
    void sbc(uint16_t addr);
    void push(uint8_t data);
    uint8_t pop();

    public:
    CPU();
    uint64_t total_cycles;
    void nmi();
    void irq();
    void reset();
    int run_instr();
    void emu_loop();
    void log_state();
    void attach_membus(MemoryBus* bus);

};