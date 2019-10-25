// 65c02incpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

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
int RMB_BY_BIT[8] = { 0x07,0x17,0x27,0x37,0x47,0x57,0x67,0x77 };
int BBR_BY_BIT[8] = { 0x0F,0x1F,0x2F,0x3F,0x4F,0x5F,0x6F,0x7F };


void LabelFixup::update_target(emulate65c02 *emulate, int t) {
	target = t;
	if (relative) {
		t = t - instruction_field_address - 1;
		if (t > 127 || t < -128) throw std::range_error("branch out of range");;
		*emulate->map_addr(instruction_field_address) = t;
	}
	else {
		uint8_t* s = emulate->map_addr(instruction_field_address);
		*s++ = instruction_field_address & 0xff;
		*s = (instruction_field_address >> 8) & 0xff;
	}
}


void BRK00(emulate65c02 *self)
{
	self->time += 7;
	self->push_word(self->pc);
	self->push_byte(self->p);
	self->p |= (int)FLAG_B;
	self->pc = self->deref_abs(0xfffe);
}
/*
enum WRITE_MODES { READ_MODE,WRITE_MODE,MODIFY_MODE, NUM_WRITE_MODES };
enum ADDRESSING_MODES { IMM, IZX, IZY, ZP, ABS, REL, IZP, ABY, ABX, NUM_ADDRESSING_MODES};
uint8_t *decode_addr(WRITE_MODES wm,ADDRESSING_MODES am, int *if_taken=nullptr)
*/


void ORA_izx01(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, IZX);
	self->test_for_N(self->test_for_Z(self->a));
}
void NOP_imm02(emulate65c02 *self)
{
	self->pc += 2;
	self->time += 2;
}
void NOP03(emulate65c02 *self)
{
	self->pc += 1;
	self->time += 1;
}
void TSB_zp04(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZP);

	int v = *add;
	self->test_for_Z(self->a & v);
	*add=(v | self->a);
}
void ORA_zp05(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, ZP);
	self->test_for_N(self->test_for_Z(self->a));
}
void ASL_zp06(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZP);
	int v = *add;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}
void RMB0_zp07(emulate65c02 *self) 
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x01;
}
void PHP08(emulate65c02 *self)
{
	self->push_byte(self->p);
	self->time += 3;
}

void ORA_imm09(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, IMM);
	self->test_for_N(self->test_for_Z(self->a));
}

void ASL0A(emulate65c02 *self)
{
	self->time += 2;
	int v = self->a;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	self->a = v;
}

void NOP0B(emulate65c02 *self)
{
}

void TSB_abs0C(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABS);

	int v = *add;
	self->test_for_Z(self->a & v);
	*add = (v | self->a);
}

void ORA_abs0D(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, ABS);
	self->test_for_N(self->test_for_Z(self->a));
}

void ASL_abs0E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABS);
	int v = *add;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR0_zpr0F(emulate65c02 *self)
{
	int v= *self->decode_addr(READ_MODE, ZP)&0x01;
	int taken_time, new_pc;
	new_pc=self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void BPL_rel10(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == ((int)FLAG_N&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void ORA_izy11(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, IZY);
	self->test_for_N(self->test_for_Z(self->a));
}

void ORA_izp12(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, IZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void NOP13(emulate65c02 *self)
{
}

void TRB_zp14(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZP);

	int v = *add;
	self->test_for_Z(self->a & v);
	*add = (v & ~(self->a));
}

void ORA_zpx15(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, ZPX);
	self->test_for_N(self->test_for_Z(self->a));
}

void ASL_zpx16(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZPX);
	int v = *add;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB1_zp17(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x02;
}

void CLC18(emulate65c02 *self)
{
	self->time += 2;
	self->p &= ~(int)FLAG_C;
}

void ORA_aby19(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, ABY);
	self->test_for_N(self->test_for_Z(self->a));
}

void INC1A(emulate65c02 *self)
{
	self->a=(self->a+1)&0xff;
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 2;
}

void NOP1B(emulate65c02 *self)
{
}

