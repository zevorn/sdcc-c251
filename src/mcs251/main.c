/*-------------------------------------------------------------------------
  main.c - MCS251 specific general functions

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 1999, Jean-Louis VERN.jlvern@writeme.com
  Copyright (C) 2000, Michael Hope

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
-------------------------------------------------------------------*/
/*
    This is the first ownership boundary between the MCS251 and MCS51 port
    implementations.  Shared MCS51 headers and symbols are removed in later
    atomic changes after this source-level split is regression tested.
*/
#include "common.h"
#include "main.h"
#include "ralloc.h"
#include "gen.h"
#include "peep.h"
#include "rtrack.h"
#include "dbuf_string.h"
#include "../SDCCutil.h"

static char _defaultRules[] =
{
#include "peeph.rul"
};

#define OPTION_SMALL_MODEL          "--model-small"
#define OPTION_MEDIUM_MODEL         "--model-medium"
#define OPTION_LARGE_MODEL          "--model-large"
#define OPTION_HUGE_MODEL           "--model-huge"
#define OPTION_STACK_SIZE           "--stack-size"

static OPTION _mcs251_options[] =
  {
    { 0, OPTION_SMALL_MODEL, NULL, "internal data space is used (default)"},
    { 0, OPTION_MEDIUM_MODEL, NULL, "external paged data space is used"},
    { 0, OPTION_LARGE_MODEL, NULL, "external data space is used"},
    { 0, OPTION_HUGE_MODEL, NULL, "functions are banked, data in external space"},
    { 0, OPTION_STACK_SIZE,  &options.stack_size, "Tells the linker to allocate this space for stack", CLAT_INTEGER },
    { 0, "--acall-ajmp",     &options.acall_ajmp, "Use acall/ajmp instead of lcall/ljmp" },
    { 0, "--no-ret-without-call", &options.no_ret_without_call, "Do not use ret independent of acall/lcall" },
    { 0, NULL }
  };

/* list of key words used by msc51 */
static char *_mcs251_keywords[] =
{
  "at",
  "banked",
  "bit",
  "code",
  "critical",
  "data",
  "far",
  "generic",
  "idata",
  "interrupt",
  "naked",
  "near",
  "nonbanked",
  "overlay",
  "pdata",
  "reentrant",
  "sbit",
  "sfr",
  "sfr16",
  "sfr32",
  "using",
  "xdata",
  NULL
};



void mcs251_assignRegisters (ebbIndex *);

static int regParmFlg = 0;      /* determine if we can register a parameter     */
static int regBitParmFlg = 0;   /* determine if we can register a bit parameter */
static struct sym_link *regParmFuncType;

extern void mcs251_init_asmops (void);

static void
_mcs251_init (void)
{
  asm_addTree (&asm_asxxxx_mapping);
  mcs251_init_asmops ();
}

static void
_mcs251_reset_regparm (struct sym_link *funcType)
{
  regParmFlg = 0;
  regBitParmFlg = 0;
  regParmFuncType = funcType;
}

static int
_mcs251_regparm (sym_link *l, bool reentrant)
{
  ++regParmFlg;

  if (IFFUNC_HASVARARGS (regParmFuncType))
    return 0;

  if (IS_STRUCT (l))
    return 0;

  // For struct return keep regs free for pushing hidden parameter.
  if (IS_STRUCT (regParmFuncType->next))
    return 0;

  if (IS_SPEC(l) && (SPEC_NOUN(l) == V_BIT))
    {
      /* bit parameters go to b0 thru b7 */
      if (reentrant && (regBitParmFlg < 8))
        {
          regBitParmFlg++;
          return 1000 + regBitParmFlg;
        }
      return 0;
    }

  bool is_regarg = mcs251IsRegArg (regParmFuncType, regParmFlg, 0);

  return (is_regarg ? regParmFlg : 0);
}

static bool
_mcs251_parseOptions (int *pargc, char **argv, int *i)
{
  /* TODO: allow port-specific command line options to specify
   * segment names here.
   */
  return FALSE;
}

static void
_mcs251_finaliseOptions (void)
{
  if (options.noXinitOpt)
    port->genXINIT=0;

  switch (options.model)
    {
    case MODEL_SMALL:
      port->mem.default_local_map = data;
      port->mem.default_globl_map = data;
      port->s.ptr_size = 3;
      break;
    case MODEL_MEDIUM:
      port->mem.default_local_map = pdata;
      port->mem.default_globl_map = pdata;
      port->s.ptr_size = 3;
      break;
    case MODEL_LARGE:
    case MODEL_HUGE:
      port->mem.default_local_map = xdata;
      port->mem.default_globl_map = xdata;
      port->s.ptr_size = 3;
      break;
    default:
      port->mem.default_local_map = data;
      port->mem.default_globl_map = data;
      break;
    }

  if (options.omitFramePtr)
    port->stack.reent_overhead = 0;

  /* set up external stack location if not explicitly specified */
  if (!options.xstack_loc)
    options.xstack_loc = options.xdata_loc;
}

static void
_mcs251_setDefaultOptions (void)
{
  /* MCS251 generic and far pointers share one flat address space.  Keep XSEG
     out of page-zero edata by default; 0x010000 is also the STC32G on-chip
     XRAM base.  Users can still override this with --xram-loc.  MCS251 stack
     objects are addressed relative to the complete 16-bit SPX, so no legacy
     8-bit frame pointer is needed. */
  options.xdata_loc = 0x010000;
  options.omitFramePtr = 1;
}

