#ifndef VM_H
#define VM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/vm_debug.h"

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

// RCND (conditional register flag) helps us with branching
// 1 << 0 - last operation yielded a positive result
// 1 << 1 - last operation yielded zero
// 1 << 2 - last operation yielded a negative result
enum Flags {
	FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2
};

enum { trp_offset = 0x20 };

void ld_img(char *fname, uint16_t offset);
void start(uint16_t offset);

#endif