void TRB_abs1C(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABS);

	int v = *add;
	self->test_for_Z(self->a & v);
	*add = (v & ~(self->a));
}

void ORA_abx1D(emulate65c02 *self)
{
	self->a |= *self->decode_addr(READ_MODE, ABX);
	self->test_for_N(self->test_for_Z(self->a));
}

void ASL_abx1E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABX);
	int v = *add;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR1_zpr1F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x02;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void JSR_abs20(emulate65c02 *self)
{
	self->push_word(0xffff & (self->pc + 2));
	self->time += 3;
	self->decode_jmp(JABS);
}

void AND_izx21(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, IZX);
	self->test_for_N(self->test_for_Z(self->a));
}

void NOP_imm22(emulate65c02 *self)
{
}

void NOP23(emulate65c02 *self)
{
}

void BIT_zp24(emulate65c02 *self)
{
	int m = *self->decode_addr(READ_MODE, ZP);
	int v = self->a & m;
	
	if (0!=(m&0x40)) self->p |= (int)FLAG_V;
	else self->p &= ~(int)FLAG_V;

	self->test_for_Z(v);
	self->test_for_N(m);
}

void AND_zp25(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, ZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void ROL_zp26(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZP);
	int v = *add;
	int c = self->p&(int)FLAG_C;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v + c) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB2_zp27(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x04;
}

void PLP28(emulate65c02 *self)
{
	self->p = self->pop_byte();
	self->time += 4;
}

void AND_imm29(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, IMM);
	self->test_for_N(self->test_for_Z(self->a));
}

void ROL2A(emulate65c02 *self)
{
	self->time += 2;
	int v = self->a;
	int c = self->p & (int)FLAG_C;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v + c) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	self->a = v;
}

void NOP2B(emulate65c02 *self)
{
}

void BIT_abs2C(emulate65c02 *self)
{
	int m = *self->decode_addr(READ_MODE, ABS);
	int v = self->a & m;

	if (0 != (m & 0x40)) self->p |= (int)FLAG_V;
	else self->p &= ~(int)FLAG_V;

	self->test_for_Z(v);
	self->test_for_N(m);
}

void AND_abs2D(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, ABS);
	self->test_for_N(self->test_for_Z(self->a));
}

void ROL_abs2E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABS);
	int v = *add;
	int c = self->p&(int)FLAG_C;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v + c) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR2_zpr2F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x04;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void BMI_rel30(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != ((int)FLAG_N&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void AND_izy31(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, IZY);
	self->test_for_N(self->test_for_Z(self->a));
}

void AND_izp32(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, IZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void NOP33(emulate65c02 *self)
{
}

void BIT_zpx34(emulate65c02 *self)
{
	int m = *self->decode_addr(READ_MODE, ZPX);
	int v = self->a & m;

	if (0 != (m & 0x40)) self->p |= (int)FLAG_V;
	else self->p &= ~(int)FLAG_V;

	self->test_for_Z(v);
	self->test_for_N(m);
}

void AND_zpx35(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, ZPX);
	self->test_for_N(self->test_for_Z(self->a));
}

void ROL_zpx36(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZPX);
	int v = *add;
	int c = self->p&(int)FLAG_C;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v + c) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB3_zp37(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x08;
}

void SEC38(emulate65c02 *self)
{
	self->time += 2;
	self->p |= (int)FLAG_C;
}

void AND_aby39(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, ABY);
	self->test_for_N(self->test_for_Z(self->a));
}

void DEC3A(emulate65c02 *self)
{
	self->a = (self->a-1) & 0xff;
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 2;
}

void NOP3B(emulate65c02 *self)
{
}

void BIT_abx3C(emulate65c02 *self)
{
	int m = *self->decode_addr(READ_MODE, ABX);
	int v = self->a & m;

	if (0 != (m & 0x40)) self->p |= (int)FLAG_V;
	else self->p &= ~(int)FLAG_V;

	self->test_for_Z(v);
	self->test_for_N(m);
}