static const char *
_mcs251_getRegName (const struct reg_info *reg)
{
  if (reg)
    return reg->name;
  return "err";
}

static void
_mcs251_genAssemblerStart (FILE * of)
{
  if (!options.noOptsdccInAsm)
    {
      fprintf (of, "\t.optsdcc -m%s", port->target);

      switch (options.model)
        {
        case MODEL_SMALL:
          fprintf (of, " --model-small");
          break;
        case MODEL_COMPACT:
          fprintf (of, " --model-compact");
          break;
        case MODEL_MEDIUM:
          fprintf (of, " --model-medium");
          break;
        case MODEL_LARGE:
          fprintf (of, " --model-large");
          break;
        case MODEL_HUGE:
          fprintf (of, " --model-huge");
          break;
        default:
          break;
        }
      /*if(options.stackAuto)      fprintf (asmFile, " --stack-auto"); */
      if (options.useXstack)
        fprintf (of, " --xstack");
      /*if(options.intlong_rent)   fprintf (asmFile, " --int-long-rent"); */
      /*if(options.float_rent)     fprintf (asmFile, " --float-rent"); */
      if (options.noRegParams)
        fprintf (of, " --no-reg-params");
      if (options.all_callee_saves)
        fprintf (of, " --all-callee-saves");
      fprintf (of, "\n");
    }
}

/* Generate interrupt vector table. */
static int
_mcs251_genIVT (struct dbuf_s *oBuf, symbol **interrupts, int maxInterrupts)
{
  int i;

  /* The reset slot is only three bytes wide.  Keep its LJMP in the reset
     region and let a HOME-area trampoline perform the full 24-bit jump to
     startup code, which may be placed elsewhere in the linear code space. */
  dbuf_printf (oBuf, "\tljmp\t__sdcc_mcs251_reset_trampoline\n");
  if(options.acall_ajmp && maxInterrupts)
    dbuf_printf (oBuf, "\t.ds\t1\n");

  /* now for the other interrupts */
  for (i = 0; i < maxInterrupts; i++)
    {
      if (interrupts[i])
        {
          /* Each MCS251 interrupt slot has room for a four-byte extended
             jump.  Unlike the three-byte reset slot, an interrupt handler
             may be linked outside the vector table's 64 KiB region. */
          dbuf_printf (oBuf, "\tejmp\t%s\n", interrupts[i]->rname);
          if (i != maxInterrupts - 1)
            dbuf_printf (oBuf, "\t.ds\t4\n");
        }
      else
        {
          dbuf_printf (oBuf, "\treti\n");
          if ( i != maxInterrupts - 1 )
            dbuf_printf (oBuf, "\t.ds\t7\n");
        }
    }

  dbuf_printf (oBuf, "__sdcc_mcs251_reset_trampoline::\n"
                     "\tejmp\t__sdcc_gsinit_startup\n");

  return true;
}

static void
_mcs251_genExtraAreas(FILE *of, bool hasMain)
{
  tfprintf (of, "\t!area\n", HOME_NAME);
  tfprintf (of, "\t!area\n", "GSINIT0 (CODE)");
  tfprintf (of, "\t!area\n", "GSINIT1 (CODE)");
  tfprintf (of, "\t!area\n", "GSINIT2 (CODE)");
  tfprintf (of, "\t!area\n", "GSINIT3 (CODE)");
  tfprintf (of, "\t!area\n", "GSINIT4 (CODE)");
  tfprintf (of, "\t!area\n", "GSINIT5 (CODE)");
  tfprintf (of, "\t!area\n", STATIC_NAME);
  tfprintf (of, "\t!area\n", port->mem.post_static_name);
  tfprintf (of, "\t!area\n", CODE_NAME);
}

static void
_mcs251_genInitStartup (FILE *of)
{
  tfprintf (of, "\t!global\n", "__sdcc_gsinit_startup");
  tfprintf (of, "\t!global\n", "__sdcc_program_startup");
  tfprintf (of, "\t!global\n", "__start__stack");

  if (options.useXstack)
    {
      tfprintf (of, "\t!global\n", "__sdcc_init_xstack");
      tfprintf (of, "\t!global\n", "__start__xstack");
    }

  // if the port can copy the XINIT segment to XISEG
  if (port->genXINIT)
    {
      port->genXINIT(of);
    }

  if (!getenv("SDCC_NOGENRAMCLEAR"))
    tfprintf (of, "\t!global\n", "__mcs51_genRAMCLEAR");
}


/* Generate code to copy XINIT to XISEG */
static void _mcs251_genXINIT (FILE * of)
{
  tfprintf (of, "\t!global\n", "__mcs51_genXINIT");

  if (!getenv("SDCC_NOGENRAMCLEAR"))
    tfprintf (of, "\t!global\n", "__mcs51_genXRAMCLEAR");
}


