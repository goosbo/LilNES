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
    case ind:
        uint16_t l = bus->read_mem(pc++);
        uint16_t h = bus->read_mem(pc++);
        uint16_t ind_addr = (h<<8)|l;
        if(ind_addr & 0xff == 0xff) addr = ((uint16_t)bus->read_mem(ind_addr&0xff00)<<8)|bus->read_mem(ind_addr);
        else addr = ((uint16_t)bus->read_mem(ind_addr+1)<<8)|bus->read_mem(ind_addr);
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

// void CPU::branch(bool cond, uint16_t addr){
//     if(cond) pc = addr;
// }

// void CPU::bcc(uint16_t addr){
//     branch(get_flag(C) == 0,addr);
// }

// void CPU::bcs(uint16_t addr){
//     branch(get_flag(C) == 1,addr);
// }

// void CPU::beq(uint16_t addr){
//     branch(get_flag(Z) == 1,addr);
// }

// void CPU::bne(uint16_t addr){
//     branch(get_flag(Z) == 0,addr);
// }

// void CPU::bmi(uint16_t addr){
//     branch(get_flag(N) == 1,addr);
// }

// void CPU::bpl(uint16_t addr){
//     branch(get_flag(N) == 0,addr);
// }

// void CPU::bvc(uint16_t addr){
//     branch(get_flag(V) == 0,addr);
// }

// void CPU::bvs(uint16_t addr){
//     branch(get_flag(V) == 1,addr);
// }

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

// void CPU::jmp(uint16_t addr){
//     pc = addr;
// }

// void CPU::jsr(uint16_t addr){
//     bus->push_stk(((pc-1)>>8)&0xff);
//     bus->push_stk((pc-1)&0xff);
//     pc = addr;
// }

void CPU::lda(uint16_t addr){
    a = bus->read_mem(addr);
    set_flag(N,a&0x80);
    set_flag(Z,a==0);
}

void CPU::ldx(uint16_t addr){
    x = bus->read_mem(addr);
    set_flag(N,x&0x80);
    set_flag(Z,x==0);
}

void CPU::ldy(uint16_t addr){
    y = bus->read_mem(addr);
    set_flag(N,y&0x80);
    set_flag(Z,y==0);
}

void CPU::lsr(uint16_t addr,addr_mode mode){
    uint8_t mem;
    if(mode == acc) mem = a;
    else mem = bus->read_mem(addr);

    set_flag(C,mem&1);
    uint8_t res = mem >> 1;
    set_flag(Z,res == 0);
    set_flag(N,false);

    if(mode == acc) a = res;
    else bus->write_mem(addr,res);
}

void CPU::ora(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    a |= mem;
    set_flag(N,a&0x80);
    set_flag(Z,a==0);
}

void CPU::pla(){
    a = bus->pop_stk();
    set_flag(N,a&0x80);
    set_flag(Z,a==0);
}

void CPU::rol(uint16_t addr,addr_mode mode){
    uint8_t mem;
    if(mode == acc)mem = a;
    else mem = bus->read_mem(addr);
    uint8_t carry = get_flag(C);
    uint8_t res = (mem << 1)|carry;
    set_flag(C,mem&0x80);
    set_flag(Z,res == 0);
    set_flag(N,res&0x80);
    if(mode == acc) a = res;
    else bus->write_mem(addr,res);
}

void CPU::ror(uint16_t addr,addr_mode mode){
    uint8_t mem;
    if(mode == acc)mem = a;
    else mem = bus->read_mem(addr);
    uint8_t carry = get_flag(C);
    uint8_t res = (mem >> 1)|(carry<<7);
    set_flag(C,mem&1);
    set_flag(Z,res == 0);
    set_flag(N,res&0x80);
    if(mode == acc) a = res;
    else bus->write_mem(addr,res);
}

void CPU::sbc(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    mem = ~mem;
    uint16_t res = a + mem + get_flag(C);
    set_flag(C,res>0xff);
    set_flag(Z,res&0xff == 0);
    set_flag(N,res&0x80);
    set_flag(V, (~((uint16_t)a ^ (uint16_t)mem) & ((uint16_t)a ^ (uint16_t)res)) & 0x0080);
    a = res & 0xff;
}