void AND_abx3D(emulate65c02 *self)
{
	self->a &= *self->decode_addr(READ_MODE, ABX);
	self->test_for_N(self->test_for_Z(self->a));
}

void ROL_abx3E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABX);
	int v = *add;
	int c = self->p&(int)FLAG_C;
	self->carry_from_shift_bit(v & 0x80);
	v = (v + v + c) & 0xff;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR3_zpr3F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x08;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void RTI40(emulate65c02 *self)
{
	self->p = self->pop_byte;
	self->pc = self->pop_word();
	self->time += 6;
}

void EOR_izx41(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, IZX);
	self->test_for_N(self->test_for_Z(self->a));
}

void NOP_imm42(emulate65c02 *self)
{
}

void NOP43(emulate65c02 *self)
{
}

void NOP_zp44(emulate65c02 *self)
{
}

void EOR_zp45(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, ZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void LSR_zp46(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZP);
	int v = *add;
	self->carry_from_shift_bit(v & 0x01);
	v>>=1;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB4_zp47(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x10;
}

void PHA48(emulate65c02 *self)
{
	self->push_byte(self->a);
	self->time += 3;
}

void EOR_imm49(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, IMM);
	self->test_for_N(self->test_for_Z(self->a));
}

void LSR4A(emulate65c02 *self)
{
	self->time += 2;
	int v = self->a;
	self->carry_from_shift_bit(v & 0x01);
	v >>= 1;
	self->test_for_N(self->test_for_Z(v));
	self->a = v;
}

void NOP4B(emulate65c02 *self)
{
}

void JMP_abs4C(emulate65c02 *self)
{
	self->decode_jmp(JABS);
}

void EOR_abs4D(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, ABS);
	self->test_for_N(self->test_for_Z(self->a));
}

void LSR_abs4E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABS);
	int v = *add;
	self->carry_from_shift_bit(v & 0x01);
	v >>= 1;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR4_zpr4F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x10;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void BVC_rel50(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == ((int)FLAG_V&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void EOR_izy51(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, IZY);
	self->test_for_N(self->test_for_Z(self->a));
}

void EOR_izp52(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, IZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void NOP53(emulate65c02 *self)
{
}

void NOP_zpx54(emulate65c02 *self)
{
}

void EOR_zpx55(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, ZPX);
	self->test_for_N(self->test_for_Z(self->a));
}

void LSR_zpx56(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZPX);
	int v = *add;
	self->carry_from_shift_bit(v & 0x01);
	v >>= 1;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB5_zp57(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x20;
}

void CLI58(emulate65c02 *self)
{
	self->p &= ~(int)FLAG_I;
	self->time += 2;
}

void EOR_aby59(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, ABY);
	self->test_for_N(self->test_for_Z(self->a));
}

void PHY5A(emulate65c02 *self)
{
	self->push_byte(self->y);
	self->time += 6;
}

void NOP5B(emulate65c02 *self)
{
}

void NOP_abs5C(emulate65c02 *self)
{
}

void EOR_abx5D(emulate65c02 *self)
{
	self->a ^= *self->decode_addr(READ_MODE, ABX);
	self->test_for_N(self->test_for_Z(self->a));
}

void LSR_abx5E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABX);
	int v = *add;
	self->carry_from_shift_bit(v & 0x01);
	v >>= 1;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR5_zpr5F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x20;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void RTS60(emulate65c02 *self)
{
	self->pc = self->pop_word();
	self->time += 6;
}

void ADC_izx61(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, IZX));
}

void NOP_imm62(emulate65c02 *self)
{
}

void NOP63(emulate65c02 *self)
{
}

void STZ_zp64(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZP) = 0;
}

void ADC_zp65(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, ZP));
}

void ROR_zp66(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZP);
	int v = *add;
	int c = (self->p&(int)FLAG_C)<<8;
	self->carry_from_shift_bit(v & 0x01);
	v = (v>>1)|c;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB6_zp67(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x40;
}

void PLA68(emulate65c02 *self)
{
	self->a = self->pop_byte();
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 4;
}