/* Do CSE estimation */
static bool cseCostEstimation (iCode *ic, iCode *pdic)
{
  operand *result = IC_RESULT(ic);
  sym_link *result_type = operandType(result);

  /* if it is a pointer then return ok for now */
  if (IC_RESULT(ic) && IS_PTR(result_type)) return 1;

  /* if bitwise | add & subtract then no since mcs51 is pretty good at it
     so we will cse only if they are local (i.e. both ic & pdic belong to
     the same basic block */
  if (IS_BITWISE_OP(ic) || ic->op == '+' || ic->op == '-')
    {
      /* then if they are the same Basic block then ok */
      if (ic->eBBlockNum == pdic->eBBlockNum) return 1;
      else return 0;
    }

  /* for others it is cheaper to do the cse */
  return 1;
}

/* Indicate which extended bit operations this port supports */
static bool
hasExtBitOp (int op, sym_link *left, int right)
{
  switch (op)
    {
    case GETABIT:
    case GETBYTE:
    case GETWORD:
      return true;
    case ROT:
      {
        unsigned int lbits = bitsForType (left);
        if (lbits % 8)
          return false;
        if (lbits == 8)
          return true;
        if (lbits <= 16 && (right % lbits  == 1 || right % lbits == lbits - 1))
          return true;
        if (lbits <= 16 && lbits == right * 2)
          return true;
      }
      return false;
    }
  return false;
}

/* Indicate the expense of an access to an output storage class */
static int
oclsExpense (struct memmap *oclass)
{
  if (IN_FARSPACE(oclass))
    return 1;

  return 0;
}

static bool
_hasNativeMulFor (iCode *ic, sym_link *left, sym_link *right)
{
  if (IS_BITINT (OP_SYM_TYPE (IC_RESULT(ic))) && SPEC_BITINTWIDTH (OP_SYM_TYPE (IC_RESULT(ic))) % 8)
    return false;

  if (getSize (left) == 1 && getSize (right) == 1)
    return true;

  return getSize (left) == 2 && getSize (right) == 2 &&
         SPEC_USIGN (getSpec (left)) && SPEC_USIGN (getSpec (right)) &&
         getSize (operandType (IC_RESULT (ic))) == 4;
}

