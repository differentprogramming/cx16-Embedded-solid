#pragma once
#include <list>
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

extern int ADDR_DELAY[NUM_WRITE_MODES*NUM_ADDRESSING_MODES];

class emulate65c02;
struct LabelFixup
{
	bool relative;
	int  instruction_field_address;
	int target;
	LabelFixup():instruction_field_address(-1), target(-1){}
	LabelFixup(int f, bool r) :instruction_field_address(f), target(r) {}
	LabelFixup(const LabelFixup &) = default;
	LabelFixup(LabelFixup &&) = default;
	void  update_target(emulate65c02 *emulate, int t);
};

struct Label
{
	int target;
	Label() :target(-1), fixups(new std::list<LabelFixup>){}
	Label(const Label&) = default;
	Label(Label&&) = default;
	std::shared_ptr<std::list<LabelFixup> > fixups;

	bool has_target() { return target != -1; }
	void set_target(emulate65c02 *emulate, int t=-1) {
		if (t == -1) t = emulate->compile_point;
		target = t; 
		for (LabelFixup & fixup : *fixups) fixup.update_target(emulate, target);
	}
	void add_fixup(int instruction_field_address, bool relative) {
		fixups->push_back(LabelFixup(instruction_field_address, relative));
	}
};

extern int RMB_BY_BIT[8];
extern int BBR_BY_BIT[8];

struct emulate65c02 {
	int a, x, y, p, s;//empty stack decending
	int pc;
	uint8_t memory[65536];
	int compile_point;
	int data_point;
	long long time;
	bool waiting;
	bool stop;
	WRITE_MODES last_mode;
	int last_address;

	//use last_mode & last_address
	//note this model assumes that dma registers can't be read, or that reading them is only a trigger
	//These seem like good assumptions for the cX16
	void do_dma_triggers()
	{

	}