void ADC_imm69(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, IMM));
}

void ROR6A(emulate65c02 *self)
{
	self->time += 2;
	int v = self->a;
	int c = (self->p&(int)FLAG_C) << 8;
	self->carry_from_shift_bit(v & 0x01);
	v = (v >> 1) | c;
	self->test_for_N(self->test_for_Z(v));
	self->a = v;
}

void NOP6B(emulate65c02 *self)
{
}

void JMP_ind6C(emulate65c02 *self)
{
	self->decode_jmp(JIND);
}

void ADC_abs6D(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, ABS));
}

void ROR_abs6E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABS);
	int v = *add;
	int c = (self->p&(int)FLAG_C) << 8;
	self->carry_from_shift_bit(v & 0x01);
	v = (v >> 1) | c;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR6_zpr6F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x40;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void BVS_rel70(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != ((int)FLAG_V&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void ADC_izy71(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, IZY));
}

void ADC_izp72(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, IZP));
}

void NOP73(emulate65c02 *self)
{
}

void STZ_zpx74(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZPX) = 0;
}

void ADC_zpx75(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, ZPX));
}

void ROR_zpx76(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ZPX);
	int v = *add;
	int c = (self->p&(int)FLAG_C) << 8;
	self->carry_from_shift_bit(v & 0x01);
	v = (v >> 1) | c;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void RMB7_zp77(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) &= ~0x80;
}

void SEI78(emulate65c02 *self)
{
	self->p |= (int)FLAG_I;
	self->time += 2;
}

void ADC_aby79(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, ABY));
}

void PLY7A(emulate65c02 *self)
{
	self->y = self->pop_byte();
	self->test_for_N(self->test_for_Z(self->y));
	self->time += 4;
}

void NOP7B(emulate65c02 *self)
{
}

void JMP_iax7C(emulate65c02 *self)
{
	self->decode_jmp(JIAX);
}

void ADC_abx7D(emulate65c02 *self)
{
	self->do_adc(*self->decode_addr(READ_MODE, ABX));
}

void ROR_abx7E(emulate65c02 *self)
{
	uint8_t *add = self->decode_addr(MODIFY_MODE, ABX);
	int v = *add;
	int c = (self->p&(int)FLAG_C) << 8;
	self->carry_from_shift_bit(v & 0x01);
	v = (v >> 1) | c;
	self->test_for_N(self->test_for_Z(v));
	*add = v;
}

void BBR7_zpr7F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x80;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == v) {
		self->pc = new_pc;
	}
}

void BRA_rel80(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	self->pc = new_pc;
	self->time += taken_time;
}

void STA_izx81(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, IZX) = self->a;
}

void NOP_imm82(emulate65c02 *self)
{
}

void NOP83(emulate65c02 *self)
{
}

void STY_zp84(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZP) = self->y;
}

void STA_zp85(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, IZP) = self->a;
}

void STX_zp86(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZP) = self->x;
}

void SMB0_zp87(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x01;
}

void DEY88(emulate65c02 *self)
{
	--self->y;
	self->test_for_N(self->test_for_Z(self->y));
	self->time += 2;
}

void BIT_imm89(emulate65c02 *self)
{
	int m = *self->decode_addr(READ_MODE, IMM);
	int v = self->a & m;

	if (0 != (m & 0x40)) self->p |= (int)FLAG_V;
	else self->p &= ~(int)FLAG_V;

	self->test_for_Z(v);
	self->test_for_N(m);
}

void TXA8A(emulate65c02 *self)
{
	self->a = self->x;
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 2;
}

void NOP8B(emulate65c02 *self)
{
}

void STY_abs8C(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABS) = self->y;
}

void STA_abs8D(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABX) = self->a;
}

void STX_abs8E(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABS) = self->x;
}

void BBS0_zpr8F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x01;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void BCC_rel90(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == ((int)FLAG_C&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void STA_izy91(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, IZY) = self->a;
}

void STA_izp92(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, IZP) = self->a;
}

