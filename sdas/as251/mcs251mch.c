/* mcs251mch.c */

/*
 *  Copyright (C) 1998-2025  Alan R. Baldwin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Alan R. Baldwin
 * 721 Berkeley St.
 * Kent, Ohio  44240
 *
 *   This Assember Ported by
 *	John L. Hartman	(JLH)
 *	jhartman at compuserve dot com
 *	noice at noicedebugger dot com
 *
 *  Benny Kim (2011/07/21)
 *  bennykim at coreriver dot com
 *  Fixed bugs in relative address with "."
 */

#include "asxxxx.h"
#include "mcs251.h"

char	*cpu	= "Intel MCS-251 (Source mode)";
char	*dsft	= "asm";

/*
 * A high opcode byte marks a native MCS-251 encoding.  In Source mode,
 * native Area III opcodes are unprefixed while conflicting classic Area II
 * opcodes are escaped with A5.  Area I (low nibble 0..5) is common.
 */
int mcs251_source_mode = 1;

static int
needs_prefix(a_uint op)
{
	return ((op & 0x0F) >= 6) &&
	       (mcs251_source_mode != ((op & 0xFF00) != 0));
}

static void
putcode(a_uint op)
{
	if (needs_prefix(op))
		outab(0xA5);
	outab(op & 0xFF);
}

static int
register_width(int mode)
{
	switch (mode) {
	case S_A:
	case S_REG:
		return 0;
	case S_WREG:
		return 1;
	case S_DREG:
		return 2;
	default:
		return -1;
	}
}

static int
register_number(int mode, struct expr *esp)
{
	return mode == S_A ? 11 : esp->e_addr;
}

static int
native_size_code(int width)
{
	return width == 2 ? 3 : width;
}

static void
out_immediate(struct expr *esp, int width)
{
	if (width == 0)
		outrb(esp, R_NORM);
	else
		outrw(esp, R_NORM);
}

static int
immediate_fits(a_uint value, unsigned int bits)
{
	a_uint unsigned_max = (((a_uint) 1) << bits) - 1;
	a_uint signed_min = a_mask - ((((a_uint) 1) << (bits - 1)) - 1);
	a_uint address = value & a_mask;

	return address <= unsigned_max || address >= signed_min;
}

static void
out_direct(struct expr *esp, int mode)
{
	if (mode == S_DIR)
		outrb(esp, R_PAG0);
	else
		outrw(esp, R_NORM);
}

static int
indexed_register(struct expr *esp)
{
	return (esp->e_addr >> 24) & 0x0F;
}

static void
out_displacement(struct expr *esp)
{
	struct expr displacement = *esp;

	if (is_abs(esp) && !immediate_fits(esp->e_addr, 16))
		xerr('a', "Indexed MOV displacement must fit in 16 bits.");
	displacement.e_addr &= 0xFFFF;
	outrw(&displacement, R_NORM);
}

#define BIT_CLASSIC 1
#define BIT_EXTENDED 2

struct mcs251_bit {
	int mode;
	int number;
	struct expr address;
};

static int
parse_bit(struct mcs251_bit *bit, int *inverted)
{
	char *start;
	char *dotp;
	char *p;
	char saved;
	int c;
	int base_mode;
	struct expr bit_number;

	memset(bit, 0, sizeof(*bit));
	clrexpr(&bit->address);
	clrexpr(&bit_number);
	*inverted = 0;
	c = getnb();
	if (c == '/')
		*inverted = 1;
	else
		unget(c);

	start = ip;
	dotp = NULL;
	for (p = start; *p && *p != ',' && *p != ';' &&
	     *p != ' ' && *p != '\t'; p++) {
		if (*p == '.')
			dotp = p;
	}
	if (!dotp) {
		expr(&bit->address, 0);
		bit->mode = BIT_CLASSIC;
		return bit->mode;
	}

	saved = *dotp;
	*dotp = '\0';
	ip = start;
	base_mode = addr(&bit->address);
	*dotp = saved;
	ip = dotp + 1;
	expr(&bit_number, 0);
	if (!is_abs(&bit_number) || bit_number.e_addr > 7) {
		xerr('a', "Bit number must be a constant from 0 through 7.");
		return 0;
	}
	if (base_mode != S_DIR) {
		xerr('a', "A MCS251 bit base must be direct data or an S: address.");
		return 0;
	}
	bit->number = bit_number.e_addr;
	if (is_abs(&bit->address) && bit->address.e_addr >= 0x20 &&
	    bit->address.e_addr <= 0x2F) {
		bit->address.e_addr = ((bit->address.e_addr - 0x20) << 3) |
		                      bit->number;
		bit->mode = BIT_CLASSIC;
	} else if (is_abs(&bit->address) && bit->address.e_addr >= 0x80 &&
	           !(bit->address.e_addr & 7)) {
		bit->address.e_addr += bit->number;
		bit->mode = BIT_CLASSIC;
	} else {
		bit->mode = BIT_EXTENDED;
	}
	return bit->mode;
}

static int
bit_syntax_before_comma(void)
{
	char *p;

	for (p = ip; *p && *p != ',' && *p != ';'; p++) {
		if (*p == '.')
			return 1;
	}
	return 0;
}

static void
out_extended_bit(int opcode, struct mcs251_bit *bit)
{
	putcode(0x1A9);
	outab(opcode + bit->number);
	outrb(&bit->address, R_PAG0);
}

static void
out_relative(struct expr *target)
{
	int distance;

	if (mchpcr(target)) {
		distance = (int) (target->e_addr - dot.s_addr - 1);
		if (pass == 2 && (distance < -128 || distance > 127))
			xerr('a', "Branching range exceeded.");
		outab(distance);
	} else {
		outrb(target, R_PCR);
	}
	if (target->e_mode != S_USER)
		rerr();
}

