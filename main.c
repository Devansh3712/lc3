#include <stdint.h>

// Instructions have the same size as the memory, 16 bits.
// The first 4 bits are the OpCode of the instruction, and then depending
// on the instruction there are parameters encoded in the remaining 12 bits.
// Because OpCode are represented in 4 bits, the maximum number of instructions
// we can encode is 16.
#define NOPS 16
#define OPC(i) (i >> 12)
typedef void (*op_ex_f)(uint16_t instruction);

#define FIMM(i) ((i >> 5) & 1)
// Destination Register (DR) is represented by bits 11-9
#define DR(i) ((i >> 9) & 0x7)
// Source Register 1 (SR1) is represented by bits 8-6
#define SR1(i) ((i >> 6) & 0x7)
// SR2 is represented by bits 2-0
#define SR2(i) (i & 0x7)
// IMM5 is represented by bits 4-0
#define IMM(i) (i & 0x1F)
// Sign extension for 5 bit immediate values
// LC3 operates with 16 bit registers, so we need to extend IMM5 to 16 bits
// preserving the sign
#define SEXTIMM(i) sext(IMM(i),5)

static inline uint16_t sext(uint16_t n, int b) {
	return ((n >> (b - 1)) & 1) ?		// if the bth bit of n is 1 (number is negative)
		(n | (0xFFFF << b)) : n;	// fill up remaining bits with 1s else return the number
}

// Lower memory (0x0000 to 0x2FFF) is often reserved for system use,
// interrupt vectors and hardware I/O mapping
uint16_t PC_START = 0x3000;
uint16_t memory[UINT16_MAX + 1] = {0};

// R0 is a general purpose register, used for reading/writing data from/to
// stdin/stdout
// R1..R7 are general purpose registers
// RPC is the program counter register, contains the memory address of the
// next instruction we will execute
// RCND is the conditional register, gives us information about the previous
// operation that happened at ALU level in the CPU
enum Register {
	R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, RCNT
};
uint16_t registers[RCNT] = {0};

// RCND (conditional register flag) helps us with branching
// 1 << 0 - last operation yielded a positive result
// 1 << 1 - last operation yielded zero
// 1 << 2 - last operation yielded a negative result
enum Flags {
	FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2
};

// NOTE: MSB is 1 for negative numbers
static inline void uf(enum Register r) {
	if (registers[r] == 0) registers[RCND] = FZ;		// the value in r is zero
	else if (registers[r]>>15) registers[RCND] = FN;	// the value in r is a negative number
	else registers[RCND] = FP;				// the value in r is a positive number
}

static inline uint16_t memread(uint16_t address) { return memory[address]; }
static inline void memwrite(uint16_t address, uint16_t value) { memory[address] = value; }

// The fifth bit of the instruction tells which version of and to use, if it is zero
// we implement add1 else add2
// add1 is used for adding values of two registers: SR1 and SR2 and storing
// their sum in DR1
// add2 is used to add a constant value (IMM5) to SR1 and store the result
// in DR1
static inline void add(uint16_t i) {
	registers[DR(i)] = registers[SR1(i)] +
		(FIMM(i) ?			// If the fifth bit is 1
   			SEXTIMM(i):		// sign extend IMM5 and add to SR1 (add2)
   			registers[SR2(i)]);	// else add the value of SR2 to SR1 (add1)
	uf(DR(i));
}