static int
instructionSize(char *inst, char *op1, char *op2)
{
  #define ISINST(s) (strncmp(inst, (s), sizeof(s)-1) == 0)
  #define IS_A(s) (*(s) == 'a' && *(s+1) == '\0')
  #define IS_C(s) (*(s) == 'c' && *(s+1) == '\0')
  #define IS_Rn(s) (*(s) == 'r' && *(s+1) >= '0' && *(s+1) <= '7')
  #define IS_atRi(s) (*(s) == '@' && *(s+1) == 'r')

  /*
   * These are conservative upper bounds for MCS251 Source-mode encodings.
   * Area-II classic opcodes acquire an A5 prefix in Source mode, while
   * native register, extended-direct, and 24-bit forms can be wider than
   * their MCS-51 counterparts.  Peephole range checks must never
   * underestimate an instruction: an overestimate merely keeps a long
   * branch, but an underestimate can create an out-of-range short branch.
   */
  #define MCS251_ISINST(s) (strcmp (inst, (s)) == 0)

  if (MCS251_ISINST ("movc") || MCS251_ISINST ("movx")) return 1;
  if (MCS251_ISINST ("mov")) return 4;
  if (MCS251_ISINST ("movh")) return 4;
  if (MCS251_ISINST ("movs") || MCS251_ISINST ("movz")) return 2;

  if (MCS251_ISINST ("push") || MCS251_ISINST ("pushw")) return 4;
  if (MCS251_ISINST ("pop")) return 2;

  if (MCS251_ISINST ("ecall") || MCS251_ISINST ("ejmp")) return 4;
  if (MCS251_ISINST ("lcall") || MCS251_ISINST ("ljmp")) return 3;
  if (MCS251_ISINST ("acall") || MCS251_ISINST ("ajmp")) return 2;
  if (MCS251_ISINST ("call") || MCS251_ISINST ("jmp")) return 4;

  if (MCS251_ISINST ("ret") || MCS251_ISINST ("reti") ||
      MCS251_ISINST ("eret") || MCS251_ISINST ("nop") ||
      MCS251_ISINST ("trap") || MCS251_ISINST ("esc")) return 1;

  if (MCS251_ISINST ("rr") || MCS251_ISINST ("rrc") ||
      MCS251_ISINST ("rl") || MCS251_ISINST ("rlc") ||
      MCS251_ISINST ("swap") || MCS251_ISINST ("da")) return 1;
  if (MCS251_ISINST ("sra") || MCS251_ISINST ("srl") ||
      MCS251_ISINST ("sll")) return 2;

  if (MCS251_ISINST ("jc") || MCS251_ISINST ("jnc") ||
      MCS251_ISINST ("jz") || MCS251_ISINST ("jnz") ||
      MCS251_ISINST ("je") || MCS251_ISINST ("jne") ||
      MCS251_ISINST ("jg") || MCS251_ISINST ("jle") ||
      MCS251_ISINST ("jsg") || MCS251_ISINST ("jsge") ||
      MCS251_ISINST ("jsl") || MCS251_ISINST ("jsle") ||
      MCS251_ISINST ("sjmp")) return 2;
  if (MCS251_ISINST ("jb") || MCS251_ISINST ("jnb") ||
      MCS251_ISINST ("jbc")) return 4;
  if (MCS251_ISINST ("cjne")) return 4;
  if (MCS251_ISINST ("djnz")) return 3;

  if (MCS251_ISINST ("inc") || MCS251_ISINST ("dec")) return 2;
  if (MCS251_ISINST ("add") || MCS251_ISINST ("sub") ||
      MCS251_ISINST ("cmp")) return 4;
  if (MCS251_ISINST ("addc") || MCS251_ISINST ("subb") ||
      MCS251_ISINST ("xch")) return 2;
  if (MCS251_ISINST ("anl") || MCS251_ISINST ("orl") ||
      MCS251_ISINST ("xrl")) return 4;
  if (MCS251_ISINST ("clr") || MCS251_ISINST ("setb") ||
      MCS251_ISINST ("cpl")) return 3;
  if (MCS251_ISINST ("mul") || MCS251_ISINST ("div") ||
      MCS251_ISINST ("xchd")) return 2;

  #undef MCS251_ISINST

  /* Based on the current (2003-08-22) code generation for the
     small library, the top instruction probability is:

       57% mov/movx/movc
        6% push
        6% pop
        4% inc
        4% lcall
        4% add
        3% clr
        2% subb
  */
  /* mov, push, & pop are the 69% of the cases. Check them first! */
  if (ISINST ("mov"))
    {
      if (*(inst+3)=='x') return 1; /* movx */
      if (*(inst+3)=='c') return 1; /* movc */
      if (IS_C (op1) || IS_C (op2)) return 2;
      if (IS_A (op1))
        {
          if (IS_Rn (op2) || IS_atRi (op2)) return 1;
          return 2;
        }
      if (IS_Rn(op1) || IS_atRi(op1))
        {
          if (IS_A(op2)) return 1;
          return 2;
        }
      if (strcmp (op1, "dptr") == 0) return 3;
      if (IS_A (op2) || IS_Rn (op2) || IS_atRi (op2)) return 2;
      return 3;
    }

  if (ISINST ("push")) return 2;
  if (ISINST ("pop")) return 2;

  if (ISINST ("lcall")) return 3;
  if (ISINST ("ecall")) return *op1 == '@' ? 2 : 4;
  if (ISINST ("eret")) return 1;
  if (ISINST ("ejmp")) return *op1 == '@' ? 2 : 4;
  if (ISINST ("ret")) return 1;
  if (ISINST ("ljmp")) return 3;
  if (ISINST ("sjmp")) return 2;
  if (ISINST ("rlc")) return 1;
  if (ISINST ("rrc")) return 1;
  if (ISINST ("rl")) return 1;
  if (ISINST ("rr")) return 1;
  if (ISINST ("swap")) return 1;
  if (ISINST ("jc")) return 2;
  if (ISINST ("jnc")) return 2;
  if (ISINST ("jb")) return 3;
  if (ISINST ("jnb")) return 3;
  if (ISINST ("jbc")) return 3;
  if (ISINST ("jmp")) return 1; // always jmp @a+dptr
  if (ISINST ("jz")) return 2;
  if (ISINST ("jnz")) return 2;
  if (ISINST ("cjne")) return 3;
  if (ISINST ("mul")) return 1;
  if (ISINST ("div")) return 1;
  if (ISINST ("da")) return 1;
  if (ISINST ("xchd")) return 1;
  if (ISINST ("reti")) return 1;
  if (ISINST ("nop")) return 1;
  if (ISINST ("acall")) return 2;
  if (ISINST ("ajmp")) return 2;


  if (ISINST ("add") || ISINST ("addc") || ISINST ("subb") || ISINST ("xch"))
    {
      if (IS_Rn(op2) || IS_atRi(op2)) return 1;
      return 2;
    }
  if (ISINST ("inc") || ISINST ("dec"))
    {
      if (*op2) return 2;
      if (IS_A(op1) || IS_Rn(op1) || IS_atRi(op1)) return 1;
      if (strcmp(op1, "dptr") == 0) return 1;
      return 2;
    }
  if (ISINST ("anl") || ISINST ("orl") || ISINST ("xrl"))
    {
      if (IS_C(op1)) return 2;
      if (IS_A(op1))
        {
          if (IS_Rn(op2) || IS_atRi(op2)) return 1;
          return 2;
        }
      else
        {
          if (IS_A(op2)) return 2;
          return 3;
        }
    }
  if (ISINST ("clr") || ISINST ("setb") || ISINST ("cpl"))
    {
      if (IS_A(op1) || IS_C(op1)) return 1;
      return 2;
    }
  if (ISINST ("djnz"))
    {
      if (IS_Rn(op1)) return 2;
      return 3;
    }

  /* If the instruction is unrecognized, we shouldn't try to optimize. */
  /* Return a large value to discourage optimization.                  */
  return 999;
}

static asmLineNode *
newAsmLineNode (void)
{
  asmLineNode *aln;

  aln = Safe_alloc ( sizeof (asmLineNode));
  aln->size = 0;
  aln->regsRead = NULL;
  aln->regsWritten = NULL;

  return aln;
}


typedef struct mcs51operanddata
  {
    char name[6];
    int regIdx1;
    int regIdx2;
  }
mcs51operanddata;