static void
out_control16(struct expr *target)
{
	a_uint next = dot.s_addr + 2;

	if (pass == 2 &&
	    (is_abs(target) || target->e_base.e_ap == dot.s_area) &&
	    ((target->e_addr ^ next) & ~((a_uint) 0xFFFF)))
		xerr('a', "LCALL/LJMP target is outside the current 64K region.");
	outrw(target, R_J16);
}

static void
out_control11(struct expr *target, a_uint opcode)
{
	a_uint next = dot.s_addr + 2;

	if (pass == 2 &&
	    (is_abs(target) || target->e_base.e_ap == dot.s_area) &&
	    ((target->e_addr ^ next) & ~((a_uint) 0x07FF)))
		xerr('a', "ACALL/AJMP target is outside the current 2K page.");
	outrwm(target, R_J11 | R_MCS251_CONTROL, opcode);
}

#define RELAX_WORDS 128
static unsigned int relax_bits[RELAX_WORDS];
static unsigned int *relax_pointer = relax_bits;
static unsigned int relax_mask = 1;

static int
relax_setbit(int value)
{
	if (relax_pointer >= &relax_bits[RELAX_WORDS])
		return 1;
	if (value)
		*relax_pointer |= relax_mask;
	relax_mask <<= 1;
	if (!relax_mask) {
		relax_mask = 1;
		relax_pointer++;
	}
	return value;
}

static int
relax_getbit(void)
{
	int value;

	if (relax_pointer >= &relax_bits[RELAX_WORDS])
		return 1;
	value = !!(*relax_pointer & relax_mask);
	relax_mask <<= 1;
	if (!relax_mask) {
		relax_mask = 1;
		relax_pointer++;
	}
	return value;
}

/* Forms: 0 relative, 1 absolute-page, 2 long, 3 extended. */
static int
select_control_form(struct expr *target, int allow_relative)
{
	int form;
	a_uint value;
	a_uint here = dot.s_addr;

	if (pass == 0)
		return 3;
	if (pass == 2) {
		form = relax_getbit() ? 1 : 0;
		form |= relax_getbit() ? 2 : 0;
		return form;
	}

	value = target->e_addr;
	if (target->e_base.e_ap == dot.s_area && value >= here)
		value -= fuzz;
	if (allow_relative && target->e_base.e_ap == dot.s_area &&
	    (int) value - (int) (here + 2) >= -128 &&
	    (int) value - (int) (here + 2) <= 127) {
		form = 0;
	} else if ((target->e_base.e_ap == dot.s_area || is_abs(target)) &&
	           (value >> 11) == ((here + 2) >> 11)) {
		form = 1;
	} else if ((target->e_base.e_ap == dot.s_area || is_abs(target)) &&
	           (value >> 16) == ((here + 3) >> 16)) {
		form = 2;
	} else {
		form = 3;
	}
	relax_setbit(form & 1);
	relax_setbit(form & 2);
	return form;
}

/*
 * Opcode Cycle Definitions
 */
#define	OPCY_SDP	((char) (0xFF))
#define	OPCY_ERR	((char) (0xFE))

/*      OPCY_NONE       ((char) (0x80)) */
/*      OPCY_MASK       ((char) (0x7F)) */

#define	UN	((char) (OPCY_NONE | 0x00))

/*
 * 8051 Cycle Count
 *
 *	opcycles = i51pg1[opcode]
 */
static char i51pg1[256] = {
/*--*--* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/*--*--* -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - */
/*00*/  12,24,24,12,12,12,12,12,12,12,12,12,12,12,12,12,
/*10*/  24,24,24,12,12,12,12,12,12,12,12,12,12,12,12,12,
/*20*/  24,24,24,12,12,12,12,12,12,12,12,12,12,12,12,12,
/*30*/  24,24,24,12,12,12,12,12,12,12,12,12,12,12,12,12,
/*40*/  24,24,12,24,12,12,12,12,12,12,12,12,12,12,12,12,
/*50*/  24,24,12,24,12,12,12,12,12,12,12,12,12,12,12,12,
/*60*/  24,24,12,24,12,12,12,12,12,12,12,12,12,12,12,12,
/*70*/  24,24,24,24,12,24,12,12,12,12,12,12,12,12,12,12,
/*80*/  24,24,24,24,48,24,24,24,24,24,24,24,24,24,24,24,
/*90*/  24,24,24,24,12,12,12,12,12,12,12,12,12,12,12,12,
/*A0*/  24,24,12,24,48,UN,24,24,24,24,24,24,24,24,24,24,
/*B0*/  24,24,12,12,24,24,24,24,24,24,24,24,24,24,24,24,
/*C0*/  24,24,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
/*D0*/  24,24,12,12,12,24,12,12,24,24,24,24,24,24,24,24,
/*E0*/  24,24,24,24,12,12,12,12,12,12,12,12,12,12,12,12,
/*F0*/  24,24,24,24,12,12,12,12,12,12,12,12,12,12,12,12
};

/*
 * Process machine ops.
 */
