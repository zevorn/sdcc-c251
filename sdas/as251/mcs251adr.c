/* mcs251adr.c */

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
 *	This Assember Ported by
 *	John L. Hartman	(JLH)
 *	jhartman at compuserve dot com
 *	noice at noicedebugger dot com
 *
 */

#include "asxxxx.h"
#include "mcs251.h"


struct adsym reg251[] = {	/* R0 through R15 and special registers */
    {	"R0",	R0	},
    {	"R1",	R1	},
    {	"R2",	R2	},
    {	"R3",	R3	},
    {	"R4",	R4	},
    {	"R5",	R5	},
    {	"R6",	R6	},
    {	"R7",	R7	},
    {	"R8",	R8	},
    {	"R9",	R9	},
    {	"R10",	R10	},
    {	"R11",	R11	},
    {	"R12",	R12	},
    {	"R13",	R13	},
    {	"R14",	R14	},
    {	"R15",	R15	},
    {	"A",	A	},
    {	"DPTR", DPTR	},
    {	"PC",	PC	},
    {	"C",	C	},
    {	"CY",	C	},
    {	"AB",	AB	},
    {	"",	0x00	}
};

struct adsym wreg251[] = {
    {   "WR0",   0 },
    {   "WR2",   1 },
    {   "WR4",   2 },
    {   "WR6",   3 },
    {   "WR8",   4 },
    {   "WR10",  5 },
    {   "WR12",  6 },
    {   "WR14",  7 },
    {   "WR16",  8 },
    {   "WR18",  9 },
    {   "WR20", 10 },
    {   "WR22", 11 },
    {   "WR24", 12 },
    {   "WR26", 13 },
    {   "WR28", 14 },
    {   "WR30", 15 },
    {   "",       0 }
};

struct adsym dreg251[] = {
    {   "DR0",  0 },
    {   "DR4",  1 },
    {   "DR8",  2 },
    {   "DR12", 3 },
    {   "DR16", 4 },
    {   "DR20", 5 },
    {   "DR24", 6 },
    {   "DR28", 7 },
    {   "DR56", 14 },
    {   "DPX",  14 },
    {   "DR60", 15 },
    {   "SPX",  15 },
    {   "",      0 }
};

static int
invalid_register_spelling(char *text)
{
	char *p = text;

	if ((*p == 'w' || *p == 'W' || *p == 'd' || *p == 'D') &&
	    (p[1] == 'r' || p[1] == 'R'))
		p += 2;
	else if (*p == 'r' || *p == 'R')
		p++;
	else
		return 0;
	if (*p < '0' || *p > '9')
		return 0;
	do {
		p++;
	} while (*p >= '0' && *p <= '9');
	return *p == '\0' || any(*p, " \t\n,];+-");
}