static mcs51operanddata mcs51operandDataTable[] =
  {
    {"a",    A_IDX,   -1},
    {"ab",   A_IDX,   B_IDX},
    {"ac",   CND_IDX, -1},
    {"acc",  A_IDX,   -1},
    {"ar0",  R0_IDX,  -1},
    {"ar1",  R1_IDX,  -1},
    {"ar2",  R2_IDX,  -1},
    {"ar3",  R3_IDX,  -1},
    {"ar4",  R4_IDX,  -1},
    {"ar5",  R5_IDX,  -1},
    {"ar6",  R6_IDX,  -1},
    {"ar7",  R7_IDX,  -1},
    {"b",    B_IDX,   -1},
    {"c",    CND_IDX, -1},
    {"cy",   CND_IDX, -1},
    {"dph",  DPH_IDX, -1},
    {"dpl",  DPL_IDX, -1},
    {"dptr", DPL_IDX, DPH_IDX},
    {"f0",   CND_IDX, -1},
    {"f1",   CND_IDX, -1},
    {"ov",   CND_IDX, -1},
    {"p",    CND_IDX, -1},
    {"psw",  CND_IDX, -1},
    {"r0",   R0_IDX,  -1},
    {"r1",   R1_IDX,  -1},
    {"r10",  B_IDX,   -1},
    {"r11",  A_IDX,   -1},
    {"r12",  R12_IDX, -1},
    {"r13",  R13_IDX, -1},
    {"r14",  R14_IDX, -1},
    {"r15",  R15_IDX, -1},
    {"r2",   R2_IDX,  -1},
    {"r3",   R3_IDX,  -1},
    {"r4",   R4_IDX,  -1},
    {"r5",   R5_IDX,  -1},
    {"r6",   R6_IDX,  -1},
    {"r7",   R7_IDX,  -1},
    {"r8",   R8_IDX,  -1},
    {"r9",   R9_IDX,  -1},
  };

static int
mcs51operandCompare (const void *key, const void *member)
{
  return strcmp((const char *)key, ((mcs51operanddata *)member)->name);
}

static void
mcs251UpdateRegRW (asmLineNode *aln, int regIdx, const char *optype)
{
  if (regIdx < 0 || regIdx >= END_IDX)
    return;

  if (strchr (optype, 'r'))
    aln->regsRead = bitVectSetBit (aln->regsRead, regIdx);
  if (strchr (optype, 'w'))
    aln->regsWritten = bitVectSetBit (aln->regsWritten, regIdx);
}

static bool
mcs251UpdateWideOperandRW (asmLineNode *aln, const char *op,
                         const char *optype)
{
  const bool indirect = *op == '@';
  const char *name = indirect ? op + 1 : op;
  char *end;
  long first;
  int bytes;

  if (!strcmp (name, "dpx") || !strncmp (name, "dpx+", 4))
    {
      const char *access = indirect ? "r" : optype;
      mcs251UpdateRegRW (aln, DPL_IDX, access);
      mcs251UpdateRegRW (aln, DPH_IDX, access);
      /* DPXL has no slot in the inherited MCS-51 register tracker.  Treat B
         as a conservative proxy so an optimization cannot move through a
         24-bit pointer update unnoticed. */
      mcs251UpdateRegRW (aln, B_IDX, access);
      return true;
    }
  if (!strcmp (name, "dpxl"))
    {
      mcs251UpdateRegRW (aln, B_IDX, optype);
      return true;
    }

  if (!strncmp (name, "wr", 2))
    bytes = 2;
  else if (!strncmp (name, "dr", 2))
    bytes = strchr (optype, 'h') ? 2 : 4;
  else
    return false;

  first = strtol (name + 2, &end, 10);
  if (end == name + 2 || (*end && *end != '+') || first < 0 || first > 31)
    return false;

  for (int i = 0; i < bytes; ++i)
    if (first + i < MCS251_BYTE_REG_COUNT)
      mcs251UpdateRegRW (
        aln, mcs251_regIdxForNumber (first + i),
        indirect ? "r" : optype);
  return true;
}