void NOP93(emulate65c02 *self)
{
}

void STY_zpx94(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZPX) = self->y;
}

void STA_zpx95(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZPX) = self->a;
}

void STX_zpy96(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ZPY) = self->x;
}

void SMB1_zp97(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x02;
}

void TYA98(emulate65c02 *self)
{
	self->a = self->y;
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 2;
}

void STA_aby99(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABY) = self->a;
}

void TXS9A(emulate65c02 *self)
{
	self->s = self->x;
	self->time += 2;
}

void NOP9B(emulate65c02 *self)
{
}

void STZ_abs9C(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABS) = 0;
}

void STA_abx9D(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABX) = self->a;
}

void STZ_abx9E(emulate65c02 *self)
{
	*self->decode_addr(WRITE_MODE, ABX) = 0;
}

void BBS1_zpr9F(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x02;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void LDY_immA0(emulate65c02 *self)
{
	self->y = *self->decode_addr(READ_MODE, IMM);
	self->test_for_N(self->test_for_Z(self->y));
}

void LDA_izxA1(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, IZX);
	self->test_for_N(self->test_for_Z(self->a));
}

void LDX_immA2(emulate65c02 *self)
{
	self->x = *self->decode_addr(READ_MODE, IMM);
	self->test_for_N(self->test_for_Z(self->x));
}

void NOPA3(emulate65c02 *self)
{
}

void LDY_zpA4(emulate65c02 *self)
{
	self->y = *self->decode_addr(READ_MODE, ZP);
	self->test_for_N(self->test_for_Z(self->y));
}

void LDA_zpA5(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, ZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void LDX_zpA6(emulate65c02 *self)
{
	self->x = *self->decode_addr(READ_MODE, ZP);
	self->test_for_N(self->test_for_Z(self->x));
}

void SMB2_zpA7(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x04;
}

void TAYA8(emulate65c02 *self)
{
	self->y = self->a;
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 2;
}

void LDA_immA9(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, IMM);
	self->test_for_N(self->test_for_Z(self->a));
}

void TAXAA(emulate65c02 *self)
{
	self->x = self->a;
	self->test_for_N(self->test_for_Z(self->a));
	self->time += 2;
}

void NOPAB(emulate65c02 *self)
{
}

void LDY_absAC(emulate65c02 *self)
{
	self->y = *self->decode_addr(READ_MODE, ABS);
	self->test_for_N(self->test_for_Z(self->y));
}

void LDA_absAD(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, ABS);
	self->test_for_N(self->test_for_Z(self->a));
}

void LDX_absAE(emulate65c02 *self)
{
	self->x = *self->decode_addr(READ_MODE, ABS);
	self->test_for_N(self->test_for_Z(self->x));
}

void BBS2_zprAF(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x04;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void BCS_relB0(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != ((int)FLAG_C&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void LDA_izyB1(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, IZY);
	self->test_for_N(self->test_for_Z(self->a));
}

void LDA_izpB2(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, IZP);
	self->test_for_N(self->test_for_Z(self->a));
}

void NOPB3(emulate65c02 *self)
{
}

void LDY_zpxB4(emulate65c02 *self)
{
	self->y = *self->decode_addr(READ_MODE, ZPX);
	self->test_for_N(self->test_for_Z(self->y));
}

void LDA_zpxB5(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, ZPX);
	self->test_for_N(self->test_for_Z(self->a));
}

void LDX_zpyB6(emulate65c02 *self)
{
	self->x = *self->decode_addr(READ_MODE, ZPY);
	self->test_for_N(self->test_for_Z(self->x));
}

void SMB3_zpB7(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x08;
}

void CLVB8(emulate65c02 *self)
{
	self->p &= ~(int)FLAG_V;
	self->time += 2;
}

void LDA_abyB9(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, ABY);
	self->test_for_N(self->test_for_Z(self->a));
}

void TSXBA(emulate65c02 *self)
{
	self->x = self->s;
	self->test_for_N(self->test_for_Z(self->x));
	self->time += 2;
}

