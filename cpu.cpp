#include "cpu.h"
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
    if(val) status |= f;
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
        if((ind_addr & 0xff) == 0xff) addr = ((uint16_t)bus->read_mem(ind_addr&0xff00)<<8)|bus->read_mem(ind_addr);
        else addr = ((uint16_t)bus->read_mem(ind_addr+1)<<8)|bus->read_mem(ind_addr);
        break;

    default:
        addr = 0;
        break;
    }
    return addr;
}

void CPU::push(uint8_t data){
    bus->push_stk(sp,data);
    sp--;
}

uint8_t CPU::pop(){
    sp++;
    return bus->pop_stk(sp);
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

void CPU::bit(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    set_flag(N,(mem&0x80)!=0);
    set_flag(V,(mem&0x40)!=0);
    set_flag(Z,(mem&a)==0);
}

void CPU::brk(){
    pc++;
    push((pc >> 8)&0xff);
    push(pc&0xff);
    push(status|B|U);
    set_flag(I,true);
    uint16_t lo = bus->read_mem(0xfffe);
    uint16_t hi = bus->read_mem(0xffff);
    pc = (hi << 8)|lo;
}

void CPU::nmi(){
    push((pc >> 8)&0xff);
    push(pc&0xff);
    set_flag(B,false);
    set_flag(U,true);
    set_flag(I,true);
    push(status);
    uint16_t lo = bus->read_mem(0xfffa);
    uint16_t hi = bus->read_mem(0xfffb);
    pc = (hi << 8)|lo;
}

void CPU::irq(){
    if(get_flag(I)) return;
    push((pc >> 8)&0xff);
    push(pc&0xff);
    set_flag(B,false);
    set_flag(U,true);
    set_flag(I,true);
    push(status);
    uint16_t lo = bus->read_mem(0xfffe);
    uint16_t hi = bus->read_mem(0xffff);
    pc = (hi << 8)|lo;
}

void CPU::reset(){
    a = x = y = 0;
    sp = 0xFD;
    status = 0x00|U;
    uint16_t lo = bus->read_mem(0xfffc);
    uint16_t hi = bus->read_mem(0xfffd);
    pc = (hi << 8)|lo;
}

void CPU::cmp(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res =(uint16_t)a-(uint16_t)mem;
    set_flag(C,a>=mem);
    set_flag(Z,(res&0xff) == 0);
    set_flag(N,res&0x80);
}

void CPU::cpx(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res =(uint16_t)x-(uint16_t)mem;
    set_flag(C,x>=mem);
    set_flag(Z,(res&0xff) == 0);
    set_flag(N,res&0x80);
}

void CPU::cpy(uint16_t addr){
    uint8_t mem = bus->read_mem(addr);
    uint16_t res =(uint16_t)y-(uint16_t)mem;
    set_flag(C,y>=mem);
    set_flag(Z,(res&0xff) == 0);
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
    a = pop();
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
            push(status|B|U);
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
                if(page_crossed)cycles++;
            }
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
            push(((pc-1)>>8)&0xff);
            push((pc-1)&0xff);
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
            uint8_t val = pop();
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
        case 0x40:
            status = pop();
            uint16_t lo = pop();   
            uint16_t hi = pop();
            pc = (hi << 8)|lo;
            set_flag(U,true);
            set_flag(B,false);
            cycles = 6;
            break;
        case 0x41:
            addr = get_addr(indx);
            eor(addr);
            cycles = 6;
            break;
        case 0x45:
            addr = get_addr(zp);
            eor(addr);
            cycles = 3;
            break;
        case 0x46:
            addr = get_addr(zp);
            lsr(addr,zp);
            cycles = 5;
            break;
        case 0x48:
            push(a);
            cycles = 3;
            break;
        case 0x49:
            addr = get_addr(imm);
            eor(addr);
            cycles = 2;
            break;
        case 0x4a:
            lsr(0,acc);
            cycles = 2;
            break;
        case 0x4c:
            addr = get_addr(abs);
            pc = addr;
            cycles = 3;
            break;
        case 0x4d:
            addr = get_addr(abs);
            eor(addr);
            cycles = 4;
            break;
        case 0x4e:
            addr = get_addr(abs);
            lsr(addr,abs);
            cycles = 6;
            break; 
        case 0x50:
            cycles = 2;
            if(!get_flag(V)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0x51:
            addr = get_addr(indy);
            eor(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0x55:
            addr = get_addr(zpx);
            eor(addr);
            cycles = 4;
            break;
        case 0x56:
            addr = get_addr(zpx);
            lsr(addr,zpx);
            cycles = 6;
            break;
        case 0x58:
            set_flag(I,false);
            cycles = 2;
            break;
        case 0x59:
            addr = get_addr(absy); 
            eor(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x5d:
            addr = get_addr(absx);
            eor(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x5e:
            addr = get_addr(absx);
            lsr(addr,absx);
            cycles = 7;
            break;
        case 0x60:
            uint16_t lo = pop();   
            uint16_t hi = pop();
            pc = ((hi << 8)|lo)+1;
            cycles = 6;
            break;
        case 0x61:
            addr = get_addr(indx);
            adc(addr);
            cycles = 6;
            break;
        case 0x65:
            addr = get_addr(zp);
            adc(addr);
            cycles = 3;
            break;
        case 0x66:
            addr = get_addr(zp);
            ror(addr,zp);
            cycles = 5;
            break;
        case 0x68:
            pla();
            cycles = 4;
            break;
        case 0x69:
            addr = get_addr(imm);
            adc(addr);
            cycles = 2;
            break;
        case 0x6a:
            ror(0,acc);
            cycles = 2;
            break;
        case 0x6c:
            addr = get_addr(ind);
            pc = addr;
            cycles = 5;
            break;
        case 0x6d:
            addr = get_addr(abs);
            adc(addr);
            cycles = 4;
            break;
        case 0x6e:
            addr = get_addr(abs);
            ror(addr,abs);
            cycles = 6;
            break;
        case 0x70:
            cycles = 2;
            if(get_flag(V)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0x71:
            addr = get_addr(indy);
            adc(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0x75:
            addr = get_addr(zpx);
            adc(addr);
            cycles = 4;
            break;
        case 0x76:
            addr = get_addr(zpx);
            ror(addr,zpx);
            cycles = 6;
            break;
        case 0x78:
            set_flag(I,true);
            cycles = 2;
            break;
        case 0x79:
            addr = get_addr(absy);
            adc(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x7d:
            addr = get_addr(absx);
            adc(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x7e:
            addr = get_addr(absx);
            ror(addr,absx);
            cycles = 7;
            break;
        case 0x81:
            addr = get_addr(indx);
            bus->write_mem(addr,a);
            cycles = 6;
            break;
        case 0x84:
            addr = get_addr(zp);
            bus->write_mem(addr,y);
            cycles = 3;
            break;
        case 0x85:
            addr = get_addr(zp);
            bus->write_mem(addr,a);
            cycles = 3;
            break;
        case 0x86:
            addr = get_addr(zp);
            bus->write_mem(addr,x);
            cycles = 3;
            break;
        case 0x88:
            dey();
            cycles = 2;
            break;
        case 0x8a:
            a = x;
            set_flag(N,a&0x80);
            set_flag(Z,a==0);
            cycles = 2;
            break;
        case 0x8c:
            addr = get_addr(abs);
            bus->write_mem(addr,y);
            cycles = 4;
            break;
        case 0x8d:
            addr = get_addr(abs);
            bus->write_mem(addr,a);
            cycles = 4;
            break;
        case 0x8e:
            addr = get_addr(abs);
            bus->write_mem(addr,x);
            cycles = 4;
            break;
        case 0x90:
            cycles = 2;
            if(!get_flag(C)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0x91:
            addr = get_addr(indy);
            bus->write_mem(addr,a);
            cycles = 6;
            if(page_crossed)cycles++;
            break;
        case 0x94:
            addr = get_addr(zpx);
            bus->write_mem(addr,y);
            cycles = 4;
            break;
        case 0x95:
            addr = get_addr(zpx);
            bus->write_mem(addr,a);
            cycles = 4;
            break;
        case 0x96:
            addr = get_addr(zpy);
            bus->write_mem(addr,x);
            cycles = 4;
            break;
        case 0x98:
            a = y;
            set_flag(N,a&0x80);
            set_flag(Z,a==0);
            cycles = 2;
            break;
        case 0x99:
            addr = get_addr(absy);
            bus->write_mem(addr,a);
            cycles = 5;
            break;
        case 0x9a:
            sp = x;
            cycles = 2;
            break;
        case 0x9d:
            addr = get_addr(absx);
            bus->write_mem(addr,a);
            cycles = 5;
            break;
        case 0xa0:
            addr = get_addr(imm);
            ldy(addr);
            cycles = 2;
            break;
        case 0xa1:
            addr = get_addr(indx);
            lda(addr);
            cycles = 6;
            break;
        case 0xa2:
            addr = get_addr(imm);
            ldx(addr);
            cycles = 2;
            break;
        case 0xa4:
            addr = get_addr(zp);
            ldy(addr);
            cycles = 3;
            break;
        case 0xa5:
            addr = get_addr(zp);
            lda(addr);
            cycles = 3;
            break;
        case 0xa6:
            addr = get_addr(zp);
            ldx(addr);
            cycles = 3;
            break;
        case 0xa8:
            y = a;
            set_flag(N,y&0x80);
            set_flag(Z,y==0);
            cycles = 2;
            break;
        case 0xa9:
            addr = get_addr(imm);
            lda(addr);
            cycles = 2;
            break;
        case 0xaa:
            x = a;
            set_flag(N,x&0x80);
            set_flag(Z,x==0);
            cycles = 2;
            break;
        case 0xac:
            addr = get_addr(abs);
            ldy(addr);
            cycles = 4;
            break;
        case 0xad:
            addr = get_addr(abs);
            lda(addr);
            cycles = 4;
            break;
        case 0xae:
            addr = get_addr(abs);
            ldx(addr);
            cycles = 4;
            break;
        case 0xb0:
            cycles = 2;
            if(get_flag(C)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0xb1:
            addr = get_addr(indy);
            lda(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0xb4:
            addr = get_addr(zpx);
            ldy(addr);
            cycles = 4;
            break;
        case 0xb5:
            addr = get_addr(zpx);
            lda(addr);
            cycles = 4;
            break;
        case 0xb6:
            addr = get_addr(zpy);
            ldx(addr);
            cycles = 4;
            break;
        case 0xb8:
            set_flag(V,false);
            cycles = 2;
            break;
        case 0xb9:
            addr = get_addr(absy);
            lda(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xba:
            x = sp;
            set_flag(N,x&0x80);
            set_flag(Z,x==0);
            cycles = 2;
            break;
        case 0xbc:
            addr = get_addr(absx);
            ldy(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xbd:
            addr = get_addr(absx);
            lda(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xbe:
            addr = get_addr(absy);
            ldx(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xc0:
            addr = get_addr(imm);
            cpy(addr);
            cycles = 2;
            break;
        case 0xc1:
            addr = get_addr(indx);
            cmp(addr);
            cycles = 6;
            break;
        case 0xc4:
            addr = get_addr(zp);
            cpy(addr);
            cycles = 3;
            break;
        case 0xc5:
            addr = get_addr(zp);
            cmp(addr);
            cycles = 3;
            break;
        case 0xc6:
            addr = get_addr(zp);
            dec(addr);
            cycles = 5;
            break;
        case 0xc8:
            iny();
            cycles = 2;
            break;
        case 0xc9:
            addr = get_addr(imm);
            cmp(addr);
            cycles = 2;
            break;
        case 0xca:
            dex();
            cycles = 2;
            break;
        case 0xcc:
            addr = get_addr(abs);
            cpy(addr);
            cycles = 4;
            break;
        case 0xcd:
            addr = get_addr(abs);
            cmp(addr);
            cycles = 4;
            break;
        case 0xce:
            addr = get_addr(abs);
            dec(addr);
            cycles = 6;
            break;
        case 0xd0:
            cycles = 2;
            if(!get_flag(Z)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0xd1:
            addr = get_addr(indy);
            cmp(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0xd5:
            addr = get_addr(zpx);
            cmp(addr);
            cycles = 4;
            break;
        case 0xd6:
            addr = get_addr(zpx);
            dec(addr);
            cycles = 6;
            break;
        case 0xd8:
            set_flag(D,false);
            cycles = 2;
            break;
        case 0xd9:
            addr = get_addr(absy);
            cmp(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xdd:
            addr = get_addr(absx);
            cmp(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xde:
            addr = get_addr(absx);
            dec(addr);
            cycles = 7;
            break;
        case 0xe0:
            addr = get_addr(imm);
            cpx(addr);
            cycles = 2;
            break;
        case 0xe1:
            addr = get_addr(indx);
            sbc(addr);
            cycles = 6;
            break;
        case 0xe4:
            addr = get_addr(zp);
            cpx(addr);
            cycles = 3;
            break;
        case 0xe5:
            addr = get_addr(zp);
            sbc(addr);
            cycles = 3;
            break;
        case 0xe6:
            addr = get_addr(zp);
            inc(addr);
            cycles = 5;
            break;
        case 0xe8:
            inx();
            cycles = 2;
            break;
        case 0xe9:
            addr = get_addr(imm);
            sbc(addr);
            cycles = 2;
            break;
        case 0xea:
            cycles = 2;
            break;
        case 0xec:
            addr = get_addr(abs);
            cpx(addr);
            cycles = 4;
            break;
        case 0xed:
            addr = get_addr(abs);
            sbc(addr);
            cycles = 4;
            break;
        case 0xee:
            addr = get_addr(abs);
            inc(addr);
            cycles = 6;
            break;
        case 0xf0:
            cycles = 2;
            if(get_flag(Z)){
                addr = get_addr(rel);
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0xf1:
            addr = get_addr(indy);
            sbc(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0xf5:
            addr = get_addr(zpx);
            sbc(addr);
            cycles = 4;
            break;
        case 0xf6:
            addr = get_addr(zpx);
            inc(addr);
            cycles = 6;
            break;
        case 0xf8:
            set_flag(D,true);
            cycles = 2;
            break;
        case 0xf9:
            addr = get_addr(absy);
            sbc(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xfd:
            addr = get_addr(absx);
            sbc(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0xfe:
            addr = get_addr(absx);
            inc(addr);
            cycles = 7;
            break;
        default:
            cycles = 0;
    }
    return cycles;
}