static void
updateOpRW (asmLineNode *aln, const char *op_in, const char *optype)
{
  mcs51operanddata *opdat;

  /* There are two bit instructions that accept negated souce bit operand,
     where a leading '/' denotes the negation.  Ignore that here.  */
  if (*op_in == '/')
    op_in += 1;

  if (mcs251UpdateWideOperandRW (aln, op_in, optype))
    return;

  /* Ignore dots or brackets in operand (bit numbes) for operand table search.
     But remember that it's a bit access for special case handling.  */
  char op[32];
  strncpy (op, op_in, 31);
  op[31] = '\0';

  char *bit_sep;
  if (bit_sep = strchr (op, '.'))
    *bit_sep = '\0';
  else if (bit_sep = strchr (op, '['))
    *bit_sep = '\0';
  opdat = bsearch (op, mcs51operandDataTable,
                   sizeof(mcs51operandDataTable)/sizeof(mcs51operanddata),
                   sizeof(mcs51operanddata), mcs51operandCompare);

  if (opdat && strchr(optype,'r'))
    {
      if (opdat->regIdx1 >= 0)
        aln->regsRead = bitVectSetBit (aln->regsRead, opdat->regIdx1);
      if (opdat->regIdx2 >= 0)
        aln->regsRead = bitVectSetBit (aln->regsRead, opdat->regIdx2);
    }
  if (opdat && strchr(optype,'w'))
    {
      if (opdat->regIdx1 >= 0)
        aln->regsWritten = bitVectSetBit (aln->regsWritten, opdat->regIdx1);
      if (opdat->regIdx2 >= 0)
        aln->regsWritten = bitVectSetBit (aln->regsWritten, opdat->regIdx2);

      /* Any bit access always implies a read of the full register.  */
      if (opdat->regIdx1 == A_IDX && bit_sep)
        aln->regsRead = bitVectSetBit (aln->regsRead, A_IDX);

      if (opdat->regIdx1 == B_IDX && bit_sep)
        aln->regsRead = bitVectSetBit (aln->regsRead, B_IDX);
    }
  if (op[0] == '@')
    {
      if (!strcmp(op, "@r0"))
        aln->regsRead = bitVectSetBit (aln->regsRead, R0_IDX);
      if (!strcmp(op, "@r1"))
        aln->regsRead = bitVectSetBit (aln->regsRead, R1_IDX);
      if (strstr(op, "dptr"))
        {
          aln->regsRead = bitVectSetBit (aln->regsRead, DPL_IDX);
          aln->regsRead = bitVectSetBit (aln->regsRead, DPH_IDX);
        }
      if (strstr(op, "a+"))
        aln->regsRead = bitVectSetBit (aln->regsRead, A_IDX);
    }
}

typedef struct mcs51opcodedata
  {
    char name[6];
    char class[3];
    char pswtype[3];
    char op1type[3];
    char op2type[3];
  }
mcs51opcodedata;

static mcs51opcodedata mcs51opcodeDataTable[] =
  {
    {"acall","j", "",   "",   ""},
    {"add",  "",  "w",  "rw", "r"},
    {"addc", "",  "rw", "rw", "r"},
    {"ajmp", "j", "",   "",   ""},
    {"anl",  "",  "",   "rw", "r"},
    {"cjne", "j", "w",  "r",  "r"},
    {"clr",  "",  "",   "w",  ""},
    {"cmp",  "",  "w",  "r",  "r"},
    {"cpl",  "",  "",   "rw", ""},
    {"da",   "",  "rw", "rw", ""},
    {"dec",  "",  "",   "rw", ""},
    {"div",  "",  "w",  "rw", "r"},
    {"djnz", "j", "",  "rw",  ""},
    {"ecall","j", "",   "",   ""},
    {"ejmp", "j", "",   "",   ""},
    {"eret", "j", "",   "",   ""},
    {"inc",  "",  "",   "rw", ""},
    {"jb",   "j", "",   "r",  ""},
    {"jbc",  "j", "",  "rw",  ""},
    {"jc",   "j", "",   "",   ""},
    {"jmp",  "j", "",  "",    ""},
    {"jnb",  "j", "",   "r",  ""},
    {"jnc",  "j", "",   "",   ""},
    {"jnz",  "j", "",  "",    ""},
    {"jz",   "j", "",  "",    ""},
    {"lcall","j", "",   "",   ""},
    {"ljmp", "j", "",   "",   ""},
    {"mov",  "",  "",   "w",  "r"},
    {"movc", "",  "",   "w",  "r"},
    {"movh", "",  "",   "wh", "r"},
    {"movs", "",  "",   "w",  "r"},
    {"movx", "",  "",   "w",  "r"},
    {"movz", "",  "",   "w",  "r"},
    {"mul",  "",  "w",  "rw", "r"},
    {"nop",  "",  "",   "",   ""},
    {"orl",  "",  "",   "rw", "r"},
    {"pop",  "",  "",   "w",  ""},
    {"push", "",  "",   "r",  ""},
    {"ret",  "j", "",   "",   ""},
    {"reti", "j", "",   "",   ""},
    {"rl",   "",  "",   "rw", ""},
    {"rlc",  "",  "rw", "rw", ""},
    {"rr",   "",  "",   "rw", ""},
    {"rrc",  "",  "rw", "rw", ""},
    {"setb", "",  "",   "w",  ""},
    {"sjmp", "j", "",   "",   ""},
    {"sll",  "",  "w",  "rw", "r"},
    {"sra",  "",  "w",  "rw", "r"},
    {"srl",  "",  "w",  "rw", "r"},
    {"sub",  "",  "w",  "rw", "r"},
    {"subb", "",  "rw", "rw", "r"},
    {"swap", "",  "",   "rw", ""},
    {"xch",  "",  "",   "rw", "rw"},
    {"xchd", "",  "",   "rw", "rw"},
    {"xrl",  "",  "",   "rw", "r"},
  };

static int
mcs51opcodeCompare (const void *key, const void *member)
{
  return strcmp((const char *)key, ((mcs51opcodedata *)member)->name);
}

static const char* skip_spaces (const char* p)
{
  while (*p && isspace(*p)) p++;
  return p;
}