int CPU::run_instr(){
    uint16_t addr;
    uint16_t op = bus->read_mem(pc++);
    int cycles = 0;
    switch(op){
        case 0x00:
            brk();
            cycles = 7;
            break;
        case 0x01:
            addr = get_addr(indx);
            ora(addr);
            cycles = 6;
            break;
        case 0x05:
            addr = get_addr(zp);
            ora(addr);
            cycles = 3;
            break;
        case 0x06:
            addr = get_addr(zp);
            asl(addr,zp);
            cycles = 5;
            break;
        case 0x08:
            bus->push_stk(status);
            cycles = 3;
            break;
        case 0x09:
            addr = get_addr(imm);
            ora(addr);
            cycles = 2;
            break;
        case 0x0a:
            asl(0,acc);
            cycles = 2;
            break;
        case 0x0d:
            addr = get_addr(abs);
            ora(addr);
            cycles = 4;
            break;
        case 0x0e:
            addr = get_addr(abs);
            asl(addr,abs);
            cycles = 6;
            break;
        case 0x10:
            addr = get_addr(rel);
            cycles = 2;
            if(!get_flag(N)){
                cycles += 1;
                pc = addr;
            }
            if(page_crossed)cycles++;
            break;
        case 0x11:
            addr = get_addr(indy);
            ora(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0x15:
            addr = get_addr(zpx);
            ora(addr);
            cycles = 4;
            break;
        case 0x16:
            addr = get_addr(zpx);
            asl(addr,zpx);
            cycles = 6;
            break;
        case 0x18:
            set_flag(C,false);
            cycles = 2;
            break;
        case 0x19:
            addr = get_addr(absy);
            ora(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x1d:
            addr = get_addr(absx);
            ora(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x1e:
            addr = get_addr(absx);
            asl(addr,absx);
            cycles = 7;
            break;
        case 0x20:
            addr = get_addr(abs);
            bus->push_stk(((pc-1)>>8)&0xff);
            bus->push_stk((pc-1)&0xff);
            pc = addr;
            cycles = 6;
            break;
        case 0x21:
            addr = get_addr(indx);
            and(addr);
            cycles = 6;
            break;
        case 0x24:
            addr = get_addr(zp);
            bit(addr);
            cycles = 3;
            break;
        case 0x25:
            addr = get_addr(zp);
            and(addr);
            cycles = 3;
            break;
        case 0x26:
            addr = get_addr(zp);
            rol(addr,zp);
            cycles = 5;
            break;
        case 0x28:
            uint8_t val = bus->pop_stk();
            status = val;
            set_flag(U,true);
            set_flag(B,false);
            cycles = 4;
            break;
        case 0x29:
            addr = get_addr(imm);
            and(addr);
            cycles = 2;
            break;
        case 0x2a:
            rol(0,acc);
            cycles = 2;
            break;
        case 0x2c:
            addr = get_addr(abs);
            bit(addr);
            cycles = 4;
            break;
        case 0x2d:
            addr = get_addr(abs);
            and(addr);
            cycles = 4;
            break;
        case 0x2e:
            addr = get_addr(abs);
            rol(addr,abs);
            cycles = 6;
            break;
        case 0x30:
            cycles = 2;
            if(get_flag(N)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0x31:
            addr = get_addr(indy);
            and(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0x35:
            addr = get_addr(zpx);
            and(addr);
            cycles = 4;
            break;
        case 0x36:
            addr = get_addr(zpx);
            rol(addr,zpx);
            cycles = 6;
            break;
        case 0x38:
            set_flag(C,true);
            cycles = 2;
            break;
        case 0x39:
            addr = get_addr(absy);
            and(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x3d:
            addr = get_addr(absx);
            and(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x3e:
            addr = get_addr(absx);
            rol(addr,absx);
            cycles = 7;
            break;
        default:



    }
    return cycles;
}