void
machine(struct mne *mp)
{
        a_uint op;
        int t, t1, v1;
        struct expr e, e1, e2;

	clrexpr(&e);
	clrexpr(&e1);
        clrexpr(&e2);

	op = mp->m_valu;
	switch (mp->m_type) {
	case S_MODE:
		mcs251_source_mode = (op != 0);
		opcycles = OPCY_SDP;
		lmode = SLIST;
		break;

	case S_3BYTE:
		do {
			clrexpr(&e);
			expr(&e, 0);
			outr3b(&e, R_NORM);
			t = getnb();
		} while (t == ',');
		unget(t);
		break;

	case S_INH:
		putcode(op);
		break;

	case S_JMP11:
		/*
		 * 11 bit destination.
		 * Top 3 bits become the MSBs of the op-code.
		 */
		expr(&e, 0);
		out_control11(&e, op);
		break;

	case S_JMP16:
		t = getnb();
		unget(t);
		if (t == '@') {
			t = addr(&e);
			if (t != S_AT_WREG) {
				xerr('a', "Indirect LJMP/LCALL requires @WRn.");
			} else {
				putcode(0x189 + (op == 0x12 ? 0x10 : 0));
				outab((e.e_addr << 4) | 0x04);
			}
		} else {
			expr(&e, 0);
			putcode(op);
			out_control16(&e);
		}
		break;

	case S_JMP24:
		t = getnb();
		unget(t);
		if (t == '@') {
			t = addr(&e);
			if (t != S_AT_DREG) {
				xerr('a', "Indirect EJMP/ECALL requires @DRn, @DPX, or @SPX.");
			} else {
				putcode(0x189 + (op == 0x19A ? 0x10 : 0));
				outab((e.e_addr << 4) | 0x08);
			}
		} else {
			expr(&e, 0);
			putcode(op);
			outr3b(&e, R_NORM);
		}
		break;

	case S_ACC:
		t = addr(&e);
		if (t != S_A && !(t == S_REG && e.e_addr == 11))
			xerr('a', "Argument must be A or R11.");
		putcode(op);
		break;

	case S_TYP1:
		/* Classic operands plus MCS251 register sizes and increments 1/2/4. */
		t = addr(&e);
		v1 = 0;
		if (more()) {
			comma(1);
			t1 = addr(&e1);
			if (t1 != S_IMMED || e1.e_flag || e1.e_base.e_ap != NULL) {
				xerr('a', "Increment must be the constant #1, #2, or #4.");
			} else if (e1.e_addr == 1) {
				v1 = 0;
			} else if (e1.e_addr == 2) {
				v1 = 1;
			} else if (e1.e_addr == 4) {
				v1 = 2;
			} else {
				xerr('a', "Increment must be #1, #2, or #4.");
			}
		}

		switch (t) {
		case S_A:
			if (v1 == 0)
				putcode(op + 4);
			else {
				putcode(0x10B + op);
				outab(0xB0 | v1);
			}
			break;

		case S_DIR:
		case S_EXT:
			if (v1 != 0)
				xerr('a', "Direct operands only support an increment of #1.");
			putcode(op + 5);
			outrb(&e, R_PAG0);
			break;

		case S_AT_R:
			if (v1 != 0)
				xerr('a', "Indirect byte operands only support an increment of #1.");
			putcode(op + 6 + e.e_addr);
			break;

		case S_REG:
			if (e.e_addr == 11 && v1 == 0) {
				putcode(op + 4);
			} else if (e.e_addr < 8 && v1 == 0 && !mcs251_source_mode) {
				putcode(op + 8 + e.e_addr);
			} else {
				putcode(0x10B + op);
				outab((e.e_addr << 4) | v1);
			}
			break;

		case S_WREG:
			putcode(0x10B + op);
			outab((e.e_addr << 4) | 0x04 | v1);
			break;

		case S_DREG:
			putcode(0x10B + op);
			outab((e.e_addr << 4) | 0x0C | v1);
			break;

		case S_DPTR:
			if (op != 0 || v1 != 0)
				xerr('a', "DPTR is valid only for INC DPTR.");
			else
				putcode(0xA3);
			break;

		default:
			xerr('a', "Invalid Addressing Mode.");
		}
		break;

	case S_TYP2:
		/* ADD has the complete MCS251 forms; ADDC/SUBB remain byte-accumulator ops. */
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);

		if (op != 0x20) {
			if (!((t == S_A) || (t == S_REG && e.e_addr == 11))) {
				xerr('a', "ADDC/SUBB first operand must be A or R11.");
				break;
			}
			switch (t1) {
			case S_IMMED:
				putcode(op + 4);
				outrb(&e1, R_NORM);
				break;
			case S_DIR:
				putcode(op + 5);
				outrb(&e1, R_PAG0);
				break;
			case S_AT_R:
				putcode(op + 6 + e1.e_addr);
				break;
			case S_REG:
				if (e1.e_addr > 7)
					xerr('a', "ADDC/SUBB register source must be R0 through R7.");
				else
					putcode(op + 8 + e1.e_addr);
				break;
			default:
				xerr('a', "Invalid ADDC/SUBB addressing mode.");
			}
			break;
		}

		v1 = register_width(t);
		if (v1 < 0) {
			xerr('a', "ADD destination must be A, Rn, WRn, or DRn.");
			break;
		}
		{
			int dst = register_number(t, &e);
			int src_width = register_width(t1);

			if (src_width >= 0) {
				int src = register_number(t1, &e1);

				if (src_width != v1) {
					xerr('a', "ADD register operands must have matching widths.");
				} else if ((t == S_A || dst == 11) && t1 == S_REG && src < 8) {
					putcode(0x28 + src);
				} else {
					putcode(0x12C + native_size_code(v1));
					outab((dst << 4) | src);
				}
				break;
			}

			switch (t1) {
			case S_IMMED:
				if (v1 == 0 && dst == 11) {
					putcode(0x24);
					outrb(&e1, R_NORM);
				} else {
					putcode(0x12E);
					outab((dst << 4) | (v1 << 2));
					out_immediate(&e1, v1);
				}
				break;
			case S_DIR:
				if (v1 == 0 && dst == 11) {
					putcode(0x25);
					outrb(&e1, R_PAG0);
				} else if (v1 == 2) {
					xerr('a', "ADD does not support a direct source for DRn.");
				} else {
					putcode(0x12E);
					outab((dst << 4) | (v1 << 2) | 1);
					outrb(&e1, R_PAG0);
				}
				break;
			case S_EXT:
				if (v1 == 2) {
					xerr('a', "ADD does not support a 16-bit direct source for DRn.");
				} else {
					putcode(0x12E);
					outab((dst << 4) | (v1 << 2) | 3);
					outrw(&e1, R_NORM);
				}
				break;
			case S_AT_R:
				if (v1 != 0 || dst != 11)
					xerr('a', "ADD @Rn source requires A or R11 destination.");
				else
					putcode(0x26 + e1.e_addr);
				break;
			case S_AT_WREG:
			case S_AT_DREG:
				if (v1 != 0) {
					xerr('a', "MCS251 indirect ADD sources are byte-sized.");
				} else {
					putcode(0x12E);
					outab((e1.e_addr << 4) | 0x09 |
					      (t1 == S_AT_DREG ? 2 : 0));
					outab(dst << 4);
				}
				break;
			default:
				xerr('a', "Invalid ADD addressing mode.");
			}
		}
		break;

	case S_SUBCMP:
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);
		v1 = register_width(t);
		if (v1 < 0) {
			xerr('a', "SUB/CMP destination must be A, Rn, WRn, or DRn.");
			break;
		}
		{
			int dst = register_number(t, &e);
			int src_width = register_width(t1);

			if (src_width >= 0) {
				if (src_width != v1) {
					xerr('a', "SUB/CMP register operands must have matching widths.");
				} else {
					putcode(op + 0x0C + native_size_code(v1));
					outab((dst << 4) | register_number(t1, &e1));
				}
				break;
			}

			switch (t1) {
			case S_IMMED:
				putcode(op + 0x0E);
				if (v1 == 2 && op == 0x1B0 &&
				    (e1.e_addr & 0xFF0000) == 0xFF0000)
					outab((dst << 4) | 0x0C);
				else
					outab((dst << 4) | (v1 << 2));
				out_immediate(&e1, v1);
				break;
			case S_DIR:
			case S_EXT:
				if (v1 == 2) {
					xerr('a', "SUB/CMP does not support direct sources for DRn.");
				} else {
					putcode(op + 0x0E);
					outab((dst << 4) | (v1 << 2) |
					      (t1 == S_DIR ? 1 : 3));
					out_direct(&e1, t1);
				}
				break;
			case S_AT_WREG:
			case S_AT_DREG:
				if (v1 != 0) {
					xerr('a', "MCS251 indirect SUB/CMP sources are byte-sized.");
				} else {
					putcode(op + 0x0E);
					outab((e1.e_addr << 4) | 0x09 |
					      (t1 == S_AT_DREG ? 2 : 0));
					outab(dst << 4);
				}
				break;
			default:
				xerr('a', "Invalid SUB/CMP addressing mode.");
			}
		}
		break;

	case S_TYP3:
		t = addr(&e);
		comma(1);
		if (t == S_C) {
			struct mcs251_bit bit;
			int inverted;
			int bit_mode;

			if (op == 0x60) {
				xerr('a', "XRL does not support boolean operands.");
				break;
			}
			bit_mode = parse_bit(&bit, &inverted);
			if (bit_mode == BIT_CLASSIC) {
				putcode(inverted ? op + 0x60 : op + 0x32);
				outrb(&bit.address, R_PAG0);
			} else if (bit_mode == BIT_EXTENDED) {
				int ext_opcode;

				if (inverted)
					ext_opcode = op == 0x50 ? 0xF0 : 0xE0;
				else
					ext_opcode = op == 0x50 ? 0x80 : 0x70;
				out_extended_bit(ext_opcode, &bit);
			}
			break;
		}
		t1 = addr(&e1);

		if (t == S_DIR) {
			if ((t1 == S_A) || (t1 == S_REG && e1.e_addr == 11)) {
				putcode(op + 2);
				outrb(&e, R_PAG0);
			} else if (t1 == S_IMMED) {
				putcode(op + 3);
				outrb(&e, R_PAG0);
				outrb(&e1, R_NORM);
			} else {
				xerr('a', "Direct logic destination requires A/R11 or an immediate.");
			}
			break;
		}

		v1 = register_width(t);
		if (v1 < 0 || v1 == 2) {
			xerr('a', "Logic destination must be A, Rn, WRn, or direct byte.");
			break;
		}
		{
			int dst = register_number(t, &e);
			int src_width = register_width(t1);

			if (src_width >= 0) {
				int src = register_number(t1, &e1);

				if (src_width != v1 || v1 == 2) {
					xerr('a', "Logic register operands must have matching byte/word widths.");
				} else if (dst == 11 && t1 == S_REG && src < 8) {
					putcode(op + 8 + src);
				} else {
					putcode(op + 0x10C + v1);
					outab((dst << 4) | src);
				}
				break;
			}

			switch (t1) {
			case S_IMMED:
				if (v1 == 0 && dst == 11) {
					putcode(op + 4);
					outrb(&e1, R_NORM);
				} else {
					putcode(op + 0x10E);
					outab((dst << 4) | (v1 << 2));
					out_immediate(&e1, v1);
				}
				break;
			case S_DIR:
				if (v1 == 0 && dst == 11) {
					putcode(op + 5);
					outrb(&e1, R_PAG0);
				} else {
					putcode(op + 0x10E);
					outab((dst << 4) | (v1 << 2) | 1);
					outrb(&e1, R_PAG0);
				}
				break;
			case S_EXT:
				putcode(op + 0x10E);
				outab((dst << 4) | (v1 << 2) | 3);
				outrw(&e1, R_NORM);
				break;
			case S_AT_R:
				if (v1 != 0 || dst != 11)
					xerr('a', "@Rn logic source requires A or R11 destination.");
				else
					putcode(op + 6 + e1.e_addr);
				break;
			case S_AT_WREG:
			case S_AT_DREG:
				if (v1 != 0) {
					xerr('a', "MCS251 indirect logic sources are byte-sized.");
				} else {
					putcode(op + 0x10E);
					outab((e1.e_addr << 4) | 0x09 |
					      (t1 == S_AT_DREG ? 2 : 0));
					outab(dst << 4);
				}
				break;
			default:
				xerr('a', "Invalid logic addressing mode.");
			}
		}
		break;

	case S_TYP4:
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);

		if (!(t == S_A || (t == S_REG && e.e_addr == 11)) &&
		    (t1 == S_A || (t1 == S_REG && e1.e_addr == 11))) {
			struct expr swap_e = e;
			int swap_t = t;

			e = e1;
			t = t1;
			e1 = swap_e;
			t1 = swap_t;
		}
		if (!(t == S_A || (t == S_REG && e.e_addr == 11))) {
			xerr('a', "One XCH operand must be A or R11.");
			break;
		}
		switch (t1) {
		case S_DIR:
			putcode(op + 5);
			outrb(&e1, R_PAG0);
			break;

		case S_AT_R:
			putcode(op + 6 + e1.e_addr);
			break;

		case S_REG:
			if (e1.e_addr > 7)
				xerr('a', "XCH register operand must be R0 through R7.");
			else
				putcode(op + 8 + e1.e_addr);
			break;

		default:
			xerr('a', "Invalid Addressing Mode.");
		}
		break;

	/* MOV instruction, all modes */
	case S_MOV:
		if (bit_syntax_before_comma()) {
			struct mcs251_bit bit;
			int inverted;
			int bit_mode = parse_bit(&bit, &inverted);

			comma(1);
			t1 = addr(&e1);
			if (inverted || t1 != S_C) {
				xerr('a', "MOV bit destination requires C/CY source.");
			} else if (bit_mode == BIT_CLASSIC) {
				putcode(0x92);
				outrb(&bit.address, R_PAG0);
			} else if (bit_mode == BIT_EXTENDED) {
				out_extended_bit(0x90, &bit);
			}
			break;
		}
		t = addr(&e);
		comma(1);
		if (t == S_C) {
			struct mcs251_bit bit;
			int inverted;
			int bit_mode = parse_bit(&bit, &inverted);

			if (inverted) {
				xerr('a', "MOV C/CY does not accept an inverted bit.");
			} else if (bit_mode == BIT_CLASSIC) {
				putcode(0xA2);
				outrb(&bit.address, R_PAG0);
			} else if (bit_mode == BIT_EXTENDED) {
				out_extended_bit(0xA0, &bit);
			}
			break;
		}
		t1 = addr(&e1);

		if (t == S_DPTR) {
			if (t1 != S_IMMED)
				xerr('a', "MOV DPTR requires a 16-bit immediate.");
			else {
				putcode(0x90);
				outrw(&e1, R_NORM);
			}
			break;
		}

		v1 = register_width(t);
		if (v1 >= 0) {
			int dst = register_number(t, &e);
			int src_width = register_width(t1);

			if (src_width >= 0) {
				int src = register_number(t1, &e1);

				if (src_width != v1) {
					xerr('a', "MOV register operands must have matching widths.");
				} else if (v1 == 0 && src == 11 && dst < 8) {
					putcode(0xF8 + dst);
				} else if (v1 == 0 && dst == 11 && src < 8) {
					putcode(0xE8 + src);
				} else {
					putcode(0x17C + native_size_code(v1));
					outab((dst << 4) | src);
				}
				break;
			}

			switch (t1) {
			case S_IMMED:
				if (v1 == 0 && dst == 11) {
					putcode(0x74);
					outrb(&e1, R_NORM);
				} else if (v1 == 0 && dst < 8 && !mcs251_source_mode) {
					putcode(0x78 + dst);
					outrb(&e1, R_NORM);
				} else {
					putcode(0x17E);
					if (v1 == 2 && (e1.e_addr & 0xFF0000) == 0xFF0000)
						outab((dst << 4) | 0x0C);
					else
						outab((dst << 4) | (v1 << 2));
					out_immediate(&e1, v1);
				}
				break;
			case S_DIR:
				if (v1 == 0 && dst == 11) {
					putcode(0xE5);
					outrb(&e1, R_PAG0);
				} else if (v1 == 0 && dst < 8 && !mcs251_source_mode) {
					putcode(0xA8 + dst);
					outrb(&e1, R_PAG0);
				} else {
					putcode(0x17E);
					outab((dst << 4) | (native_size_code(v1) << 2) | 1);
					outrb(&e1, R_PAG0);
				}
				break;
			case S_EXT:
				putcode(0x17E);
				outab((dst << 4) | (native_size_code(v1) << 2) | 3);
				outrw(&e1, R_NORM);
				break;
			case S_AT_R:
				if (v1 != 0 || dst != 11)
					xerr('a', "MOV from @Rn requires A or R11 destination.");
				else
					putcode(0xE6 + e1.e_addr);
				break;
			case S_AT_WREG:
			case S_AT_DREG:
				if (v1 == 0) {
					putcode(0x17E);
					outab((e1.e_addr << 4) | 0x09 |
					      (t1 == S_AT_DREG ? 2 : 0));
					outab(dst << 4);
				} else if (v1 == 1) {
					putcode(0x10B);
					outab((e1.e_addr << 4) | 0x08 |
					      (t1 == S_AT_DREG ? 2 : 0));
					outab(dst << 4);
				} else {
					xerr('a', "MOV cannot load DRn through a register pointer.");
				}
				break;
			case S_IDX_WREG:
			case S_IDX_DREG:
				if (v1 == 2) {
					xerr('a', "Indexed MOV supports byte and word data only.");
				} else {
					putcode(0x109 + (t1 == S_IDX_DREG ? 0x20 : 0) +
					        (v1 << 6));
					outab((dst << 4) | indexed_register(&e1));
					out_displacement(&e1);
				}
				break;
			default:
				xerr('a', "Invalid MOV source addressing mode.");
			}
			break;
		}

		if (t == S_AT_R) {
			switch (t1) {
			case S_IMMED:
				putcode(0x76 + e.e_addr);
				outrb(&e1, R_NORM);
				break;
			case S_DIR:
				putcode(0xA6 + e.e_addr);
				outrb(&e1, R_PAG0);
				break;
			case S_A:
				putcode(0xF6 + e.e_addr);
				break;
			case S_REG:
				if (e1.e_addr == 11)
					putcode(0xF6 + e.e_addr);
				else
					xerr('a', "MOV @Rn register source must be A or R11.");
				break;
			default:
				xerr('a', "Invalid MOV @Rn source.");
			}
			break;
		}

		if (t == S_AT_WREG || t == S_AT_DREG ||
		    t == S_IDX_WREG || t == S_IDX_DREG) {
			int src_width = register_width(t1);
			int pointer_is_dword = t == S_AT_DREG || t == S_IDX_DREG;
			int pointer = (t == S_IDX_WREG || t == S_IDX_DREG) ?
			              indexed_register(&e) : e.e_addr;

			if (src_width < 0 || src_width > 1) {
				xerr('a', "MOV pointer destination requires a byte/word register source.");
			} else if (t == S_AT_WREG || t == S_AT_DREG) {
				putcode(src_width == 0 ? 0x17A : 0x11B);
				outab((pointer << 4) | (src_width == 0 ? 0x09 : 0x08) |
				      (pointer_is_dword ? 2 : 0));
				outab(register_number(t1, &e1) << 4);
			} else {
				putcode(0x119 + (pointer_is_dword ? 0x20 : 0) +
				        (src_width << 6));
				outab((register_number(t1, &e1) << 4) | pointer);
				out_displacement(&e);
			}
			break;
		}

		if (t == S_DIR || t == S_EXT) {
			int src_width = register_width(t1);

			if (src_width >= 0) {
				int src = register_number(t1, &e1);

				if (t == S_DIR && src_width == 0 && src == 11) {
					putcode(0xF5);
					outrb(&e, R_PAG0);
				} else if (t == S_DIR && src_width == 0 && src < 8 && !mcs251_source_mode) {
					putcode(0x88 + src);
					outrb(&e, R_PAG0);
				} else {
					putcode(0x17A);
					outab((src << 4) | (native_size_code(src_width) << 2) |
					      (t == S_DIR ? 1 : 3));
					out_direct(&e, t);
				}
			} else if (t == S_DIR && t1 == S_IMMED) {
				putcode(0x75);
				outrb(&e, R_PAG0);
				outrb(&e1, R_NORM);
			} else if (t == S_DIR && t1 == S_DIR) {
				putcode(0x85);
				outrb(&e1, R_PAG0);
				outrb(&e, R_PAG0);
			} else if (t == S_DIR && t1 == S_AT_R) {
				putcode(0x86 + e1.e_addr);
				outrb(&e, R_PAG0);
			} else if (t == S_DIR && t1 == S_C) {
				putcode(0x92);
				outrb(&e, R_PAG0);
			} else {
				xerr('a', "Invalid direct MOV source.");
			}
			break;
		}

		if (t == S_C) {
			if (t1 != S_DIR)
				xerr('a', "MOV C requires a bit address.");
			else {
				putcode(0xA2);
				outrb(&e1, R_PAG0);
			}
			break;
		}

		xerr('a', "Invalid MOV destination addressing mode.");
		break;

	case S_MOVH:
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);
		if (t != S_DREG || t1 != S_IMMED) {
			xerr('a', "MOVH requires DRn,#imm16.");
		} else {
			putcode(0x17A);
			outab((e.e_addr << 4) | 0x0C);
			outrw(&e1, R_NORM);
		}
		break;

	case S_MOVZS:
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);
		if (t != S_WREG || register_width(t1) != 0) {
			xerr('a', "MOVS/MOVZ requires WRn followed by a byte register.");
		} else {
			putcode(0x10A + op);
			outab((e.e_addr << 4) | register_number(t1, &e1));
		}
		break;

	case S_BITBR:   /* JB, JBC, JNB bit,rel */
		{
			struct mcs251_bit bit;
			int inverted;
			int bit_mode = parse_bit(&bit, &inverted);

			comma(1);
			expr(&e1, 0);
			if (inverted) {
				xerr('a', "Bit branches do not accept an inverted bit operand.");
			} else if (bit_mode == BIT_CLASSIC) {
				putcode(op);
				outrb(&bit.address, R_PAG0);
				out_relative(&e1);
			} else if (bit_mode == BIT_EXTENDED) {
				out_extended_bit(op, &bit);
				out_relative(&e1);
			}
		}
		break;

	case S_BR:  /* JC, JNC, JZ, JNZ */
		expr(&e1, 0);
		putcode(op);
		out_relative(&e1);
		break;

	case S_CJNE:
		/* A,#;  A,dir;  @R0,#;  @R1,#;  Rn,# */
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);

		/* Benny */
		comma(1);
                expr(&e2, 0);

		switch (t) {
		case S_A:
			if (t1 == S_IMMED) {
				putcode(op + 4);
				outrb(&e1, R_NORM);
			}
			else if (t1 == S_DIR) {
				putcode(op + 5);
				outrb(&e1, R_PAG0);
			} else {
				xerr('a', "Invalid Addressing Mode.");
			}
			break;

		case S_AT_R:
			putcode(op + 6 + e.e_addr);
			if (t1 != S_IMMED)
				xerr('a', "#__ is required second argument.");
			outrb(&e1, R_NORM);
			break;

		case S_REG:
			if (e.e_addr > 7)
				xerr('a', "CJNE register operand must be R0 through R7.");
			putcode(op + 8 + e.e_addr);
			if (t1 != S_IMMED)
				xerr('a', "#__ is required second argument.");
			outrb(&e1, R_NORM);
			break;

		default:
			xerr('a', "Invalid Addressing Mode.");
			break;
		}

		out_relative(&e2);
		break;

	case S_DJNZ:
		/* Dir,dest;  Reg,dest */
		t = addr(&e);
                /* sdcc svn rev #4994: fixed bug 1865114 */
		comma(1);
		expr(&e1, 0);

		switch (t) {
		case S_DIR:
			putcode(op + 5);
			outrb(&e, R_PAG0);
			break;

		case S_REG:
			if (e.e_addr > 7)
				xerr('a', "DJNZ register operand must be R0 through R7.");
			putcode(op + 8 + e.e_addr);
			break;

		default:
			xerr('a', "Invalid Addressing Mode.");
		}

		out_relative(&e1);
		break;

	case S_JMP:
	case S_CALL:
		t = getnb();
		unget(t);
		if (t == '@') {
			t = addr(&e);
			if (mp->m_type == S_JMP && t == S_AT_ADP) {
				putcode(0x73);
			} else if (t == S_AT_WREG || t == S_AT_DREG) {
				putcode(mp->m_type == S_CALL ? 0x199 : 0x189);
				outab((e.e_addr << 4) | (0x04 +
				      (t == S_AT_DREG ? 4 : 0)));
			} else {
				xerr('a', "Invalid indirect JMP/CALL operand.");
			}
		} else {
			int form;

			expr(&e, 0);
			form = select_control_form(&e, mp->m_type == S_JMP);
			switch (form) {
			case 0:
				putcode(0x80);
				out_relative(&e);
				break;
			case 1:
				outrwm(&e, R_J11 | R_MCS251_CONTROL,
				       mp->m_type == S_CALL ? 0x11 : 0x01);
				break;
			case 2:
				putcode(mp->m_type == S_CALL ? 0x12 : 0x02);
				out_control16(&e);
				break;
			default:
				putcode(mp->m_type == S_CALL ? 0x19A : 0x18A);
				outr3b(&e, R_NORM);
				break;
			}
		}
		break;

	case S_MOVC:
		/* A,@A+DPTR  A,@A+PC */
		t = addr(&e);
		if (t != S_A)
			xerr('a', "First argument must be A.");
		comma(1);
		t1 = addr(&e1);
		if (t1 == S_AT_ADP) {
			outab(0x93);
		} else
		if (t1 == S_AT_APC) {
			outab(0x83);
		} else {
			xerr('a', "MOVC A,@A+DPTR; A,@A+PC are the allowed modes.");
		}
		break;

	case S_MOVX:
		/* A/R11,@DPTR|@Rn and the reversed stores. */
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);

		if (t == S_A || (t == S_REG && e.e_addr == 11)) {
			switch (t1) {
			case S_AT_DP:
				putcode(0xE0);
				break;

			case S_AT_R:
				putcode(0xE2 + e1.e_addr);
				break;

			default:
				xerr('a', "Second argument must be @DPTR or @Rn.");
			}
		} else if (t == S_AT_DP) {
			if (t1 == S_A || (t1 == S_REG && e1.e_addr == 11)) {
				putcode(0xF0);
			} else {
				xerr('a', "Second argument must be A or R11.");
			}
		} else if (t == S_AT_R) {
			if (t1 == S_A || (t1 == S_REG && e1.e_addr == 11)) {
				putcode(0xF2 + e.e_addr);
			} else {
				xerr('a', "Second argument must be A or R11.");
			}
		} else {
			xerr('a', "Invalid Addressing Mode.");
		}
		break;

	/* MUL/DIV AB or matching byte/word register pairs. */
	case S_AB:
		t = addr(&e);
		if (!more()) {
			if (t != S_RAB)
				xerr('a', "One-operand form requires AB.");
			putcode(op);
			break;
		}
		comma(1);
		t1 = addr(&e1);
		if ((t != S_REG && t != S_WREG) || t1 != t) {
			xerr('a', "MUL/DIV operands must be matching byte or word registers.");
			break;
		}
		putcode((op == 0xA4 ? 0x1AC : 0x18C) + (t == S_WREG));
		outab((e.e_addr << 4) | e1.e_addr);
		break;

	case S_SHIFT:
		t = addr(&e);
		if (t == S_A) {
			putcode(op);
			outab(0xB0);
		} else if (t == S_REG) {
			putcode(op);
			outab(e.e_addr << 4);
		} else if (t == S_WREG) {
			putcode(op);
			outab((e.e_addr << 4) | 0x04);
		} else {
			xerr('a', "Shift operand must be A, Rn, or WRn.");
		}
		break;

	/* CLR or CPL:  A, C, or bit */
	case S_ACBIT:
		{
			char *operand_start = ip;
			int special = admode(reg251);

			ip = operand_start;
			if (special == A) {
				(void) addr(&e);
				if (op == 0xB2)
					putcode(0xF4);
				else
					putcode(0xE4);
			} else if (special == C) {
				(void) addr(&e);
				putcode(op + 1);
			} else {
				struct mcs251_bit bit;
				int inverted;
				int bit_mode = parse_bit(&bit, &inverted);

				if (inverted)
					xerr('a', "CLR/CPL bit operand cannot be inverted.");
				else if (bit_mode == BIT_CLASSIC) {
					putcode(op);
					outrb(&bit.address, R_PAG0);
				} else if (bit_mode == BIT_EXTENDED) {
					out_extended_bit(op & 0xF0, &bit);
				}
			}
		}
		break;

	/* SETB C or bit */
	case S_SETB:
		{
			char *operand_start = ip;
			int special = admode(reg251);

			ip = operand_start;
			if (special == C) {
				(void) addr(&e);
				putcode(op + 1);
			} else {
				struct mcs251_bit bit;
				int inverted;
				int bit_mode = parse_bit(&bit, &inverted);

				if (inverted)
					xerr('a', "SETB bit operand cannot be inverted.");
				else if (bit_mode == BIT_CLASSIC) {
					putcode(op);
					outrb(&bit.address, R_PAG0);
				} else if (bit_mode == BIT_EXTENDED) {
					out_extended_bit(op & 0xF0, &bit);
				}
			}
		}
		break;

	case S_STACK:
		t = addr(&e);
		if (t == S_DIR) {
			if (op == 2)
				xerr('a', "PUSHW does not accept a direct-byte operand.");
			else {
				putcode(0xC0 + (op << 4));
				outrb(&e, R_PAG0);
			}
		} else if (register_width(t) >= 0) {
			int width = register_width(t);

			if (op == 2)
				xerr('a', "PUSHW is the immediate-word spelling; use PUSH for registers.");
			else {
				putcode(0x1CA + (op << 4));
				outab((register_number(t, &e) << 4) | 0x08 |
				      native_size_code(width));
			}
		} else if (t == S_IMMED && op != 1) {
			int width = op == 2 || !is_abs(&e) ||
			            !immediate_fits(e.e_addr, 8);

			if (is_abs(&e) && !immediate_fits(e.e_addr, 16))
				xerr('a', "PUSH immediate must fit in 8 or 16 bits.");

			putcode(0x1CA);
			outab(0x02 | (width << 2));
			out_immediate(&e, width);
		} else {
			xerr('a', "Invalid PUSH/POP operand.");
		}
		break;

	/* direct */
	case S_DIRECT:
		t = addr(&e);
		if (t == S_A) {
			e.e_addr = 0xE0;
			e.e_mode = S_DIR;
		} else
		if ((t != S_DIR) && (t != S_EXT)) {
			xerr('a', "Argument must be an address.");
			break;
		}
		outab(op);
		outrb(&e, R_PAG0);
		break;

	/* XCHD A,@Rn */
	case S_XCHD:
		t = addr(&e);
		comma(1);
		t1 = addr(&e1);
		if (t == S_AT_R &&
		    (t1 == S_A || (t1 == S_REG && e1.e_addr == 11))) {
			putcode(op + e.e_addr);
		} else if ((t == S_A || (t == S_REG && e.e_addr == 11)) &&
		           t1 == S_AT_R) {
			putcode(op + e1.e_addr);
		} else {
			xerr('a', "XCHD requires A/R11 and @R0/@R1.");
		}
		break;

	default:
		opcycles = OPCY_ERR;
		xerr('o', "Internal Opcode Error.");
		break;
	}
	if (opcycles == OPCY_NONE) {
		opcycles = i51pg1[cb[0] & 0xFF];
	}
}

