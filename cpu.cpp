#include <cpu.h>
#include <stdint.h>

CPU::CPU(){
    a = 0x00;
    x = 0x00;
    y = 0x00;
    sp = 0xFD;
    status = 0x00;
    pc = 0x0000;
    page_crossed = false;
}

uint8_t CPU::get_flag(flag f){
    return ((status & f) > 0)?1:0;
}

void CPU::set_flag(flag f, bool val){
    if(val) status != f;
    else status &= ~f;
}

uint16_t CPU::get_addr(addr_mode mode){
    uint16_t addr;
    page_crossed = 0;
    switch (mode)
    {
    case imm:
        addr = pc++;
        break;
    case zp:
        addr = bus->read_mem(pc++)&0xff;
        break;
    case zpx:
        addr = (bus->read_mem(pc++)+x)&0xff;
        break;
    case zpy:
        addr = (bus->read_mem(pc++)+y)&0xff;
        break;
    case indx:
        uint16_t base = bus->read_mem(pc++);
        uint16_t lo = bus->read_mem((uint16_t)(base+(uint16_t)x)&0xff);
        uint16_t hi = bus->read_mem((uint16_t)(base+(uint16_t)x+1)&0xff);
        addr = (hi << 8)|lo;
        break;
    case indy:
        uint16_t base = bus->read_mem(pc++);
        uint16_t lo = bus->read_mem(base&0xff);
        uint16_t hi = bus->read_mem((base+1)&0xff);
        addr = ((hi << 8)|lo)+y;
        if ((addr & 0xFF00) != (hi << 8)) page_crossed = true;
        break;
    case abs:
        uint16_t lo = bus->read_mem(pc++);
        uint16_t hi = bus->read_mem(pc++);
        addr = (hi << 8)|lo;
        break;
    case absx:
        uint16_t lo = bus->read_mem(pc++);
        uint16_t hi = bus->read_mem(pc++);
        addr = ((hi << 8)|lo) + (uint16_t)x;
        if((addr&0xff00) != (hi<<8)) page_crossed = true;
        break;
    case absy:
        uint16_t lo = bus->read_mem(pc++);
        uint16_t hi = bus->read_mem(pc++);
        addr = ((hi << 8)|lo) + (uint16_t)y;
        if((addr&0xff00) != (hi<<8)) page_crossed = true;
        break;
    case rel:
        uint16_t rel_offset = bus->read_mem(pc++);
        if(rel_offset&0x80) rel_offset |= 0xff00;
        addr = pc + (int16_t)rel_offset;
        if((addr & 0xff00) != (pc & 0xff00)) page_crossed = true;
        break;
    default:
        addr = 0;
        break;
    }
    return addr;
}

void CPU::adc(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res = a + mem + get_flag(C);
    set_flag(C,res > 0xff);
    set_flag(Z,(res&0xff) == 0);
    set_flag(N,res & 0x80);
    set_flag(V,(~((uint16_t)a ^ (uint16_t)mem) & ((uint16_t)a ^ (uint16_t)res)) & 0x80);
    a = res & 0xff;
}

void CPU::and(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    a &= mem;
    set_flag(Z,(a&0xff) == 0);
    set_flag(N,a&0x80);
}

void CPU::asl(uint16_t addr, addr_mode mode){
    uint8_t mem;
    if(mode == acc) mem = a;
    else mem = bus->read_mem(addr);

    set_flag(C,mem&0x80);
    uint8_t res = mem << 1;
    set_flag(Z,res == 0);
    set_flag(N,res&0x80);

    if(mode == acc) a = res;
    else bus->write_mem(addr,res);
}

void CPU::branch(bool cond, uint16_t addr){
    if(cond) pc = addr;
}

void CPU::bcc(uint16_t addr){
    branch(get_flag(C) == 0,addr);
}

void CPU::bcs(uint16_t addr){
    branch(get_flag(C) == 1,addr);
}

void CPU::beq(uint16_t addr){
    branch(get_flag(Z) == 1,addr);
}

void CPU::bne(uint16_t addr){
    branch(get_flag(Z) == 0,addr);
}

void CPU::bmi(uint16_t addr){
    branch(get_flag(N) == 1,addr);
}

void CPU::bpl(uint16_t addr){
    branch(get_flag(N) == 0,addr);
}

void CPU::bvc(uint16_t addr){
    branch(get_flag(V) == 0,addr);
}

void CPU::bvs(uint16_t addr){
    branch(get_flag(V) == 1,addr);
}

void CPU::bit(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    set_flag(N,(mem&0x80)!=0);
    set_flag(V,(mem&0x40)!=0);
    set_flag(Z,(mem&a)==0);
}

void CPU::brk(){
    bus->push_stk((pc >> 8)&0xff);
    bus->push_stk(pc&0xff);
    bus->push_stk(status|B|U);
    set_flag(I,true);
    uint16_t lo = bus->read_mem(0xfffe);
    uint16_t hi = bus->read_mem(0xffff);
    pc = (hi << 8)|lo;
}

void CPU::clc(){
    set_flag(C,false);
}

void CPU::cld(){
    set_flag(D,false);
}

void CPU::cli(){
    set_flag(I,false);
}

void CPU::clv(){
    set_flag(V,false);
}

void CPU::cmp(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res =(uint16_t)a-(uint16_t)mem;
    set_flag(C,a>=mem);
    set_flag(Z,res&0xff == 0);
    set_flag(N,res&0x80);
}

void CPU::cpx(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res =(uint16_t)x-(uint16_t)mem;
    set_flag(C,x>=mem);
    set_flag(Z,res&0xff == 0);
    set_flag(N,res&0x80);
}

void CPU::cpy(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res =(uint16_t)y-(uint16_t)mem;
    set_flag(C,y>=mem);
    set_flag(Z,res&0xff == 0);
    set_flag(N,res&0x80);
}

void CPU::dec(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    mem -= 1;
    set_flag(N,mem & 0x80);
    set_flag(Z,mem == 0);
    bus->write_mem(addr,mem);
}

void CPU::dex(){
    x-=1;
    set_flag(N,x&0x80);
    set_flag(Z,x == 0);
}

void CPU::dey(){
    y-=1;
    set_flag(N,y&0x80);
    set_flag(Z,y == 0);
}

void CPU::eor(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    a ^= mem;
    set_flag(N,a&0x80);
    set_flag(Z,a == 0);
}

void CPU::inc(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    mem += 1;
    set_flag(N,mem & 0x80);
    set_flag(Z,mem == 0);
    bus->write_mem(addr,mem);
}

void CPU::inx(){
    x+=1;
    set_flag(N,x&0x80);
    set_flag(Z,x == 0);
}

void CPU::iny(){
    y+=1;
    set_flag(N,y&0x80);
    set_flag(Z,y == 0);
}