void NOPBB(emulate65c02 *self)
{
}

void LDY_abxBC(emulate65c02 *self)
{
	self->y = *self->decode_addr(READ_MODE, ABX);
	self->test_for_N(self->test_for_Z(self->y));
}

void LDA_abxBD(emulate65c02 *self)
{
	self->a = *self->decode_addr(READ_MODE, ABX);
	self->test_for_N(self->test_for_Z(self->a));
}

void LDX_abyBE(emulate65c02 *self)
{
	self->x = *self->decode_addr(READ_MODE, ABY);
	self->test_for_N(self->test_for_Z(self->x));
}

void BBS3_zprBF(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x08;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void CPY_immC0(emulate65c02 *self)
{
	self->do_cmp(self->y, *self->decode_addr(READ_MODE, IMM));
}

void CMP_izxC1(emulate65c02 *self)
{
	self->do_cmp(self->a,*self->decode_addr(READ_MODE, IZX));
}

void NOP_immC2(emulate65c02 *self)
{
}

void NOPC3(emulate65c02 *self)
{
}

void CPY_zpC4(emulate65c02 *self)
{
	self->do_cmp(self->y, *self->decode_addr(READ_MODE, ZP));
}

void CMP_zpC5(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, ZP));
}

void DEC_zpC6(emulate65c02 *self)
{
	int v = --*self->decode_addr(MODIFY_MODE, ZP) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void SMB4_zpC7(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x10;
}

void INYC8(emulate65c02 *self)
{
	++self->y;
	self->test_for_N(self->test_for_Z(self->y));
	self->time += 2;
}

void CMP_immC9(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, IMM));
}

void DEXCA(emulate65c02 *self)
{
	--self->x;
	self->test_for_N(self->test_for_Z(self->x));
	self->time += 2;
}

void WAICB(emulate65c02 *self)
{
	self->waiting = false;
	self->time += 3;
}

void CPY_absCC(emulate65c02 *self)
{
	self->do_cmp(self->y, *self->decode_addr(READ_MODE, ABS));
}

void CMP_absCD(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, ABS));
}

void DEC_absCE(emulate65c02 *self)
{
	int v = --*self->decode_addr(MODIFY_MODE, ABS) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void BBS4_zprCF(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x10;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void BNE_relD0(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != ((int)FLAG_Z&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void CMP_izyD1(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, IZY));
}

void CMP_izpD2(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, IZP));
}

void NOPD3(emulate65c02 *self)
{
}

void NOP_zpxD4(emulate65c02 *self)
{
}

void CMP_zpxD5(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, ZPX));
}

void DEC_zpxD6(emulate65c02 *self)
{
	int v = --*self->decode_addr(MODIFY_MODE, ZPX) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void SMB5_zpD7(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x20;
}

void CLDD8(emulate65c02 *self)
{
	self->p &= ~(int)FLAG_D;
	self->time += 2;
}

void CMP_abyD9(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, ABY));
}

void PHXDA(emulate65c02 *self)
{
	self->push_byte(self->x);
	self->time += 3;
}

void STPDB(emulate65c02 *self)
{
	self->time += 3;
	self->stop = true;
}

void NOP_absDC(emulate65c02 *self)
{
}

void CMP_abxDD(emulate65c02 *self)
{
	self->do_cmp(self->a, *self->decode_addr(READ_MODE, ABX));
}

void DEC_abxDE(emulate65c02 *self)
{
	int v = --*self->decode_addr(MODIFY_MODE, ABX) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void BBS5_zprDF(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x20;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void CPX_immE0(emulate65c02 *self)
{
	self->do_cmp(self->x, *self->decode_addr(READ_MODE, IMM));
}

void SBC_izxE1(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, IZX));
}

void NOP_immE2(emulate65c02 *self)
{
}

void NOPE3(emulate65c02 *self)
{
}

void CPX_zpE4(emulate65c02 *self)
{
	self->do_cmp(self->x, *self->decode_addr(READ_MODE, ZP));
}

void SBC_zpE5(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, ZP));
}

