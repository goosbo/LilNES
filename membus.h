#include <cstdint>
#include <array>

class MemoryBus{
    private:
    std::array<uint8_t,2048> RAM;
    std::array<uint8_t,256> stk;

    public:
    MemoryBus();
    ~MemoryBus();

    void write_mem(uint16_t addr, uint8_t data);
    uint8_t read_mem(uint16_t addr);

    uint8_t pop_stk();
    void push_stk(uint8_t data);
};