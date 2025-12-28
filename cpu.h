#include <cstdint>
#include <array>
#include <membus.h>

enum addr_mode{
    imm,//immediate
    zp,//zeropage
    zpx,//zeropage + xindexed
    zpy,//zeropage + yindexed
    indx,//indirect + xindexed
    indy,//indirect + yindexed
    abs,//absolute
    absx,//absolute + xindexed
    absy, //absolute + yindexed
    acc,//accumulator
    imp,//implied,
    rel//relative
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
    void and(uint16_t addr);
    void asl(uint16_t addr,addr_mode mode);
    void branch(bool cond, uint16_t addr);
    void bcc(uint16_t addr);
    void bcs(uint16_t addr);
    void beq(uint16_t addr);
    void bne(uint16_t addr);
    void bmi(uint16_t addr);
    void bpl(uint16_t addr);
    void bvc(uint16_t addr);
    void bvs(uint16_t addr);
    void bit(uint16_t addr);
    void brk();
    void clc();
    void cld();
    void cli();
    void clv();
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
    
    public:
    CPU();
    ~CPU();

    int run_instr();
    void attach_membus(MemoryBus* bus);

};