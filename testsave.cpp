#include "pch.h"
#include <stdio.h>


const char *message = "HELLO WORLD!";
uint8_t header[15] = { 0x01,0x08,0x0C,0x08,0x0A,0x00,0x9E,0x20,0x32,0x30,0x36,0x32,0x00,0x00,0x00 };
struct testsave : public emulate65c02
{
	void compile() 
	{
		compile_point = 0x7ff;
		for (int i = 0; i < sizeof(header); ++i) comp_byte(header[i]);

		Label chrout,chrin,data,loop,done;
	chrout.set_target(this, 0xFFD2);
	chrin.set_target(this, 0xFFCF);
		ldx_imm(0);
	loop.set_target(this);
		lda_abx(data);
		beq(done, true);
		jsr(chrout);
		inx();
		bra(loop);
	done.set_target(this);
		jsr(chrin);
		stp();
//		rts();
	data.set_target(this);
		int i = 0;
		do {
			comp_byte(message[i]);
		} while (message[i++] != 0);
		// trace = true;
		execute(2062);
//		FILE* file = fopen("hello.prg", "wb");
//		fwrite(map_addr(0x7ff), 1, compile_point - 0x7ff, file);
//		fclose(file);
	}
};

void do_compile()
{
	testsave temp;
	temp.compile();
}