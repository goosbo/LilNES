all:
	g++ src/*.cpp src/mapper/*.cpp -o lilNES.exe -lraylib -lgdi32 -lwinmm