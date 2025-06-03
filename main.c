#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Instructions have the same size as the memory, 16 bits.
// The first 4 bits are the OpCode of the instruction, and then depending
// on the instruction there are parameters encoded in the remaining 12 bits.
// Because OpCode are represented in 4 bits, the maximum number of instructions
// we can encode is 16.
#define NOPS 16
// OpCode of the instruction is represented by bits 15-12
#define OPC(i) (i >> 12)
#define FIMM(i) ((i >> 5) & 1)
// Destination Register (DR) is represented by bits 11-9
#define DR(i) ((i >> 9) & 0x7)
// Source Register 1 (SR1) is represented by bits 8-6
#define SR1(i) ((i >> 6) & 0x7)
// SR2 is represented by bits 2-0
#define SR2(i) (i & 0x7)
// IMM5 is represented by bits 4-0
#define IMM(i) (i & 0x1F)
#define FL(i) ((i >> 11) & 1)
// NZP flags represented by bits 11-9
#define FCND(i) ((i >> 9) & 0x7)
// TRAP vector represented by bits 7-0
#define TRP(i) (i & 0xFF)
// Sign extension for 5 bit immediate values
// LC3 operates with 16 bit registers, so we need to extend IMM5 to 16 bits
// preserving the sign
#define SEXTIMM(i) sext(IMM(i),5)
#define POFF(i) sext((i & 0x3F), 6)
#define POFF9(i) sext((i & 0x1FF), 9)
#define POFF11(i) sext((i & 0x7FF), 11)

typedef void (*op_ex_f)(uint16_t instruction);
typedef void (*trp_ex_f)();

static inline uint16_t sext(uint16_t n, int b) {
	return ((n >> (b - 1)) & 1) ?		// if the bth bit of n is 1 (number is negative)
		(n | (0xFFFF << b)) : n;	// fill up remaining bits with 1s else return the number
}

bool running = true;
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
   			SEXTIMM(i) :		// sign extend IMM5 and add to SR1 (add2)
   			registers[SR2(i)]);	// else add the value of SR2 to SR1 (add1)
	uf(DR(i));
}

// and instruction is similar to add and has two version, usage is based on
// value of fifth bit of the instruction
static inline void and(uint16_t i) {
	registers[DR(i)] = registers[SR1(i)] &
		(FIMM(i) ?
   			SEXTIMM(i) :
   			registers[SR2(i)]);
	uf(DR(i));
}

// not performs a bitwise complement on SR1 and stores the value in DR1
static inline void not(uint16_t i) {
	registers[DR(i)] = ~registers[SR1(i)];
	uf(DR(i));
}

// ld is used to load data from main memory to a destination register.
// The memory location is obtained by adding an offset to the RPC register, ld doesn't
// modify the RPC
static inline void ld(uint16_t i) {
	registers[DR(i)] = memread(registers[RPC] + POFF9(i));
	uf(DR(i));
}

// ldi (load indirect) is used to load data into registers, using intermediary addresses to access
// fat away memory areas
static inline void ldi(uint16_t i) {
	registers[DR(i)] = memread(memread(registers[RPC] + POFF9(i)));
	uf(DR(i));
}

// ldr (load base + offset) is used to load data into registers, it uses a different base
// (memory kept in a register) than RPC
// The macro SR1 is used to extract BASER (base register) from the instruction as they
// have the same bit positions (8-6)
static inline void ldr(uint16_t i) {
	registers[DR(i)] = memread(registers[SR1(i)] + POFF(i));
	uf(DR(i));
}

// lea (load effective address) is used to load memory addresses into registers
static inline void lea(uint16_t i) {
	registers[DR(i)] = registers[RPC] + POFF9(i);
	uf(DR(i));
}

// st is used to store the value of a given register to a memory location
// Flags are not updated because no computation is done on registers
static inline void st(uint16_t i) {
	memwrite(registers[RPC] + POFF9(i), registers[DR(i)]);
}

// sti (store indirect) uses an intermediary address from the main memory to intermediate
// the write, the secondary address contains the actual memory location where it writes
static inline void sti(uint16_t i) {
	memwrite(memread(registers[RPC] + POFF9(i)), registers[DR(i)]);
}

// str (store base + offset) uses a BASER a reference register, to which we add the offset
static inline void str(uint16_t i) {
	memwrite(registers[SR1(i)] + POFF(i), registers[DR(i)]);
}

// jmp (jump) makes our RPC jump to the location specified by the contents of BASER
static inline void jmp(uint16_t i) { registers[RPC] = registers[SR1(i)]; }

// jsr (jump to subroutine) is a control flow instruction that helps in implementing
// subroutines
// The eleventh bit of the instruction tells which version of jsr to use
static inline void jsr(uint16_t i) {
	registers[R7] = registers[RPC];
	registers[RPC] = (FL(i) ?
			registers[RPC] + POFF11(i):	// RPC + offset
			registers[SR1(i)]);		// base register
}

// br is similar to jsr, but branching only happens when some conditions are met
static inline void br(uint16_t i) {
	if (registers[RCND] & FCND(i))
		registers[RPC] += POFF9(i);	// if conditions are met, branch to offset
}

static inline void rti(uint16_t i) {}	// unused
static inline void res(uint16_t i) {}	// unused

static inline void tgetc() { registers[R0] = getchar(); }
static inline void tout() { fprintf(stdout, "%c", (char)registers[R0]); }

// Iterate through the memory location starting at R0 until 0x0000 is found
// and print each character in order
static inline void tputs() {
	uint16_t *ptr = memory + registers[R0];
	while (*ptr) {
		fprintf(stdout, "%c", (char)*ptr);
		ptr++;
	}
}

static inline void tin() {
	registers[R0] = getchar();
	fprintf(stdout, "%c", registers[R0]);
}

static inline void thalt() { running = false; }
static inline void tinu16() { fscanf(stdin, "%hu", &registers[R0]); }
static inline void toutu16() { fprintf(stdout, "%hu\n", registers[R0]); }
static inline void tputsp() { /* Not implemented */ }

enum { trp_offset = 0x20 };
trp_ex_f trp_ex[8] = {
	tgetc, tout, tputs, tin, tputsp, thalt, tinu16, toutu16
};

static inline void trap(uint16_t i) { trp_ex[TRP(i) - trp_offset](); }

op_ex_f op_ex[NOPS] = {
	br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap
};

void ld_img(char *fname, uint16_t offset) {
	FILE *in = fopen(fname, "rb");
	if (in == NULL) {
		fprintf(stderr, "Cannot open file %s\n", fname);
		exit(1);
	}

	uint16_t *ptr = memory + PC_START + offset;
	fread(ptr, sizeof(uint16_t), (UINT16_MAX - PC_START), in);
	fclose(in);
}

void start(uint16_t offset) {
	registers[RPC] = PC_START + offset;
	while (running) {
		// Extract instructions from the memory location pointed by RPC
		// and auto increment the RPC
		uint16_t i = memread(registers[RPC]++);
		op_ex[OPC(i)](i);
	}
}

int main(int argc, char* argv[]) {
	ld_img(argv[1], 0x0);
	start(0x0);
	return 0;
}