void INC_zpE6(emulate65c02 *self)
{
	int v = ++*self->decode_addr(MODIFY_MODE, ZP) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void SMB6_zpE7(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x40;
}

void INXE8(emulate65c02 *self)
{
	++self->x;
	self->test_for_N(self->test_for_Z(self->x));
	self->time += 2;
}

void SBC_immE9(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, IMM));
}

void NOPEA(emulate65c02 *self)
{
}

void NOPEB(emulate65c02 *self)
{
}

void CPX_absEC(emulate65c02 *self)
{
	self->do_cmp(self->x, *self->decode_addr(READ_MODE, ABS));
}

void SBC_absED(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, ABS));
}

void INC_absEE(emulate65c02 *self)
{
	int v = ++*self->decode_addr(MODIFY_MODE, ABS) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void BBS6_zprEF(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x40;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}

void BEQ_relF0(emulate65c02 *self)
{
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 == ((int)FLAG_Z&self->p)) {
		self->pc = new_pc;
		self->time += taken_time;
	}
}

void SBC_izyF1(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, IZY));
}

void SBC_izpF2(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, IZP));
}

void NOPF3(emulate65c02 *self)
{
}

void NOP_zpxF4(emulate65c02 *self)
{
}

void SBC_zpxF5(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, ZPX));
}

void INC_zpxF6(emulate65c02 *self)
{
	int v = ++*self->decode_addr(MODIFY_MODE, ZPX) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void SMB7_zpF7(emulate65c02 *self)
{
	*self->decode_addr(MODIFY_MODE, ZP) |= 0x80;
}

void SEDF8(emulate65c02 *self)
{
	self->p |= (int)FLAG_D;
	self->time += 2;
}

void SBC_abyF9(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, ABY));
}

void PLXFA(emulate65c02 *self)
{
	self->x = self->pop_byte();
	self->test_for_N(self->test_for_Z(self->x));
	self->time += 4;
}

void NOPFB(emulate65c02 *self)
{
}

void NOP_absFC(emulate65c02 *self)
{
}

void SBC_abxFD(emulate65c02 *self)
{
	self->do_sbc(*self->decode_addr(READ_MODE, ABX));
}

void INC_abxFE(emulate65c02 *self)
{
	int v = ++*self->decode_addr(MODIFY_MODE, ABX) & 0xff;
	self->test_for_N(self->test_for_Z(v));
}

void BBS7_ZprFF(emulate65c02 *self)
{
	int v = *self->decode_addr(READ_MODE, ZP) & 0x80;
	int taken_time, new_pc;
	new_pc = self->decode_branch(&taken_time);
	if (0 != v) {
		self->pc = new_pc;
	}
}


