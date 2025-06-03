#include "../include/vm_debug.h"

void fprintf_binary(FILE *f, uint16_t num) {
	int c = 16;
	while (c-- > 0) {
		if ((c + 1) % 4 == 0) fprintf(f, " ");
		fprintf(f, "%d", (num >> c) & 1);
	}
}

void fprintf_instruction(FILE *f, uint16_t instruction) {
	fprintf(f, "instruction=%u, binary=", instruction);
	fprintf_binary(f, instruction);
	fprintf(f, "\n");
}

void fprintf_memory(FILE *f, uint16_t *memory, uint16_t from, uint16_t to) {
	for (int i = from; i < to; i++) {
		fprintf(f, "mem[%d|0x%.04x]=", i, i);
		fprintf_binary(f, memory[i]);
		fprintf(f, "\n");
	}
}

void fprintf_memory_nonzero(FILE *f, uint16_t *memory, uint16_t stop) {
	for (int i = 0; i < stop; i++) {
		if (memory[i] != 0) {
			fprintf(f, "mem[%d|0x%.04x]=", i, i);
			fprintf_binary(f, memory[i]);
			fprintf(f, "\n");
		}
	}
}

void fprintf_register(FILE *f, uint16_t *reg, int index) {
	fprintf(stdout, "reg[%d]=0x%.04x\n", index, reg[index]);
}

void fprintf_register_all(FILE *f, uint16_t *reg, int size) {
	for (int i = 0; i < size; i++) {
		fprintf_register(f, reg, i);
	}
}
