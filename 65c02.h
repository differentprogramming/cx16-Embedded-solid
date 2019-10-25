#pragma once

struct emulate65c02;
typedef void INSTRUCTION(emulate65c02 *);
enum FLAGS {
	FLAG_C = 1,
	FLAG_Z = 2,
	FLAG_I = 4,
	FLAG_D = 8,
	FLAG_B = 16,
	FLAG_V = 64,
	FLAG_N = 128
};

enum WRITE_MODES { READ_MODE,WRITE_MODE,MODIFY_MODE, NUM_WRITE_MODES };
enum ADDRESSING_MODES { IMM, IZX, IZY, ZP, ABS, IZP, ABY, ABX, ZPX, ZPY, NUM_ADDRESSING_MODES};
enum JMP_MODES { JABS, JIND, JIAX };

int ADDR_DELAY[NUM_WRITE_MODES*NUM_ADDRESSING_MODES] =
{
	2,0,0, //imm 0 for this combination doesn't exist
	6,6,0, //izx
	5,6,0, //izy
	3,3,5, //zp
	4,4,6, //abs
	5,5,0, //izp
	4,5,0, //aby
	4,5,6, //abx dec and inc take 7 
};

struct emulate65c02 {
	int a, x, y, p, s;//empty stack decending
	int pc;
	uint8_t memory[65536];
	int compile_point;
	int data_point;
	long long time;
	bool waiting;
	bool stop;

	long long execute(int addr)
	{
		long long t = time;
		stop = waiting = false;
		pc = addr;
		for (;;) {
			instructions[*map_addr(pc)](this);
			if (waiting || stop) break;
			++pc;
		}
		return time - t;
	}

	//for JMP and JSR
	void decode_jmp(JMP_MODES jmode)
	{
		time += 3;
		int add;
		add = deref_abs(pc+1);
		switch (jmode)
		{
		case JABS:
			pc = (add - 1) & 0xffff;
			break;
		case JIND:
			pc = (deref_abs(add) - 1) & 0xffff;
			time += 2;
			break;
		case JIAX:
			pc = (deref_abs(add+x) - 1) & 0xffff;
			time += 3;
		}
	}
	int decode_branch(int *if_taken)
	{
		time += 2;
		int add;
		pc = ((pc + 1) & 0xffff);
		add = *map_addr(pc);
		if (0 != (add & 0x80)) add |= 0xffffff00;
		time += 2;
		if (0 != (((pc - 1) ^ (pc + add + 1)) & 0x100)) *if_taken = 2;
		else *if_taken = 1;
		return (pc + add + 1) & 0xffff;
	}
	uint8_t *decode_addr(WRITE_MODES wm,ADDRESSING_MODES am)
	{
		int delay = ADDR_DELAY[(int)wm + (int)am*(int)NUM_WRITE_MODES];
		if (delay == 0) throw "impossible addressing and write mode combination";
		time += delay;
		int add;
		switch (am) {
			case IMM: 
				pc = ((pc + 1) & 0xffff);
				return map_addr(pc);
			case  IZX: 
				pc = ((pc + 1) & 0xffff);
				return map_addr(deref_zp(*map_addr(pc)+x));
			case  IZY:
				pc = ((pc + 1) & 0xffff);
				add = deref_zp(*map_addr(pc));
				if (0 != ((add ^ (add + 1 + y)) & 0x100))++delay;
				return map_addr(add+y);
			case  IZP:
				pc = ((pc + 1) & 0xffff);
				return map_addr(deref_zp(*map_addr(pc)));
			case  ABS:
				pc = ((pc + 2) & 0xffff);
				return map_addr(*map_addr(pc - 1)+ (*map_addr(pc)<<8));
			case  ZP:
				pc = ((pc + 1) & 0xffff);
				return map_addr(*map_addr(pc));
			case  ABY: 
				pc = ((pc + 2) & 0xffff);
				add = deref_abs(pc-1);
				if (0 != ((add ^ (add + 1 + y)) & 0x100))++delay;
				return map_addr(add + y);
			case  ABX:
				pc = ((pc + 2) & 0xffff);
				add = deref_abs(pc-1);
				if (0 != ((add ^ (add + 1 + x)) & 0x100))++delay;
				return map_addr(add + x);
			case  ZPY:
				pc = ((pc + 1) & 0xffff);
				add = *map_addr(pc);
				return map_addr(0xff&(add + y));
			case  ZPX:
				pc = ((pc + 1) & 0xffff);
				add = *map_addr(pc);
				return map_addr(0xff & (add + x));
		}
	}
	static int sign_extend(int n)
	{
		if (0 != (n & 0x80))return n | 0xffffff00;
		return n&0xff;
	}
	void do_adc(int u)
	{
		int s = sign_extend(u);
		int sa = sign_extend(a);
		u &= 0xff;
		a &= 0xff;
		const int c=(p&(int)FLAG_C);

		a += c + u;
		sa += c + s;

		if (0 != (a & 0x100)) p |= (int)FLAG_C;
		else p &= ~(int)FLAG_C;

		if ((0 != (sa & 0x80000000)) != (0 != (sa & 0x00000080)))p |= (int)FLAG_V;
		else p &= ~(int)FLAG_V;
		a &= 0xff;
		test_for_N(test_for_Z(a));
	}