INSTRUCTION * emulate65c02::instructions[256] = {
	BRK00,     ORA_izx01, NOP_imm02, NOP03, TSB_zp04,  ORA_zp05,  ASL_zp06,  RMB0_zp07, PHP08, ORA_imm09, ASL0A, NOP0B, TSB_abs0C, ORA_abs0D, ASL_abs0E, BBR0_zpr0F,
	BPL_rel10, ORA_izy11, ORA_izp12, NOP13, TRB_zp14,  ORA_zpx15, ASL_zpx16, RMB1_zp17, CLC18, ORA_aby19, INC1A, NOP1B, TRB_abs1C, ORA_abx1D, ASL_abx1E, BBR1_zpr1F,
	JSR_abs20, AND_izx21, NOP_imm22, NOP23, BIT_zp24,  AND_zp25,  ROL_zp26,  RMB2_zp27, PLP28, AND_imm29, ROL2A, NOP2B, BIT_abs2C, AND_abs2D, ROL_abs2E, BBR2_zpr2F,
	BMI_rel30, AND_izy31, AND_izp32, NOP33, BIT_zpx34, AND_zpx35, ROL_zpx36, RMB3_zp37, SEC38, AND_aby39, DEC3A, NOP3B, BIT_abx3C, AND_abx3D, ROL_abx3E, BBR3_zpr3F,
	RTI40,     EOR_izx41, NOP_imm42, NOP43, NOP_zp44,  EOR_zp45,  LSR_zp46,  RMB4_zp47, PHA48, EOR_imm49, LSR4A, NOP4B, JMP_abs4C, EOR_abs4D, LSR_abs4E, BBR4_zpr4F,
	BVC_rel50, EOR_izy51, EOR_izp52, NOP53, NOP_zpx54, EOR_zpx55, LSR_zpx56, RMB5_zp57, CLI58, EOR_aby59, PHY5A, NOP5B, NOP_abs5C, EOR_abx5D, LSR_abx5E, BBR5_zpr5F,
	RTS60,     ADC_izx61, NOP_imm62, NOP63, STZ_zp64,  ADC_zp65,  ROR_zp66,  RMB6_zp67, PLA68, ADC_imm69, ROR6A, NOP6B, JMP_ind6C, ADC_abs6D, ROR_abs6E, BBR6_zpr6F,
	BVS_rel70, ADC_izy71, ADC_izp72, NOP73, STZ_zpx74, ADC_zpx75, ROR_zpx76, RMB7_zp77, SEI78, ADC_aby79, PLY7A, NOP7B, JMP_iax7C, ADC_abx7D, ROR_abx7E, BBR7_zpr7F,
	BRA_rel80, STA_izx81, NOP_imm82, NOP83, STY_zp84,  STA_zp85,  STX_zp86,  SMB0_zp87, DEY88, BIT_imm89, TXA8A, NOP8B, STY_abs8C, STA_abs8D, STX_abs8E, BBS0_zpr8F,
	BCC_rel90, STA_izy91, STA_izp92, NOP93, STY_zpx94, STA_zpx95, STX_zpy96, SMB1_zp97, TYA98, STA_aby99, TXS9A, NOP9B, STZ_abs9C, STA_abx9D, STZ_abx9E, BBS1_zpr9F,
	LDY_immA0, LDA_izxA1, LDX_immA2, NOPA3, LDY_zpA4,  LDA_zpA5,  LDX_zpA6,  SMB2_zpA7, TAYA8, LDA_immA9, TAXAA, NOPAB, LDY_absAC, LDA_absAD, LDX_absAE, BBS2_zprAF,
	BCS_relB0, LDA_izyB1, LDA_izpB2, NOPB3, LDY_zpxB4, LDA_zpxB5, LDX_zpyB6, SMB3_zpB7, CLVB8, LDA_abyB9, TSXBA, NOPBB, LDY_abxBC, LDA_abxBD, LDX_abyBE, BBS3_zprBF,
	CPY_immC0, CMP_izxC1, NOP_immC2, NOPC3, CPY_zpC4,  CMP_zpC5,  DEC_zpC6,  SMB4_zpC7, INYC8, CMP_immC9, DEXCA, WAICB, CPY_absCC, CMP_absCD, DEC_absCE, BBS4_zprCF,
	BNE_relD0, CMP_izyD1, CMP_izpD2, NOPD3, NOP_zpxD4, CMP_zpxD5, DEC_zpxD6, SMB5_zpD7, CLDD8, CMP_abyD9, PHXDA, STPDB, NOP_absDC, CMP_abxDD, DEC_abxDE, BBS5_zprDF,
	CPX_immE0, SBC_izxE1, NOP_immE2, NOPE3, CPX_zpE4,  SBC_zpE5,  INC_zpE6,  SMB6_zpE7, INXE8, SBC_immE9, NOPEA, NOPEB, CPX_absEC, SBC_absED, INC_absEE, BBS6_zprEF,
	BEQ_relF0, SBC_izyF1, SBC_izpF2, NOPF3, NOP_zpxF4, SBC_zpxF5, INC_zpxF6, SMB7_zpF7, SEDF8, SBC_abyF9, PLXFA, NOPFB, NOP_absFC, SBC_abxFD, INC_abxFE, BBS7_ZprFF,

};

int main()
{
    std::cout << "Hello World!\n"; 
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
