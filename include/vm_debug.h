#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void fprintf_binary(FILE *f, uint16_t num);
void fprintf_instruction(FILE *f, uint16_t instruction);
void fprintf_memory(FILE *f, uint16_t *memory, uint16_t from, uint16_t to);
void fprintf_memory_nonzero(FILE *f, uint16_t *memory, uint16_t stop);
void fprintf_register(FILE *f, uint16_t *reg, int index);
void fprintf_register_all(FILE *f, uint16_t *reg, int size);