	void carry_from_shift_bit(int v)
	{
		if (0 != v) p |= (int)FLAG_C;
		else p &= ~(int)FLAG_C;
	}

	void do_sbc(int u)
	{
		int s = sign_extend(u);
		int sa = sign_extend(a);
		u &= 0xff;
		a &= 0xff;
		const int c = (p&(int)FLAG_C);

		a -= 1-c + u;
		sa -= 1-c + s;

		if (0 == (a & 0x100)) p |= (int)FLAG_C;
		else p &= ~(int)FLAG_C;

		if ((0 != (sa & 0x80000000)) != (0 != (sa & 0x00000080)))p |= (int)FLAG_V;
		else p &= ~(int)FLAG_V;
		a &= 0xff;
		test_for_N(test_for_Z(a));
	}
	void do_cmp(int o, int u)
	{
		u &= 0xff;
		o &= 0xff;
	
		o -= u;

		if (0 == (o & 0x100)) p |= (int)FLAG_C;
		else p &= ~(int)FLAG_C;

		o &= 0xff;
		test_for_N(test_for_Z(o));
	}


	uint8_t* map_addr(int addr)
	{
		//add bank switching later
		&memory[addr & 0xffff];
	}
	int deref_abs(int addr)
	{
		*map_addr(addr) + (*map_addr(addr + 1) << 8);
	}
	int deref_zp(int addr)
	{
		addr &= 0xff;
		//if (0!=(addr&&~0xff)) throw "derefed non-zp address as zp";
		*map_addr(addr) + (*map_addr((addr + 1)&&0xff) << 8);
	}
	static int stack_mask(int stack)
	{
		return (stack&0xff)+0x100;
	}
	int deref_stack(int addr)
	{
		if (0 == (addr&&0x1) || 0 != (addr && ~0x1ff)) throw "derefed non-stack address as stack";
		*map_addr(addr) + (*map_addr(stack_mask(addr + 1)) << 8);
	}
	void set_mem(int addr, int v)
	{
		*map_addr(addr) = (uint8_t)v;
	}
	uint8_t get_mem(int addr)
	{
		return *map_addr(addr);
	}

	void push_byte(int v)
	{
		set_mem(s-- + 0x100, v);
		s &= 0xff;
	}
	void push_word(int v) {
		push_byte(v>>8);
		push_byte(v);
	}
	uint8_t pop_byte()
	{
		s= (s+1 & 0xff);
		return get_mem(s + 0x100);
	}
	uint16_t pop_word()
	{
		uint8_t t = pop_byte();
		return (pop_byte() << 8) + t;
	}
	uint16_t zp_add(int i) {
		return get_mem(i) + (get_mem((i + 1) & 0xff) << 8);
	}
	emulate65c02() :a(0), x(0), y(0), p((int)FLAG_I), s(0xff), pc(0x100),compile_point(0x100), data_point(0x8000),
		waiting(false), stop(false)
	{
	}
	void comp_byte(uint8_t v) { memory[compile_point++] = v; }
	void _brk() { comp_byte(0); }
	void _ora_ix(int v) { comp_byte(1); comp_byte(v); }
	//won't be an entry for every nop
	void _nop_imm(int v) { comp_byte(2); comp_byte(v); }
	void _nop() { comp_byte(3); }
	void _tsb_z(int v) { comp_byte(4); comp_byte(v); }
	void _ora_z(int v) { comp_byte(5); comp_byte(v); }
	void _asl_z(int v) { comp_byte(6); comp_byte(v); }

	void reset() {
		time += 7;
		p &= ~(int)FLAG_B;
		pc = (get_mem(0xfffd) << 8) + get_mem(0xfffc);
	}
	void irq() {
		if ((p&(int)FLAG_I) != 0) {
			push_word(pc);
			push_byte(p);
			time += 7;
			p &= ~(int)FLAG_B;
			pc = (get_mem(0xffff) << 8) + get_mem(0xfffe);
		}
	}
	void nmi() {
		push_word(pc);
		push_byte(p);
		time += 7;
		p &= ~(int)FLAG_B;
		pc = (get_mem(0xfffb) << 8) + get_mem(0xfffa);
	}
	int test_for_Z(int v) {
		if ((v&0xff) == 0) p |= (int)FLAG_Z; else p &= ~(int)FLAG_Z;
		return v;
	}
	int test_for_N(int v) {
		if ((v & 0x80) != 0) p |= (int)FLAG_N; else p &= ~(int)FLAG_N;
		return v;
	}
	static INSTRUCTION * instructions[256];

};

