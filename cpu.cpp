#include "cpu.h"
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <sstream>

CPU::CPU(){
    a = 0x00;
    x = 0x00;
    y = 0x00;
    sp = 0xFD;
    status = 0x24;
    pc = 0xC000;
    page_crossed = false;
    total_cycles = 7;
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
    uint16_t lo,hi,base;
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
        base = bus->read_mem(pc++);
        lo = bus->read_mem((uint16_t)(base+(uint16_t)x)&0xff);
        hi = bus->read_mem((uint16_t)(base+(uint16_t)x+1)&0xff);
        addr = (hi << 8)|lo;
        break;
    case indy:
        base = bus->read_mem(pc++);
        lo = bus->read_mem(base&0xff);
        hi = bus->read_mem((base+1)&0xff);
        addr = ((hi << 8)|lo)+y;
        if ((addr & 0xFF00) != (hi << 8)) page_crossed = true;
        break;
    case _abs:
        lo = bus->read_mem(pc++);
        hi = bus->read_mem(pc++);
        addr = (hi << 8)|lo;
        break;
    case absx:
        lo = bus->read_mem(pc++);
        hi = bus->read_mem(pc++);
        addr = ((hi << 8)|lo) + (uint16_t)x;
        if((addr&0xff00) != (hi<<8)) page_crossed = true;
        break;
    case absy:
        lo = bus->read_mem(pc++);
        hi = bus->read_mem(pc++);
        addr = ((hi << 8)|lo) + (uint16_t)y;
        if((addr&0xff00) != (hi<<8)) page_crossed = true;
        break;
    case rel:{
        uint16_t rel_offset = bus->read_mem(pc++);
        if(rel_offset&0x80) rel_offset |= 0xff00;
        addr = pc + (int16_t)rel_offset;
        if((addr & 0xff00) != (pc & 0xff00)) page_crossed = true;
        break;
    }
    case ind:{
        lo = bus->read_mem(pc++);
        hi = bus->read_mem(pc++);
        uint16_t ind_addr = (hi<<8)|lo;
        if((ind_addr & 0xff) == 0xff) addr = ((uint16_t)bus->read_mem(ind_addr&0xff00)<<8)|bus->read_mem(ind_addr);
        else addr = ((uint16_t)bus->read_mem(ind_addr+1)<<8)|bus->read_mem(ind_addr);
        break;
    }
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

void CPU::_and(uint16_t addr){
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
    total_cycles += 7;
}

void CPU::nmi(){
    push((pc >> 8)&0xff);
    push(pc&0xff);
    set_flag(B,false);
    set_flag(U,true);
    push(status);
    set_flag(I,true);
    uint16_t lo = bus->read_mem(0xfffa);
    uint16_t hi = bus->read_mem(0xfffb);
    pc = (hi << 8)|lo;
    total_cycles+=7;
}

void CPU::irq(){
    if(get_flag(I)) return;
    push((pc >> 8)&0xff);
    push(pc&0xff);
    set_flag(B,false);
    set_flag(U,true);
    push(status);
    set_flag(I,true);
    uint16_t lo = bus->read_mem(0xfffe);
    uint16_t hi = bus->read_mem(0xffff);
    pc = (hi << 8)|lo;
    total_cycles+=7;
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
    set_flag(Z,(res&0xff) == 0);
    set_flag(N,res&0x80);
    set_flag(V, (~((uint16_t)a ^ (uint16_t)mem) & ((uint16_t)a ^ (uint16_t)res)) & 0x0080);
    a = res & 0xff;
}

int CPU::run_instr(){
    uint16_t addr;
    uint16_t op = bus->read_mem(pc++);
    uint8_t val;
    uint16_t lo,hi;
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
            addr = get_addr(_abs);
            ora(addr);
            cycles = 4;
            break;
        case 0x0e:
            addr = get_addr(_abs);
            asl(addr,_abs);
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
            addr = get_addr(_abs);
            push(((pc-1)>>8)&0xff);
            push((pc-1)&0xff);
            pc = addr;
            cycles = 6;
            break;
        case 0x21:
            addr = get_addr(indx);
            _and(addr);
            cycles = 6;
            break;
        case 0x24:
            addr = get_addr(zp);
            bit(addr);
            cycles = 3;
            break;
        case 0x25:
            addr = get_addr(zp);
            _and(addr);
            cycles = 3;
            break;
        case 0x26:
            addr = get_addr(zp);
            rol(addr,zp);
            cycles = 5;
            break;
        case 0x28:
            val = pop();
            status = val;
            set_flag(U,true);
            set_flag(B,false);
            cycles = 4;
            break;
        case 0x29:
            addr = get_addr(imm);
            _and(addr);
            cycles = 2;
            break;
        case 0x2a:
            rol(0,acc);
            cycles = 2;
            break;
        case 0x2c:
            addr = get_addr(_abs);
            bit(addr);
            cycles = 4;
            break;
        case 0x2d:
            addr = get_addr(_abs);
            _and(addr);
            cycles = 4;
            break;
        case 0x2e:
            addr = get_addr(_abs);
            rol(addr,_abs);
            cycles = 6;
            break;
        case 0x30:
            addr = get_addr(rel);
            cycles = 2;
            if(get_flag(N)){    
                pc = addr;
                if(page_crossed)cycles++;
                cycles++;
            }
            break;
        case 0x31:
            addr = get_addr(indy);
            _and(addr);
            cycles = 5;
            if(page_crossed)cycles++;
            break;
        case 0x35:
            addr = get_addr(zpx);
            _and(addr);
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
            _and(addr);
            cycles = 4;
            if(page_crossed)cycles++;
            break;
        case 0x3d:
            addr = get_addr(absx);
            _and(addr);
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
            lo = pop();   
            hi = pop();
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
            addr = get_addr(_abs);
            pc = addr;
            cycles = 3;
            break;
        case 0x4d:
            addr = get_addr(_abs);
            eor(addr);
            cycles = 4;
            break;
        case 0x4e:
            addr = get_addr(_abs);
            lsr(addr,_abs);
            cycles = 6;
            break; 
        case 0x50:
            addr = get_addr(rel);
            cycles = 2;
            if(!get_flag(V)){
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
            lo = pop();   
            hi = pop();
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
            addr = get_addr(_abs);
            adc(addr);
            cycles = 4;
            break;
        case 0x6e:
            addr = get_addr(_abs);
            ror(addr,_abs);
            cycles = 6;
            break;
        case 0x70:
            addr = get_addr(rel);
            cycles = 2;
            if(get_flag(V)){
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
            addr = get_addr(_abs);
            bus->write_mem(addr,y);
            cycles = 4;
            break;
        case 0x8d:
            addr = get_addr(_abs);
            bus->write_mem(addr,a);
            cycles = 4;
            break;
        case 0x8e:
            addr = get_addr(_abs);
            bus->write_mem(addr,x);
            cycles = 4;
            break;
        case 0x90:
            addr = get_addr(rel);
            cycles = 2;
            if(!get_flag(C)){
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
            addr = get_addr(_abs);
            ldy(addr);
            cycles = 4;
            break;
        case 0xad:
            addr = get_addr(_abs);
            lda(addr);
            cycles = 4;
            break;
        case 0xae:
            addr = get_addr(_abs);
            ldx(addr);
            cycles = 4;
            break;
        case 0xb0:
            addr = get_addr(rel);
            cycles = 2;
            if(get_flag(C)){    
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
            addr = get_addr(_abs);
            cpy(addr);
            cycles = 4;
            break;
        case 0xcd:
            addr = get_addr(_abs);
            cmp(addr);
            cycles = 4;
            break;
        case 0xce:
            addr = get_addr(_abs);
            dec(addr);
            cycles = 6;
            break;
        case 0xd0:
            addr = get_addr(rel);
            cycles = 2;
            if(!get_flag(Z)){
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
            addr = get_addr(_abs);
            cpx(addr);
            cycles = 4;
            break;
        case 0xed:
            addr = get_addr(_abs);
            sbc(addr);
            cycles = 4;
            break;
        case 0xee:
            addr = get_addr(_abs);
            inc(addr);
            cycles = 6;
            break;
        case 0xf0:
            addr = get_addr(rel);
            cycles = 2;
            if(get_flag(Z)){
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

void CPU::attach_membus(MemoryBus* m){
    bus = m;
}



/* debug output for testing with nestest.nes

std::string hex(uint32_t n, uint8_t d) {
    std::string s(d, '0');
    for (int i = d - 1; i >= 0; i--, n >>= 4)
        s[i] = "0123456789ABCDEF"[n & 0xF];
    return s;
}

void CPU::log_state() {
    uint8_t opcode = bus->read_mem(pc);
    uint8_t lo = bus->read_mem(pc + 1);
    uint8_t hi = bus->read_mem(pc + 2);

    struct Instr { std::string name; addr_mode mode; int len; };
    static Instr lookup[256];
    static bool init = false;

    if (!init) {
        for (int i = 0; i < 256; i++) lookup[i] = {"???", imp, 1};

        lookup[0xA9] = {"LDA", imm, 2}; lookup[0xA5] = {"LDA", zp, 2};  lookup[0xB5] = {"LDA", zpx, 2}; lookup[0xAD] = {"LDA", _abs, 3}; lookup[0xBD] = {"LDA", absx, 3}; lookup[0xB9] = {"LDA", absy, 3}; lookup[0xA1] = {"LDA", indx, 2}; lookup[0xB1] = {"LDA", indy, 2};
        lookup[0xA2] = {"LDX", imm, 2}; lookup[0xA6] = {"LDX", zp, 2};  lookup[0xB6] = {"LDX", zpy, 2}; lookup[0xAE] = {"LDX", _abs, 3}; lookup[0xBE] = {"LDX", absy, 3};
        lookup[0xA0] = {"LDY", imm, 2}; lookup[0xA4] = {"LDY", zp, 2};  lookup[0xB4] = {"LDY", zpx, 2}; lookup[0xAC] = {"LDY", _abs, 3}; lookup[0xBC] = {"LDY", absx, 3};
        lookup[0x85] = {"STA", zp, 2};  lookup[0x95] = {"STA", zpx, 2}; lookup[0x8D] = {"STA", _abs, 3}; lookup[0x9D] = {"STA", absx, 3}; lookup[0x99] = {"STA", absy, 3}; lookup[0x81] = {"STA", indx, 2}; lookup[0x91] = {"STA", indy, 2};
        lookup[0x86] = {"STX", zp, 2};  lookup[0x96] = {"STX", zpy, 2}; lookup[0x8E] = {"STX", _abs, 3};
        lookup[0x84] = {"STY", zp, 2};  lookup[0x94] = {"STY", zpx, 2}; lookup[0x8C] = {"STY", _abs, 3};
        lookup[0xAA] = {"TAX", imp, 1}; lookup[0xA8] = {"TAY", imp, 1}; lookup[0x8A] = {"TXA", imp, 1}; lookup[0x98] = {"TYA", imp, 1}; lookup[0xBA] = {"TSX", imp, 1}; lookup[0x9A] = {"TXS", imp, 1};
        lookup[0x48] = {"PHA", imp, 1}; lookup[0x68] = {"PLA", imp, 1}; lookup[0x08] = {"PHP", imp, 1}; lookup[0x28] = {"PLP", imp, 1};
        lookup[0x69] = {"ADC", imm, 2}; lookup[0x65] = {"ADC", zp, 2}; lookup[0x75] = {"ADC", zpx, 2}; lookup[0x6D] = {"ADC", _abs, 3}; lookup[0x7D] = {"ADC", absx, 3}; lookup[0x79] = {"ADC", absy, 3}; lookup[0x61] = {"ADC", indx, 2}; lookup[0x71] = {"ADC", indy, 2};
        lookup[0xE9] = {"SBC", imm, 2}; lookup[0xE5] = {"SBC", zp, 2}; lookup[0xF5] = {"SBC", zpx, 2}; lookup[0xED] = {"SBC", _abs, 3}; lookup[0xFD] = {"SBC", absx, 3}; lookup[0xF9] = {"SBC", absy, 3}; lookup[0xE1] = {"SBC", indx, 2}; lookup[0xF1] = {"SBC", indy, 2};
        lookup[0x29] = {"AND", imm, 2}; lookup[0x25] = {"AND", zp, 2}; lookup[0x35] = {"AND", zpx, 2}; lookup[0x2D] = {"AND", _abs, 3}; lookup[0x3D] = {"AND", absx, 3}; lookup[0x39] = {"AND", absy, 3}; lookup[0x21] = {"AND", indx, 2}; lookup[0x31] = {"AND", indy, 2};
        lookup[0x49] = {"EOR", imm, 2}; lookup[0x45] = {"EOR", zp, 2}; lookup[0x55] = {"EOR", zpx, 2}; lookup[0x4D] = {"EOR", _abs, 3}; lookup[0x5D] = {"EOR", absx, 3}; lookup[0x59] = {"EOR", absy, 3}; lookup[0x41] = {"EOR", indx, 2}; lookup[0x51] = {"EOR", indy, 2};
        lookup[0x09] = {"ORA", imm, 2}; lookup[0x05] = {"ORA", zp, 2}; lookup[0x15] = {"ORA", zpx, 2}; lookup[0x0D] = {"ORA", _abs, 3}; lookup[0x1D] = {"ORA", absx, 3}; lookup[0x19] = {"ORA", absy, 3}; lookup[0x01] = {"ORA", indx, 2}; lookup[0x11] = {"ORA", indy, 2};
        lookup[0xC9] = {"CMP", imm, 2}; lookup[0xC5] = {"CMP", zp, 2}; lookup[0xD5] = {"CMP", zpx, 2}; lookup[0xCD] = {"CMP", _abs, 3}; lookup[0xDD] = {"CMP", absx, 3}; lookup[0xD9] = {"CMP", absy, 3}; lookup[0xC1] = {"CMP", indx, 2}; lookup[0xD1] = {"CMP", indy, 2};
        lookup[0xE0] = {"CPX", imm, 2}; lookup[0xE4] = {"CPX", zp, 2}; lookup[0xEC] = {"CPX", _abs, 3};
        lookup[0xC0] = {"CPY", imm, 2}; lookup[0xC4] = {"CPY", zp, 2}; lookup[0xCC] = {"CPY", _abs, 3};
        lookup[0x24] = {"BIT", zp, 2};  lookup[0x2C] = {"BIT", _abs, 3};
        lookup[0xE6] = {"INC", zp, 2};  lookup[0xF6] = {"INC", zpx, 2}; lookup[0xEE] = {"INC", _abs, 3}; lookup[0xFE] = {"INC", absx, 3};
        lookup[0xC6] = {"DEC", zp, 2};  lookup[0xD6] = {"DEC", zpx, 2}; lookup[0xCE] = {"DEC", _abs, 3}; lookup[0xDE] = {"DEC", absx, 3};
        lookup[0xE8] = {"INX", imp, 1}; lookup[0xC8] = {"INY", imp, 1}; lookup[0xCA] = {"DEX", imp, 1}; lookup[0x88] = {"DEY", imp, 1};
        lookup[0x0A] = {"ASL", acc, 1}; lookup[0x06] = {"ASL", zp, 2};  lookup[0x16] = {"ASL", zpx, 2}; lookup[0x0E] = {"ASL", _abs, 3}; lookup[0x1E] = {"ASL", absx, 3};
        lookup[0x4A] = {"LSR", acc, 1}; lookup[0x46] = {"LSR", zp, 2};  lookup[0x56] = {"LSR", zpx, 2}; lookup[0x4E] = {"LSR", _abs, 3}; lookup[0x5E] = {"LSR", absx, 3};
        lookup[0x2A] = {"ROL", acc, 1}; lookup[0x26] = {"ROL", zp, 2};  lookup[0x36] = {"ROL", zpx, 2}; lookup[0x2E] = {"ROL", _abs, 3}; lookup[0x3E] = {"ROL", absx, 3};
        lookup[0x6A] = {"ROR", acc, 1}; lookup[0x66] = {"ROR", zp, 2};  lookup[0x76] = {"ROR", zpx, 2}; lookup[0x6E] = {"ROR", _abs, 3}; lookup[0x7E] = {"ROR", absx, 3};
        lookup[0x4C] = {"JMP", _abs, 3}; lookup[0x6C] = {"JMP", ind, 3};
        lookup[0x20] = {"JSR", _abs, 3}; lookup[0x60] = {"RTS", imp, 1};
        lookup[0x00] = {"BRK", imp, 1};  lookup[0x40] = {"RTI", imp, 1};
        lookup[0x10] = {"BPL", rel, 2}; lookup[0x30] = {"BMI", rel, 2};
        lookup[0x50] = {"BVC", rel, 2}; lookup[0x70] = {"BVS", rel, 2};
        lookup[0x90] = {"BCC", rel, 2}; lookup[0xB0] = {"BCS", rel, 2};
        lookup[0xF0] = {"BEQ", rel, 2}; lookup[0xD0] = {"BNE", rel, 2};
        lookup[0xEA] = {"NOP", imp, 1};
        lookup[0x18] = {"CLC", imp, 1}; lookup[0x38] = {"SEC", imp, 1};
        lookup[0x58] = {"CLI", imp, 1}; lookup[0x78] = {"SEI", imp, 1};
        lookup[0xB8] = {"CLV", imp, 1}; lookup[0xD8] = {"CLD", imp, 1}; lookup[0xF8] = {"SED", imp, 1};
        init = true;
    }

    Instr inst = lookup[opcode];
    std::stringstream hex_str;
    hex_str << hex(opcode, 2);
    if (inst.len > 1) hex_str << " " << hex(lo, 2);
    if (inst.len > 2) hex_str << " " << hex(hi, 2);

    std::stringstream asm_str;
    asm_str << inst.name << " ";

    if (inst.mode == imm) {
        asm_str << "#$" << hex(lo, 2);
    } 
    else if (inst.mode == zp) {
        asm_str << "$" << hex(lo, 2) << " = " << hex(bus->read_mem(lo), 2);
    } 
    else if (inst.mode == zpx) {
        asm_str << "$" << hex(lo, 2) << ",X @ " << hex((lo+x)&0xFF, 2) << " = " << hex(bus->read_mem((lo+x)&0xFF), 2);
    }
    else if (inst.mode == zpy) {
        asm_str << "$" << hex(lo, 2) << ",Y @ " << hex((lo+y)&0xFF, 2) << " = " << hex(bus->read_mem((lo+y)&0xFF), 2);
    }
    else if (inst.mode == _abs) {
        uint16_t addr = (hi << 8) | lo;
        if(inst.name == "JMP" || inst.name == "JSR")
             asm_str << "$" << hex(addr, 4);
        else 
             asm_str << "$" << hex(addr, 4) << " = " << hex(bus->read_mem(addr), 2);
    } 
    else if (inst.mode == absx) {
         uint16_t addr = (hi << 8) | lo;
         asm_str << "$" << hex(addr, 4) << ",X @ " << hex((addr+x)&0xFFFF, 4) << " = " << hex(bus->read_mem((addr+x)&0xFFFF), 2);
    }
    else if (inst.mode == absy) {
         uint16_t addr = (hi << 8) | lo;
         asm_str << "$" << hex(addr, 4) << ",Y @ " << hex((addr+y)&0xFFFF, 4) << " = " << hex(bus->read_mem((addr+y)&0xFFFF), 2);
    }
    else if (inst.mode == ind) {
         uint16_t addr = (hi << 8) | lo;
         asm_str << "($" << hex(addr, 4) << ")";
    }
    else if (inst.mode == indx) {
         asm_str << "($" << hex(lo, 2) << ",X) @ " << hex((lo+x)&0xFF, 2) << " = " << hex(bus->read_mem((lo+x)&0xFF), 4);
    }
    else if (inst.mode == indy) {
         asm_str << "($" << hex(lo, 2) << "),Y = " << hex(bus->read_mem((lo+y)&0xFF), 4);
    }
    else if (inst.mode == rel) {
        int8_t offset = (int8_t)lo;
        uint16_t target = pc + 2 + offset;
        asm_str << "$" << hex(target, 4);
    }

    std::cout << hex(pc, 4) << "  ";
    
    std::string s_hex = hex_str.str();
    std::cout << s_hex;
    for(int i=0; i < 10 - s_hex.length(); i++) std::cout << " "; 
    std::string s_asm = asm_str.str();
    std::cout << s_asm;
    for(int i=0; i < 32 - s_asm.length(); i++) std::cout << " "; 
    std::cout << "A:" << hex(a, 2) << " "
              << "X:" << hex(x, 2) << " "
              << "Y:" << hex(y, 2) << " "
              << "P:" << hex(status, 2) << " "
              << "SP:" << hex(sp, 2) << " "
              << "CYC:" << std::dec << total_cycles 
              << "\n";
}
*/