#include "rom.h"
#include <iostream>
#include "mapper/mapper0.h"
#include "mapper/mapper1.h"
#include "mapper/mapper3.h"

ROM::ROM(){
    prg_size = 0;
    chr_size = 0;
}

bool ROM::load_rom(const std::string& rom_path){
    std::ifstream rom(rom_path,std::ios::binary);
    if(!rom.is_open()){
        std::cout<<"ROM path invalid\n";
        return false;
    }

    ROM_header header;
    rom.read((char*)&header,sizeof(char)*16);

    if(header.magic_bytes[0] != 'N' || header.magic_bytes[1] != 'E' || header.magic_bytes[2] != 'S' || header.magic_bytes[3] != 0x1a){
        std::cout<<"invalid rom magic bytes\n";
        return false;
    }

    prg_size = header.prg_size;
    chr_size = header.chr_size;
    mapper_id = (header.mapper2&0xf0)|(header.mapper1>>4);
    mirroring = (header.mapper1&1)? VERTICAL : HORIZONTAL;

    if(header.mapper1&4)rom.seekg(512,std::ios::cur); 
    prg_rom.resize(prg_size*16384);
    rom.read((char*)prg_rom.data(),prg_rom.size());
    if(chr_size>0){
        chr_rom.resize(chr_size*8192);
        rom.read((char*)chr_rom.data(),chr_rom.size());
    }
    else chr_rom.resize(8192);
    prg_ram.resize(8192,0x00);
    rom.close();

    switch(mapper_id){
        case 0:
            mapper = std::make_unique<Mapper0>(prg_size,chr_size);
            break;
        case 1:
            mapper = std::make_unique<Mapper1>(prg_size,chr_size);
            break;
        case 3:
            mapper = std::make_unique<Mapper3>(prg_size,chr_size);
            break;
        default:
            std::cout<<"mapper not supported: "<<(int)mapper_id<<"\n";
            return false;
    }

    return true; 
}

bool ROM::cpu_read(uint16_t addr, uint8_t &data){
    uint32_t mapaddr = 0;
    if(mapper->cpu_read(addr,mapaddr)){
        data = prg_rom[mapaddr];
        return true;
    }
    if(addr >= 0x6000 && addr <= 0x7fff){
        data = prg_ram[addr&0x1fff];
        return true;
    }
    return false;
}

bool ROM::cpu_write(uint16_t addr, uint8_t data){
    uint32_t mapaddr = 0;
    if(mapper->cpu_write(addr,mapaddr,data)) return true;
    if(addr >= 0x6000 && addr <= 0x7fff){
        prg_ram[addr&0x1fff] = data;
        return true;
    }
    return false;
}

bool ROM::ppu_read(uint16_t addr, uint8_t &data){
    uint32_t mapaddr = 0;
    if(mapper->ppu_read(addr,mapaddr)){
        data = chr_rom[mapaddr];
        return true;
    }
    return false;
}

bool ROM::ppu_write(uint16_t addr, uint8_t data){
    uint32_t mapaddr = 0;
    if(mapper->ppu_write(addr,mapaddr)){
        chr_rom[mapaddr] = data;
        return true;
    }
    return false;
}