#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint16_t program[] = {
	0xF026,	// 1111 0000 0010 0110		TRAP trp_in_u16		; Read an uint16_t from stdin and put it in R0
	0x1220,	// 0001 0010 0010 0000		ADD R1,R0,x0		; Add contents of R0 to R1
	0xF026,	// 1111 0000 0010 0110		TRAP trp_in_u16		; Read an uint16_t from stdin and put it in R0
	0x1240,	// 0001 0010 0100 0000		ADD R1,R1,R0		; Add contents of R0 to R1
	0x1060,	// 0001 0000 0110 0000		ADD R0,R1,x0		; Add contents of R1 to R0
	0xF027,	// 1111 0000 0010 0111		TRAP trp_out_u16	; Show contents of R0 to stdout
	0xF025,	// 1111 0000 0010 0101		HALT			; Halt
};

int main() {
	char *outf = "sum.obj";
	FILE *f = fopen(outf, "wb");
	if (f == NULL) {
		fprintf(stderr, "Cannot write to file %s\n", outf);
		exit(1);
	}
	size_t write = fwrite(program, sizeof(uint16_t), sizeof(program), f);
	fprintf(stdout, "Written size_t=%lu to file %s\n", write, outf);
	fclose(f);
	return 0;
}
