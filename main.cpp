#include "raylib.h"
#include "cpu.h"
#include "membus.h"
#include "rom.h"
#include <iostream>


const int WIDTH = 256;
const int HEIGHT = 240;
const int SCALE = 3;

int main(int argc, char* argv[]){

    if (argc < 2) {
        std::cout << "run with rom path as argument";
        return 1;
    }
    std::string romPath = argv[1];
    InitWindow(WIDTH*SCALE, HEIGHT*SCALE, "lilNES");
    SetTargetFPS(60);
    PPU ppu;
    CPU cpu;
    MemoryBus membus;
    ROM rom;

    rom.load_rom(romPath);
    membus.attach_ppu(&ppu);
    membus.attach_rom(&rom);
    membus.attach_cpu(&cpu);
    cpu.attach_membus(&membus);
    ppu.attach_cpu(&cpu);
    ppu.attach_rom(&rom);
    cpu.reset();
    for(int i = 0; i < 21; i++) ppu.step();
    
    Image screenImg = {ppu.screen,WIDTH,HEIGHT,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    RenderTexture2D screenTexture = LoadRenderTexture(WIDTH, HEIGHT);
    Texture2D screenTex = LoadTextureFromImage(screenImg);
    
    while(!WindowShouldClose()){
        while(!ppu.framedone){
            //if (cpu.suspended == 0) cpu.log_state(); 
            
            int cpu_cycles = 0;
            if (cpu.suspended > 0){
                cpu.suspended--;
                cpu_cycles = 1;
            }
            else cpu_cycles = cpu.run_instr();
            cpu.total_cycles += cpu_cycles;

            for(int i = 0; i < cpu_cycles * 3; i++) ppu.step();
        }
        ppu.framedone = false;
        UpdateTexture(screenTex, ppu.screen);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            screenTex ,
            { 0, 0, (float)WIDTH, (float)HEIGHT }, 
            { 0, 0, (float)(WIDTH * SCALE), (float)(HEIGHT * SCALE) },
            { 0, 0 }, 
            0.0f, 
            WHITE
        );

        EndDrawing();
    }
    UnloadTexture(screenTex);
    CloseWindow();
    return 0;
}