/*
 * Branch/Jump PCR Mode Check
 */
int
mchpcr(struct expr *esp)
{
	if (esp->e_base.e_ap == dot.s_area) {
		return(1);
	}
	if (esp->e_flag==0 && esp->e_base.e_ap==NULL) {
		/*
		 * Absolute Destination
		 *
		 * Use the global symbol '.__.ABS.'
		 * of value zero and force the assembler
		 * to use this absolute constant as the
		 * base value for the relocation.
		 */
		esp->e_flag = 1;
		esp->e_base.e_sp = &sym[1];
	}
	return(0);
}

/*
 * Machine specific initialization
 */

static int beenHere = 0;        /* set non-zero if we have done that... */

void
minit(void)
{
	struct sym	*sp;
	struct PreDef	*pd;
	int i;
	char pid[8];
	char *p;

	if (pass == 0)
		memset(relax_bits, 0, sizeof(relax_bits));
	relax_pointer = relax_bits;
	relax_mask = 1;

	/*
	 * Byte Order
	 */
	hilo = 1;

	/*
         * Address Space
	 */
        exprmasks(3);

	/*
	 * First time only:
	 *	add the pre-defined symbols to the table
	 *	as local symbols.
	 */
        if (beenHere == 0) {
		pd = preDef;
		while (pd->id) {
			strcpy(pid, pd->id);
			for (i=0; i<2; i++) {
				/*
				 * i == 0,  Create Upper Case Symbols
				 * i == 1,  Create Lower Case Symbols
				 */
				if (i == 1) {
					p = pid;
					while (*p) {
						*p = ccase[*p & 0x007F];
						p++;
					}
				}
				sp = lookup(pid);
				if (sp->s_type == S_NEW) {
					sp->s_addr = pd->value;
					sp->s_type = S_USER;
					sp->s_flag = S_LCL | S_ASG;
				}
			}
			pd++;
		}
                beenHere = 1;
	}
}