static asmLineNode *
asmLineNodeFromLineNode (lineNode *ln)
{
  asmLineNode *aln = newAsmLineNode();
  char *op, op1[256], op2[256];
  int opsize;
  const char *p;
  char inst[8];
  mcs51opcodedata *opdat;
  bool op_ignore_case;

  p = ln->line;

  /* extract instruction */

  p = skip_spaces (p);
  for (op = inst, opsize=1; *p; p++)
    {
      if (isspace(*p) || *p == ';' || *p == ':' || *p == '=')
        break;
      else
        if (opsize < sizeof(inst))
          *op++ = tolower(*p), opsize++;
    }
  *op = '\0';

  if (*p == ';' || *p == ':' || *p == '=')
    return aln;

  p = skip_spaces (p);
  if (*p == '=')
    return aln;


  /* extract first operand.  if it starts with '_' that usually means
     it's a case sensitive symbol from c code.  */
  op_ignore_case = *p != '_';

  for (op = op1, opsize=1; *p && *p != ','; p++)
    {
      if (!isspace(*p) && opsize < sizeof(op1))
        *op++ = (op_ignore_case ? tolower(*p) : *p), opsize++;
    }
  *op = '\0';

  if (*p == ',') p++;

  /* extract second operand.  if it starts with '_' that usually means
     it's a case sensitive symbol from c code.  */
  p = skip_spaces (p);
  op_ignore_case = *p != '_';

  for (op = op2, opsize=1; *p && *p != ','; p++)
    {
      if (!isspace(*p) && opsize < sizeof(op2))
        *op++ = (op_ignore_case ? tolower(*p) : *p), opsize++;
    }
  *op = '\0';

  aln->size = instructionSize(inst, op1, op2);

  aln->regsRead = newBitVect (END_IDX);
  aln->regsWritten = newBitVect (END_IDX);

  opdat = bsearch (inst, mcs51opcodeDataTable,
                   sizeof(mcs51opcodeDataTable)/sizeof(mcs51opcodedata),
                   sizeof(mcs51opcodedata), mcs51opcodeCompare);

  if (opdat)
    {
      updateOpRW (aln, op1, opdat->op1type);
      updateOpRW (aln, op2, opdat->op2type);
      if (!strcmp (inst, "jnz") || !strcmp (inst, "jz"))
        aln->regsRead = bitVectSetBit (aln->regsRead, A_IDX);
      if (strchr(opdat->pswtype,'r'))
        aln->regsRead = bitVectSetBit (aln->regsRead, CND_IDX);
      if (strchr(opdat->pswtype,'w'))
        aln->regsWritten = bitVectSetBit (aln->regsWritten, CND_IDX);
    }

  return aln;
}

static int
getInstructionSize (lineNode *line)
{
  if (!line->aln)
    line->aln = (asmLineNodeBase *) asmLineNodeFromLineNode (line);

  return line->aln->size;
}

static bitVect *
getRegsRead (lineNode *line)
{
  if (!line->aln)
    line->aln = (asmLineNodeBase *) asmLineNodeFromLineNode (line);

  return line->aln->regsRead;
}

static bitVect *
getRegsWritten (lineNode *line)
{
  if (!line->aln)
    line->aln = (asmLineNodeBase *) asmLineNodeFromLineNode (line);

  return line->aln->regsWritten;
}

static const char * models[] =
{
  "mcs251-small",  "mcs251-small-xstack",  "mcs251-small-stack-auto",  "mcs251-small-xstack-auto",
  "mcs251-medium", "mcs251-medium-xstack", "mcs251-medium-stack-auto", "mcs251-medium-xstack-auto",
  "mcs251-large",  "mcs251-large-xstack",  "mcs251-large-stack-auto",  "mcs251-large-xstack-auto",
  "mcs251-huge",   "mcs251-huge-xstack",   "mcs251-huge-stack-auto",   "mcs251-huge-xstack-auto",
};

static const char *
get_model (void)
{
  int index;

  switch (options.model)
    {
    case MODEL_SMALL:
      index = 0;
      break;
    case MODEL_MEDIUM:
      index = 4;
      break;
    case MODEL_LARGE:
      index = 8;
      break;
    case MODEL_HUGE:
      index = 12;
      break;
    default:
      werror (W_UNKNOWN_MODEL, __FILE__, __LINE__);
      return "unknown";
    }
  if (options.stackAuto)
    index += 2;
  if (options.useXstack)
    index += 1;
  return models[index];
}

/** $1 is always the basename.
    $2 is always the output file.
    $3 varies
    $l is the list of extra options that should be there somewhere...
    $L is the list of extra options that should be passed on the command line...
    MUST be terminated with a NULL.
*/
static const char *_linkCmd[] =
{
  "sdld", "-r", "-nf", "$1", "$L", NULL
};

/* $3 is replaced by assembler.debug_opts resp. port->assembler.plain_opts */
static const char *_asmCmd[] =
{
  "sdas251", "$l", "$3", "$2", "$1.asm", NULL
};

static const char * const _libs[] = {
  "mcs251", STD_LIB, STD_INT_LIB, STD_LONG_LIB, "liblonglong", STD_FP_LIB,
  NULL,
};