/*  Classify argument as to address mode */
int
addr(struct expr *esp)
{
	int c;
	int r;
	unsigned rd;
	char *start;
	struct PreDef *pd;

	if ((c = getnb()) == '#') {
		/*  Immediate mode */
		expr(esp, 0);
		esp->e_mode = S_IMMED;
	}
	else if (c == '@') {
		/* Byte, word and dword register-indirect forms. */
		if ((r = admode(reg251)) != -1) switch (r) {
		case R0:
			esp->e_mode = S_AT_R;
			esp->e_addr = R0;
			break;
		case R1:
			esp->e_mode = S_AT_R;
			esp->e_addr = R1;
			break;
		case DPTR:
			esp->e_mode = S_AT_DP;
			esp->e_addr = DPTR;
			break;
		case A:
			if (getnb() == '+') {
				rd = reg();
				if (rd == PC) {
					esp->e_mode = S_AT_APC;
					esp->e_addr = 0;
				} else if (rd == DPTR) {
					esp->e_mode = S_AT_ADP;
					esp->e_addr = 0;
				} else {
					xerr('a', "@A+DPTR and A,@A+PC are the allowed modes.");
				}
			} else
				xerr('a', "Invalid Addressing Mode.");
			break;
		default:
			xerr('a', "Only R0 and R1 are byte register-indirect operands.");
			break;
		} else if ((r = admode(wreg251)) != -1) {
			c = getnb();
			if (c == '+' || c == '-') {
				unget(c);
				expr(esp, 0);
				esp->e_addr = ((a_uint) r << 24) | (esp->e_addr & 0xFFFFFF);
				esp->e_mode = S_IDX_WREG;
			} else {
				unget(c);
				esp->e_addr = r;
				esp->e_mode = S_AT_WREG;
			}
		} else if ((r = admode(dreg251)) != -1) {
			c = getnb();
			if (c == '+' || c == '-') {
				unget(c);
				expr(esp, 0);
				esp->e_addr = ((a_uint) r << 24) | (esp->e_addr & 0xFFFFFF);
				esp->e_mode = S_IDX_DREG;
			} else {
				unget(c);
				esp->e_addr = r;
				esp->e_mode = S_AT_DREG;
			}
		} else {
			xerr('a', "Invalid register-indirect operand.");
		}

		esp->e_flag = 0;
		esp->e_base.e_ap = NULL;
	}
	else if (c == '*') {
		if ((c = getnb()) == '/') {
			/* Force inverted bit */
			expr(esp, 0);
			esp->e_mode = S_NOT_BIT;
		} else {
			unget(c);
			/* Force direct page */
			expr(esp, 0);
			esp->e_mode = S_DIR;
		}
		if (esp->e_addr & ~0xFF)
			xerr('d', "A Direct Page addressing error.");
	}
	else if (c == '/') {
		/* Force inverted bit  */
		expr(esp, 0);
		esp->e_mode = S_NOT_BIT;
	}
	else {
		unget(c);
		start = ip;
		c = getnb();
		if ((c == 's' || c == 'S') && get() == ':') {
			expr(esp, 0);
			esp->e_mode = S_DIR;
			if (!esp->e_flag && esp->e_base.e_ap == NULL &&
			    (esp->e_addr < 0x80 || esp->e_addr > 0xFF))
				xerr('d', "S: requires an SFR address from 0x80 through 0xFF.");
			return esp->e_mode;
		}
		ip = start;

		/* Try byte and special registers first, then the wide classes. */
		if ((esp->e_addr = admode(reg251)) != -1) {
			switch (esp->e_addr) {
			case A:
				esp->e_mode = S_A;
				break;
			case AB:
				esp->e_mode = S_RAB;
				break;
			case DPTR:
				esp->e_mode = S_DPTR;
				break;
			case PC:
				esp->e_mode = S_PC;
				break;
			case C:
				esp->e_mode = S_C;
				break;
			default:
				/* R0-R15 */
				esp->e_mode = S_REG;
			}
		} else if ((esp->e_addr = admode(wreg251)) != -1) {
			esp->e_mode = S_WREG;
		} else if ((esp->e_addr = admode(dreg251)) != -1) {
			esp->e_mode = S_DREG;
		} else {
			/* Named predefined SFRs retain direct-page semantics. */
			start = ip;
			for (pd = preDef; pd->id; pd++) {
				if (pd->value <= 0xFF && srch(pd->id)) {
					esp->e_addr = pd->value;
					esp->e_mode = S_DIR;
					esp->e_flag = 0;
					esp->e_base.e_ap = NULL;
					return esp->e_mode;
				}
				ip = start;
			}
			if (invalid_register_spelling(start))
				xerr('a', "Invalid MCS251 register name.");
			/* Must be an expression */
			esp->e_addr = 0;
			expr(esp, 0);
			if ((!esp->e_flag
			     && esp->e_base.e_ap == NULL
			     && !(esp->e_addr & ~(mcs251_source_mode ? 0xFF : 0x7F)))
			    || (!esp->e_flag
			        && esp->e_base.e_ap != NULL
			        && !(esp->e_base.e_ap->a_flag & (A_CODE | A_XDATA)))
			    || (mcs251_source_mode && esp->e_flag)) {
				esp->e_mode = S_DIR;
			} else {
				esp->e_mode = S_EXT;
			}
		}
	}
	return (esp->e_mode);
}

/*
 * When building a table that has variations of a common
 * symbol always start with the most complex symbol first.
 * for example if x, x+, and x++ are in the same table
 * the order should be x++, x+, and then x.  The search
 * order is then most to least complex.
 */

/*
 * When searching symbol tables that contain characters
 * not of type LTR16, eg with '-' or '+', always search
 * the more complex symbol tables first. For example:
 * searching for x+ will match the first part of x++,
 * a false match if the table with x+ is searched
 * before the table with x++.
 */

/*
 * Enter admode() to search a specific addressing mode table
 * for a match. Return the addressing value on a match or
 * -1 for no match.
 */
int
admode(struct adsym *sp)
{
	char *ptr;
	int i;
	unget(getnb());
	i = 0;
	while ( *(ptr = &sp[i].a_str[0]) ) {
		if (srch(ptr)) {
			return(sp[i].a_val);
		}
		i++;
	}
	return(-1);
}

/*
 *	srch --- does string match ?
 */
int
srch(char *str)
{
	char *ptr;
	ptr = ip;

	while (*ptr && *str) {
		if(ccase[*ptr & 0x007F] != ccase[*str & 0x007F])
			break;
		ptr++;
		str++;
	}
	if (ccase[*ptr & 0x007F] == ccase[*str & 0x007F]) {
		ip = ptr;
		return(1);
	}

	if (!*str)
                if (any(*ptr," \t\n,];+-")) {
			ip = ptr;
			return(1);
		}
	return(0);
}

/*
 *      any --- does str contain c?
 */
int
any(int c, char *str)
{
        while (*str)
                if(*str++ == c)
			return(1);
	return(0);
}

/*
 * Read a register name.  Return register value, -1 if no register found
 */
int
reg()
{
	struct mne *mp;
	char id[NCPS];

	getid(id, -1);
	if ((mp = mlookup(id))==NULL) {
		aerr();
		return (-1);
	}
	switch (mp->m_type) {
	case S_A:
	case S_AB:
	case S_DPTR:
	case S_PC:
	case S_REG:
	case S_WREG:
	case S_DREG:
		return ((int) mp->m_valu);

	default:
		return (-1);
	}
}