	long long execute(int addr)
	{
		long long t = time;
		stop = waiting = false;
		pc = addr;
		for (;;) {
			last_mode = NUM_WRITE_MODES;
			instructions[*map_addr(pc)](this);
			if (last_mode != NUM_WRITE_MODES) do_dma_triggers();
			++pc;
			if (waiting || stop) break;
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
		
		if (am!=IMM && am!=ZP) last_mode = wm;

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

		int r =a + c + u;
		sa += c + s;

		if (0 != (p&FLAG_D)) {
			++time;
			if ((u&0xf)+(a&0xf)+c>=0xa) {
				r += 0x6;
			}
			if (r >= 0xa0) {
				r += 0x60;
			}
		}

		if (0 != (r & 0x100)) p |= (int)FLAG_C;
		else p &= ~(int)FLAG_C;

		if ((0 != (sa & 0x80000000)) != (0 != (sa & 0x00000080)))p |= (int)FLAG_V;
		else p &= ~(int)FLAG_V;
		a = r & 0xff;
		test_for_N(test_for_Z(a));
	}

	void carry_from_shift_bit(int v)
	{
		if (0 != v) p |= (int)FLAG_C;
		else p &= ~(int)FLAG_C;
	}

	void do_sbc(int u)
	{
		if (0 != (p&FLAG_D)) return do_adc(0x99 - u); return do_adc(u ^ 0xff);
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
		last_address = addr;
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
		waiting(false), stop(false), last_mode(NUM_WRITE_MODES),last_address(-1)
	{
	}
	void comp_byte(uint8_t v) { memory[compile_point++] = v; }
	void comp_word(uint8_t v) { comp_byte(v & 0x0ff); comp_byte(v>>8); }

	void brk() { comp_byte(0); }
	void ora_ix(int v) { comp_byte(1); comp_byte(v); }
	//won't be an entry for every nop
	void nop_imm(int v) { comp_byte(2); comp_byte(v); }
	void nop() { comp_byte(3); }
	void tsb_zp(int v) { comp_byte(4); comp_byte(v); }
	void ora_zp(int v) { comp_byte(5); comp_byte(v); }
	void asl_zp(int v) { comp_byte(6); comp_byte(v); }
	void rmb(int bit, int zp) 
	{
		comp_byte(RMB_BY_BIT[bit]);
		comp_byte(zp);
	}
	void php() { comp_byte(8); }
	void ora_imm(int v) { comp_byte(9); comp_byte(v); }
	void asl_a() { comp_byte(0x0a); }
	void tsb_zp(int v) { comp_byte(0x0c); comp_byte(v); }
	void ora_abs(int v) { comp_byte(0x0d); comp_word(v); }
	void asl_abs(int v) { comp_byte(0x0e); comp_word(v); }
	void bbr(int bit, int zp)
	{
		comp_byte(BBR_BY_BIT[bit]);
		comp_byte(zp);
	}

	//if branch address is known then it either branches short (if it can reach) otherwise
	//if it can't reach it synthesizes a long branch
	//if the target isn't known it reserves space for a short if force_short is true
	//otherwise it reserves space for a long branch and adds a fixup
	void compile_branch(int branch, int inverted_branch, Label& label, bool force_short) {
		int cur_add = (compile_point + 2) & 0xffff;
		if (label.has_target() && label.target - cur_add <= 128 && cur_add - label.target <= 127) {
			comp_byte(branch);
			comp_byte(cur_add - label.target);
		}
		else if (force_short) {
			label.add_fixup((cur_add - 1) & 0xffff, true);
			comp_byte(branch);
			comp_byte(0);
		}
		else {
			label.add_fixup((cur_add + 1) & 0xffff, false);
			comp_byte(inverted_branch); //the code is designed so that if you try to run it when the fixup hasn't happened, the branch falls through
			comp_byte(3);//over jump
			comp_byte(0x4c);//jmp
			cur_add = (compile_point + 2) & 0xffff;
			comp_byte(cur_add & 0xff);
			comp_byte((cur_add >> 8) & 0xff);
		}
	}

	void bpl(Label& label, bool force_short=false) {
		compile_branch(0x10, 0x30, label, force_short);
	}
	void ora_izy(int v) { comp_byte(0x11); comp_byte(v); }
	void ora_izp(int v) { comp_byte(0x12); comp_byte(v); }
	void trb_zp(int v) { comp_byte(0x14); comp_byte(v); }
	void ora_zpx(int v){ comp_byte(0x15); comp_byte(v); }
	void asl_zpx(int v) { comp_byte(0x16); comp_byte(v); }
	void clc() { comp_byte(0x18); }
	void ora_aby(int v) { comp_byte(0x19); comp_word(v); }
	void inc() { comp_byte(0x1a); }
	void trb_abs(int v) { comp_byte(0x1c); comp_word(v); }
	void ora_abx(int v) { comp_byte(0x1d); comp_word(v); }
	void asl_abx(int v) { comp_byte(0x1e); comp_word(v); }
	void jsr(Label& label) {
		comp_byte(0x20);
		if (label.has_target()) {
			comp_word(label.target);
		}
		else {
			label.add_fixup(compile_point, false);
			comp_word((compile_point+2)&0xffff);
		}
	}
	void and_izx(int v) { comp_byte(0x21); comp_byte(v); }
	void bit_zp(int v) { comp_byte(0x24); comp_byte(v); }
	void and_zp(int v) { comp_byte(0x25); comp_byte(v); }
	void rol_zp(int v) { comp_byte(0x26); comp_byte(v); }
	void plp() { comp_byte(0x28); }
	void and_imm(int v) { comp_byte(0x29); comp_byte(v); }
	void rol() { comp_byte(0x2a); }
	void bit_abs(int v) { comp_byte(0x2c); comp_word(v); }
	void and_abs(int v) { comp_byte(0x2d); comp_word(v); }
	void rol_abs(int v) { comp_byte(0x2e); comp_word(v); }
	void bmi(Label& label, bool force_short = false) {
		compile_branch(0x30, 0x10, label, force_short);
	}
	void and_izy(int v) { comp_byte(0x31); comp_byte(v); }
	void and_izp(int v) { comp_byte(0x32); comp_byte(v); }
	void bit_zpx(int v) { comp_byte(0x34); comp_byte(v); }
	void and_zpx(int v){ comp_byte(0x35); comp_byte(v); }
	void rol_zpx(int v){ comp_byte(0x36); comp_byte(v); }
	void sec(){ comp_byte(0x38); }
	void and_aby(int v){ comp_byte(0x39); comp_word(v); }
	void dec(){ comp_byte(0x3a); }
	void bit_abx(int v){ comp_byte(0x3c); comp_word(v); }
	void and_abx(int v){ comp_byte(0x3d); comp_word(v); }
	void rol_abx(int v){ comp_byte(0x3e); comp_word(v); }
	void rti(int v){ comp_byte(0x40); }
	void eor_izx(int v){ comp_byte(0x41); comp_byte(v); }
	void eor_zp(int v){ comp_byte(0x45); comp_byte(v); }
	void lsr_zp(int v){ comp_byte(0x46); comp_byte(v); }
	void pha(int v){ comp_byte(0x48); }
	void eor_imm(int v){ comp_byte(0x49); comp_byte(v); }
	void lsr(int v){ comp_byte(0x4a); }
	void jmp_abs(int v){ comp_byte(0x4c); comp_word(v); }
	void jmp(Label &label) { 
		comp_byte(0x4c); 
		if (label.has_target()) {
			comp_word(label.target);
		}
		else {
			label.add_fixup(compile_point, false);
			comp_word(0);
		}

	}
	void eor_abs(int v){ comp_byte(0x4d); comp_word(v); }
	void lsr_abs(int v){ comp_byte(0x4e); comp_word(v); }
	void bvc(Label &label, bool force_short=false){ compile_branch(0x50, 0x70, label, force_short); }
	void eor_izy(int v){ comp_byte(0x51); comp_byte(v); }
	void eor_izp(int v){ comp_byte(0x52); comp_byte(v); }
	void eor_zpx(int v){ comp_byte(0x55); comp_byte(v); }
	void lsr_zpx(int v){ comp_byte(0x56); comp_byte(v); }
	void cli(int v){ comp_byte(0x58); }
	void eor_aby(int v){ comp_byte(0x59); comp_word(v); }
	void phy(int v){ comp_byte(0x5a); }
	void eor_abx(int v){ comp_byte(0x5d); comp_word(v); }
	void lsr_abx(int v){ comp_byte(0x5e); comp_word(v); }
	void rts(int v){ comp_byte(0x60); }
	void adc_izx(int v){ comp_byte(0x61); comp_byte(v); }
	void stz_zp(int v){ comp_byte(0x64); comp_byte(v); }
	void adc_zp(int v){ comp_byte(0x65); comp_byte(v); }
	void ror_zp(int v){ comp_byte(0x66); comp_byte(v); }
	void pla(int v){ comp_byte(0x68); }
	void adc_imm(int v){ comp_byte(0x69); comp_byte(v); }
	void ror(int v){ comp_byte(0x6a); }
	void jmp_ind(int v){ comp_byte(0x6c); comp_word(v); }
	void adc_abs(int v){ comp_byte(0x6d); comp_word(v); }
	void ror_abs(int v){ comp_byte(0x6e); comp_word(v); }
	void bvs(Label &label, bool force_short = false) { compile_branch(0x70, 0x50, label, force_short); }
	void adc_izy(int v){ comp_byte(0x71); comp_byte(v); }
	void adc_izp(int v){ comp_byte(0x72); comp_byte(v); }
	void stz_zpx(int v){ comp_byte(0x74); comp_byte(v); }
	void adc_zpx(int v){ comp_byte(0x75); comp_byte(v); }
	void ror_zpx(int v){ comp_byte(0x76); comp_byte(v); }
	void sei(int v){ comp_byte(0x78); }
	void adc_aby(int v){ comp_byte(0x79); comp_word(v); }
	void ply(int v){ comp_byte(0x7a); }
	void jmp_iax(int v){ comp_byte(0x7c); comp_word(v); }
	void adc_abx(int v){ comp_byte(0x7d); comp_word(v); }
	void ror_abx(int v){ comp_byte(0x7e); comp_word(v); }
	void bra(Label& label, bool force_short) {
		int cur_add = (compile_point + 2) & 0xffff;
		if (label.has_target() && label.target - cur_add <= 128 && cur_add - label.target <= 127) {
			comp_byte(0x80);
			comp_byte(cur_add - label.target);
		}
		else if (force_short) {
			label.add_fixup((cur_add - 1) & 0xffff, true);
			comp_byte(0x80);
			comp_byte(0);
		}
		else {
			comp_byte(0x4c);//jmp
			label.add_fixup(compile_point, false);
			comp_word((compile_point + 2) & 0xffff);
		}
	}
	void sta_izx(int v){ comp_byte(0x81); comp_byte(v); }
	void sty_zp(int v){ comp_byte(0x84); comp_byte(v); }
	void sta_zp(int v){ comp_byte(0x85); comp_byte(v); }
	void stx_zp(int v){ comp_byte(0x86); comp_byte(v); }
	void dey(int v){ comp_byte(0x88); }
	void bit_imm(int v){ comp_byte(0x89); comp_byte(v); }
	void txa(int v){ comp_byte(0x8a); }
	void sty_abs(int v){ comp_byte(0x8c); comp_word(v); }
	void sta_abs(int v){ comp_byte(0x8d); comp_word(v); }
	void stx_abs(int v){ comp_byte(0x8e); comp_word(v); }
	void bcc(Label &label, bool force_short = false) { compile_branch(0x90, 0xb0, label, force_short); }
	void sta_izy(int v){ comp_byte(0x91); comp_byte(v); }
	void sta_izp(int v){ comp_byte(0x92); comp_byte(v); }
	void sty_zpx(int v){ comp_byte(0x94); comp_byte(v); }
	void sta_zpx(int v){ comp_byte(0x95); comp_byte(v); }
	void stx_zpy(int v){ comp_byte(0x96); comp_byte(v); }
	void tya(int v){ comp_byte(0x98); }
	void sta_aby(int v){ comp_byte(0x99); comp_word(v); }
	void txs(int v){ comp_byte(0x9a); }
	void stz_abs(int v){ comp_byte(0x9c); comp_word(v); }
	void sta_abx(int v){ comp_byte(0x9d); comp_word(v); }
	void stz_abx(int v){ comp_byte(0x9e); comp_word(v); }
	void ldy_imm(int v){ comp_byte(0xa0); comp_byte(v); }
	void lda_izx(int v){ comp_byte(0xa1); comp_byte(v); }
	void ldx_imm(int v){ comp_byte(0xa2); comp_byte(v); }
	void ldy_zp(int v){ comp_byte(0xa4); comp_byte(v); }
	void lda_zp(int v){ comp_byte(0xa5); comp_byte(v); }
	void ldx_zp(int v){ comp_byte(0xa6); comp_byte(v); }
	void tay(int v){ comp_byte(0xa8); }
	void lda_imm(int v){ comp_byte(0xa9); comp_byte(v); }
	void tax(int v){ comp_byte(0xaa); }
	void ldy_abs(int v){ comp_byte(0xac); comp_word(v); }
	void lda_abs(int v){ comp_byte(0xad); comp_word(v); }
	void ldx_abs(int v){ comp_byte(0xae); comp_word(v); }
	void bcs_relb0(Label &label, bool force_short = false) { compile_branch(0xb0, 0x90, label, force_short); }
	void lda_izy(int v){ comp_byte(0xb1); comp_byte(v); }
	void lda_izp(int v){ comp_byte(0xb2); comp_byte(v); }
	void ldy_zpx(int v){ comp_byte(0xb4); comp_byte(v); }
	void lda_zpx(int v){ comp_byte(0xb5); comp_byte(v); }
	void ldx_zpy(int v){ comp_byte(0xb6); comp_byte(v); }
	void clv(int v){ comp_byte(0xb8); }
	void lda_aby(int v){ comp_byte(0xb9); comp_word(v); }
	void tsx(int v){ comp_byte(0xba); }
	void ldy_abx(int v){ comp_byte(0xbc); comp_word(v); }
	void lda_abx(int v){ comp_byte(0xbd); comp_word(v); }
	void ldx_aby(int v){ comp_byte(0xbe); comp_word(v); }
	void cpy_imm(int v){ comp_byte(0xc0); comp_byte(v); }
	void cmp_izx(int v){ comp_byte(0xc1); comp_byte(v); }
	void cpy_zp(int v){ comp_byte(0xc4); comp_byte(v); }
	void cmp_zp(int v){ comp_byte(0xc5); comp_byte(v); }
	void dec_zp(int v){ comp_byte(0xc6); comp_byte(v); }
	void iny(int v){ comp_byte(0xc8); }
	void cmp_imm(int v){ comp_byte(0xc9); comp_byte(v); }
	void dex(int v){ comp_byte(0xca); }
	void wai(int v){ comp_byte(0xcb); }
	void cpy_abs(int v){ comp_byte(0xcc); comp_word(v); }
	void cmp_abs(int v){ comp_byte(0xcd); comp_word(v); }
	void dec_abs(int v){ comp_byte(0xce); comp_word(v); }
	void bne(Label &label, bool force_short = false) { compile_branch(0xd0, 0xf0, label, force_short); }
	void cmp_izy(int v){ comp_byte(0xd1); comp_byte(v); }
	void cmp_izp(int v){ comp_byte(0xd2); comp_byte(v); }
	void cmp_zpx(int v){ comp_byte(0xd5); comp_byte(v); }
	void dec_zpx(int v){ comp_byte(0xd6); comp_byte(v); }
	void cld(int v){ comp_byte(0xd8); }
	void cmp_aby(int v){ comp_byte(0xd9); comp_word(v); }
	void phx(int v){ comp_byte(0xda); }
	void stp(int v){ comp_byte(0xdb); }
	void cmp_abx(int v){ comp_byte(0xdd); comp_word(v); }
	void dec_abx(int v){ comp_byte(0xde); comp_word(v); }
	void cpx_imm(int v){ comp_byte(0xe0); comp_byte(v); }
	void sbc_izx(int v){ comp_byte(0xe1); comp_byte(v); }
	void cpx_zp(int v){ comp_byte(0xe4); comp_byte(v); }
	void sbc_zp(int v){ comp_byte(0xe5); comp_byte(v); }
	void inc_zp(int v){ comp_byte(0xe6); comp_byte(v); }
	void inx(int v){ comp_byte(0xe8); }
	void sbc_imm(int v){ comp_byte(0xe9); comp_byte(v); }
	void cpx_abs(int v){ comp_byte(0xec); comp_word(v); }
	void sbc_abs(int v){ comp_byte(0xed); comp_word(v); }
	void inc_abs(int v){ comp_byte(0xee); comp_word(v); }
	void beq(Label &label, bool force_short = false) { compile_branch(0xf0, 0xd0, label, force_short); }
	void sbc_izy(int v){ comp_byte(0xf1); comp_byte(v); }
	void sbc_izp(int v){ comp_byte(0xf2); comp_byte(v); }
	void sbc_zpx(int v){ comp_byte(0xf5); comp_byte(v); }
	void inc_zpx(int v){ comp_byte(0xf6); comp_byte(v); }
	void sed(int v){ comp_byte(0xf8); }
	void sbc_aby(int v){ comp_byte(0xf9); comp_word(v); }
	void plx(int v){ comp_byte(0xfa); }
	void sbc_abx(int v){ comp_byte(0xfd); comp_word(v); }
	void inc_abx(int v){ comp_byte(0xfe); comp_word(v); }

	bool test_assembler();

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