/* Globals */
PORT mcs251_port =
{
  TARGET_ID_MCS251,
  "mcs251",
  "Intel MCS-251",             /* Target name */
  NULL,                         /* Processor name */
  {
    glue,
    TRUE,                       /* glue_up_main: Emit glue around main */
    MODEL_SMALL | MODEL_LARGE,
    MODEL_SMALL,
    get_model,
  },
  {                             /* Assembler */
    _asmCmd,
    NULL,
    "-plosgffwy",               /* Options with debug */
    "-plosgffw",                /* Options without debug */
    0,
    ".asm",
    NULL                        /* no do_assemble function */
  },
  {                             /* Linker */
    _linkCmd,
    NULL,
    NULL,
    ".rel",
    1,
    NULL,                       /* crt */
    _libs,                      /* libs */
  },
  {                             /* Peephole optimizer */
    _defaultRules,
    getInstructionSize,
    getRegsRead,
    getRegsWritten,
    mcs251DeadMove,
    mcs251notUsed,
    mcs251CanAssign,
    mcs251notUsedFrom,
    NULL,
    NULL,
    NULL,
  },
  /* Sizes: char, short, int, long, long long, near ptr, far ptr, gptr, func ptr, banked func ptr, bit, float, _BitInt (in bits) */
  { 1, 2, 2, 4, 8, 1, 3, 3, 3, 3, 1, 4, 64 },
  /* tags for generic pointers */
  { 0x00, 0x40, 0x60, 0x80 },   /* far, near, xstack, code */
  {
    "XSTK    (PAG,XDATA)",      // xstack_name
    "STACK   (DATA)",           // istack_name
    "CSEG    (CODE)",           // code_name
    "DSEG    (DATA)",           // data_name
    "ISEG    (DATA)",           // idata_name
    "PSEG    (PAG,XDATA)",      // pdata_name
    "XSEG    (XDATA)",          // xdata_name
    NULL,                       // xconst_name
    "BSEG    (BIT)",            // bit_name
    "RSEG    (ABS,DATA)",       // reg_name
    "GSINIT  (CODE)",           // static_name
    "OSEG    (OVR,DATA)",       // overlay_name
    "GSFINAL (CODE)",           // post_static_name
    "HOME    (CODE)",           // home_name
    "XISEG   (XDATA)",          // xidata_name - initialized xdata
    "XINIT   (CODE)",           // xinit_name - a code copy of xiseg
    "CONST   (CODE)",           // const_name - const data (code or not)
    "CABS    (ABS,CODE)",       // cabs_name - const absolute data (code or not)
    "XABS    (ABS,XDATA)",      // xabs_name - absolute xdata/pdata
    "IABS    (ABS,DATA)",       // iabs_name - absolute idata/data
    NULL,                       // name of segment for initialized variables
    NULL,                       // name of segment for copies of initialized variables in code space
    NULL,
    NULL,
    1,
    false,                      // Flat MCS251 generic pointers address edata/xdata/code, not the direct SFR window.
    1                           // No fancy alignments supported.
  },
  { _mcs251_genExtraAreas, NULL },
  2,                            // SDCC MCS251 ABI revision
  {
    +1,         /* direction (+1 = stack grows up) */
    0,          /* bank_overhead (switch between register banks) */
    4,          /* isr_overhead */
    2,          /* call_overhead (3-byte extended return address - pre-increment) */
    1,          /* reent_overhead */
    1,          /* banked_overhead (switch between code banks) */
    0           /* sp points directly at last item pushed */
  },
  { -1, false, false },         // Neither int x int -> long nor unsigned long x unsigned char -> unsigned long long multiplication support routine.
  { mcs251_emitDebuggerSymbol },
  {
    256,        /* maxCount */
    2,          /* sizeofElement */
    {6,9,15},   /* sizeofMatchJump[] */
    {9,18,36},  /* sizeofRangeCompare[] */
    4,          /* sizeofSubtract */
    6,          /* sizeofDispatch */
  },
  "_",
  _mcs251_init,
  _mcs251_parseOptions,
  _mcs251_options,
  NULL,
  _mcs251_finaliseOptions,
  _mcs251_setDefaultOptions,
  mcs251_assignRegisters,
  _mcs251_getRegName,
  0,
  _mcs251_rtrackUpdate,
  _mcs251_keywords,
  _mcs251_genAssemblerStart,
  NULL,                         /* no genAssemblerEnd */
  _mcs251_genIVT,
  _mcs251_genXINIT,
  _mcs251_genInitStartup,
  _mcs251_reset_regparm,
  _mcs251_regparm,
  NULL,                         /* process_pragma */
  NULL,                         /* getMangledFunctionName */
  _hasNativeMulFor,             /* hasNativeMulFor */
  hasExtBitOp,                  /* hasExtBitOp */
  oclsExpense,                  /* oclsExpense */
  FALSE,                        /* use_dw_for_init */
  FALSE,                        /* Native MCS251 scalars are big-endian. */
  0,                            /* leave lt */
  0,                            /* leave gt */
  1,                            /* transform <= to ! > */
  1,                            /* transform >= to ! < */
  1,                            /* transform != to !(a == b) */
  0,                            /* leave == */
  FALSE,                        /* No array initializer support. */
  cseCostEstimation,
  "",                           // no builtin functions
  GPOINTER,                     /* treat unqualified pointers as "generic" pointers */
  true,                         // __far is a subspace of the generic space.
  true,                         // MCS251 generic and __far pointers share a flat 24-bit representation.
  1,                            /* reset labelKey to 1 */
  1,                            /* globals & local statics allowed */
  0,                            /* Number of registers handled in the tree-decomposition-based register allocator in SDCCralloc.hpp */
  PORT_MAGIC
};
