/*-------------------------------------------------------------------------
  gen_lower.c - MCS251 code generation: emission, ASMOP, value access

  Included by gen.c to improve physical modularity.  This file is NOT
  compiled separately; it is part of the gen.c translation unit.

  Contains the bottom three layers of the code generator:
    1. Primitive code emission (emitcode wrappers, push/pop, mova/movb)
    2. ASMOP management (newAsmop, aopForSym, aopOp, freeAsmop, etc.)
    3. Value access (aopGet, aopPut, loadDptrFromOperand, genCopy, genMove)

  The operation generators (genPlus, genMinus, ...) and main dispatch
  remain in gen.c.

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 1999, Jean-Louis VERN.jlvern@writeme.com

  SPDX-License-Identifier: GPL-2.0-or-later
-------------------------------------------------------------------------*/

static char *zero = "#0x00";
static char *one = "#0x01";
static char *spname;

unsigned mcs251_fReturnSize = 4;  /* shared with ralloc.c */
const char **mcs251_fReturn = mcs251_fReturnRegisters;
const char *mcs251_fReturnRegisters[] = { "dpl", "dph", "b", "a", "r4", "r5", "r6", "r7" }; // Historically the registers used for return values. Got abused for other stuff since, then partially replaced by aopArg and aopRet.
static asmop *aopRet (sym_link *ftype);

static short rbank = -1;

#define REG_WITH_INDEX   mcs251_regWithIdx

#define AOP(op) op->aop
#define AOP_TYPE(op) AOP(op)->type
#define AOP_SIZE(op) AOP(op)->size
#define IS_AOP_PREG(x) (AOP(x) && (AOP_TYPE(x) == AOP_R1 || \
                                   AOP_TYPE(x) == AOP_R0))

#define AOP_NEEDSACC(x) (AOP(x) && (AOP_TYPE(x) == AOP_CRY ||  \
                                    AOP_TYPE(x) == AOP_DPTR || \
                                    AOP(x)->paged))

#define AOP_INPREG(x) (x && (x->type == AOP_REG &&                        \
                       (x->aopu.aop_reg[0] == REG_WITH_INDEX(R0_IDX) || \
                        x->aopu.aop_reg[0] == REG_WITH_INDEX(R1_IDX) )))

#define IS_AOP_IMMEDIATE(x) (AOP(x) && (AOP_TYPE(x) == AOP_LIT || \
                                        AOP_TYPE(x) == AOP_IMMD || \
                                        AOP_TYPE(x) == AOP_STR))

#define SP_BP(sp, bp) (options.omitFramePtr ? sp : bp)
#define SYM_BP(sym)   (SPEC_OCLS (sym->etype)->paged ? SP_BP("_spx", "_bpx") : SP_BP("sp", "_bp"))

#define EQ(a, b)      (strcmp (a, b) == 0)

#define R0INB   _G.bu.bs.r0InB
#define R1INB   _G.bu.bs.r1InB
#define OPINB   _G.bu.bs.OpInB
#define BITSINB _G.bu.bs.bitsInB
#define BINUSE  _G.bu.BInUse

static struct
{
  short r0Pushed;
  short r1Pushed;
  union
  {
    struct
    {
      short r0InB: 2;           //2 so we can see it overflow
      short r1InB: 2;           //2 so we can see it overflow
      short OpInB: 2;           //2 so we can see it overflow
      short bitsInB: 2;         //2 so we can see it overflow
    } bs;
    short BInUse;
  } bu;
  short accInUse;
  struct
  {
    int pushed;
    int pushedregs;
    int offset;
    int param_offset;
    int xpushed;
    int xpushedregs;
    int xoffset;
  } stack;
  set *sendSet;
  symbol *currentFunc;
  /* DPTR value tracking: when the code generator knows that dptr/dpxl
     already hold a particular far-symbol address, it can skip the
     redundant mov dptr,#X / mov dpxl,#Y pair.  The cache is invalidated
     at every label (control-flow join) and after any instruction that
     might modify dptr. */
  struct {
    char symbol[64];      /* cached dptr value from opGet() */
    char dpxlSymbol[64];  /* cached dpxl value from opGet() */
    bool valid;
  } dptrCache;
}
_G;

static char *rb1regs[] =
{
  "b1_0", "b1_1", "b1_2", "b1_3", "b1_4", "b1_5", "b1_6", "b1_7",
  "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7"
};

extern struct dbuf_s *codeOutBuf;

#define RESULTONSTACK(x) \
                         (IC_RESULT (x) && IC_RESULT (x)->aop && \
                         IC_RESULT (x)->aop->type == AOP_STK )

#define MOVA(x)  mova (x)       /* use function to avoid multiple eval */
#define MOVB(x)  movb (x)

#define CLRC     emitcode ("clr","c")
#define SETC     emitcode ("setb","c")
#define RUNTIME_CALL "ecall"
#define LONG_JUMP "ejmp"
#define LOOP_ENTRY_JUMP "ejmp"

static unsigned char SLMask[] = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0,
                                  0xE0, 0xC0, 0x80, 0x00
                                };

static unsigned char SRMask[] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F,
                                  0x07, 0x03, 0x01, 0x00
                                };

#define MAX_REGISTER_BANKS 4

#define LSB     0
#define MSB16   1
#define MSB24   2
#define MSB32   3

static bool isFixedByteRegisterOperand (const char *);

static struct asmop asmop_a, asmop_dpl, asmop_dptr, asmop_bdptr, asmop_abdptr, asmop_r4abdptr, asmop_r5r4abdptr, asmop_r6r5r4abdptr, asmop_r7r6r5r4abdptr;
static struct asmop *const ASMOP_A = &asmop_a;
static struct asmop *const ASMOP_DPL = &asmop_dpl;
static struct asmop *const ASMOP_DPTR = &asmop_dptr;
static struct asmop *const ASMOP_BDPTR = &asmop_bdptr;
static struct asmop *const ASMOP_ABDPTR = &asmop_abdptr;
static struct asmop *const ASMOP_R4ABDPTR = &asmop_r4abdptr;
static struct asmop *const ASMOP_R5R4ABDPTR = &asmop_r5r4abdptr;
static struct asmop *const ASMOP_R6R5R4ABDPTR = &asmop_r6r5r4abdptr;
static struct asmop *const ASMOP_R7R6R5R4ABDPTR = &asmop_r7r6r5r4abdptr;

// Init aop as a an asmop for data in registers, as given by the -1-terminated array regidx.
static void
mcs251_init_reg_asmop (asmop *aop, const signed char *regidx)
{
  aop->type = AOP_REG;
  aop->size = 0;
  memset (aop->regs, -1, sizeof(aop->regs));

  for(int i = 0; regidx[i] >= 0; i++)
    {
      aop->aopu.aop_reg[i] = mcs251_regs + regidx[i];
      aop->regs[regidx[i]] = i;
      aop->size++;
    }
}

void
mcs251_init_asmops (void)
{
  mcs251_init_reg_asmop(&asmop_a, (const signed char[]){A_IDX, -1});
  mcs251_init_reg_asmop(&asmop_dpl, (const signed char[]){DPL_IDX, -1});
  mcs251_init_reg_asmop(&asmop_dptr, (const signed char[]){DPL_IDX, DPH_IDX, -1});
  mcs251_init_reg_asmop(&asmop_bdptr, (const signed char[]){DPL_IDX, DPH_IDX, B_IDX, -1});
  mcs251_init_reg_asmop(&asmop_abdptr, (const signed char[]){DPL_IDX, DPH_IDX, B_IDX, A_IDX, -1});
  mcs251_init_reg_asmop(&asmop_r4abdptr, (const signed char[]){DPL_IDX, DPH_IDX, B_IDX, A_IDX, R4_IDX, -1});
  mcs251_init_reg_asmop(&asmop_r5r4abdptr, (const signed char[]){DPL_IDX, DPH_IDX, B_IDX, A_IDX, R4_IDX, R5_IDX, -1});
  mcs251_init_reg_asmop(&asmop_r6r5r4abdptr, (const signed char[]){DPL_IDX, DPH_IDX, B_IDX, A_IDX, R4_IDX, R5_IDX, R6_IDX, R7_IDX, -1});
  mcs251_init_reg_asmop(&asmop_r7r6r5r4abdptr, (const signed char[]){DPL_IDX, DPH_IDX, B_IDX, A_IDX, R4_IDX, R5_IDX,R6_IDX, R7_IDX, -1});
}

/*-----------------------------------------------------------------*/
/* mcs251_emitDebuggerSymbol - associate the current code location  */
/*   with a debugger symbol                                        */
/*-----------------------------------------------------------------*/
void
mcs251_emitDebuggerSymbol (const char *debugSym)
{
  genLine.lineElement.isDebug = 1;
  emitcode ("", "%s ==.", debugSym);
  genLine.lineElement.isDebug = 0;
}

/*-----------------------------------------------------------------*/
/* mova - moves specified value into accumulator                   */
/*-----------------------------------------------------------------*/
static void
mova (const char *x)
{
  /* do some early peephole optimization */
  if (!strncmp (x, "a", 2) || !strncmp (x, "acc", 4))
    return;

  /* if it is a literal mov try to get it cheaper */
  if (*x == '#' && mcs251_rtrackMoveALit (x))
    return;

  /* another early peephole optimization */
  if (EQ (x, "#0x00"))
    {
      emitcode ("clr", "a");
      return;
    }

  emitcode ("mov", "a,%s", x);
}

/*-----------------------------------------------------------------*/
/* movb - moves specified value into register b                    */
/*-----------------------------------------------------------------*/
static void
movb (const char *x)
{
  /* do some early peephole optimization */
  if (!strncmp (x, "b", 2))
    return;

  /* if it is a literal mov try to get it cheaper */
  if (*x == '#')
    {
      emitcode ("mov", "b,%s", mcs251_rtrackGetLit (x));
      return;
    }

  emitcode ("mov", "b,%s", x);
}

/* MCS251 uses the full 24-bit DPX register for flat data and code pointers.
   Keeping these operations here prevents individual lowering paths from
   accidentally falling back to a 16-bit DPTR update at a region boundary. */
static const char *
farPointerRegister (void)
{
  return "dpx";
}

static void
incrementFarPointer (void)
{
  _G.dptrCache.valid = false;
  emitcode ("inc", farPointerRegister ());
}

static void
decrementFarPointer (void)
{
  emitcode ("dec", "dpx");
}

static void
loadFarPointerByte (bool code)
{
  if (code)
    {
      emitcode ("clr", "a");
      emitcode ("movc", "a,@a+dptr");
    }
  else
    emitcode ("mov", "a,@dpx");
}

static void
storeFarPointerByte (void)
{
  emitcode ("mov", "@dpx,a");
}

/* A MCS251 far symbol has a relocatable 24-bit address.  Materialize it at
   the point of use.  More than one far asmop can be live in a single iCode;
   treating DPX as implicit per-operand state makes the last aopOp() silently
   alias all earlier operands. */
static void
mcs251LoadFarSymbol (asmop *aop, int offset)
{
  const char *name = aop->aopu.aop_sym->rname;

  wassert (aop->type == AOP_DPTR && aop->aopu.aop_sym);
  if (offset)
    {
      emitcode ("mov", "dptr,#(%s + %d)", name, offset);
      emitcode ("mov", "dpxl,#((%s + %d) >> 16)", name, offset);
    }
  else
    {
      emitcode ("mov", "dptr,#%s", name);
      emitcode ("mov", "dpxl,#(%s >> 16)", name);
    }
  aop->coff = offset;
}

/* DJNZ has an 8-bit displacement even in Source mode.  A MCS251 loop body
   can easily exceed that after 24-bit far-address materialization, so use
   a nearby DJNZ trampoline and an unrestricted EJMP.  Preserve the exact
   original instruction for MCS-51. */
static void
emitLoopDjnz (const char *counter, symbol *target)
{
  symbol *repeat = newiTempLabel (NULL);
  symbol *done = newiTempLabel (NULL);

  emitcode ("djnz", "%s,!tlabel", counter, labelKey2num (repeat->key));
  emitcode ("sjmp", "!tlabel", labelKey2num (done->key));
  emitLabel (repeat);
  emitcode ("ejmp", "!tlabel", labelKey2num (target->key));
  emitLabel (done);
}

/* The MCS251 hardware stack is the full DR60/SPX register.  Native INC/DEC
   forms change it without borrowing A, a general register, or carry; use the
   largest short increment first to keep large frames compact. */
static void
mcs251AdjustStackPointer (int amount)
{
  const char *instruction = amount < 0 ? "dec" : "inc";
  unsigned int remaining = amount < 0 ? -amount : amount;

  while (remaining >= 4)
    {
      emitcode (instruction, "spx,#4");
      remaining -= 4;
    }
  if (remaining >= 2)
    {
      emitcode (instruction, "spx,#2");
      remaining -= 2;
    }
  if (remaining)
    emitcode (instruction, "spx");
}

/*-----------------------------------------------------------------*/
/* emitpush - push something on internal stack                     */
/*-----------------------------------------------------------------*/
static void
emitpush (const char *arg)
{
  char buf[] = "ar?";

  _G.stack.pushed++;
  if (!arg)
    {
      mcs251AdjustStackPointer (1);
      return;
    }
  else if (EQ (arg, "a"))
    {
      arg = "acc";
    }
  else if ((*arg == '@') || (*arg == '#'))
    {
      MOVA (arg);
      arg = "acc";
    }
  else if (EQ (arg, "r0") || EQ (arg, "r1") || EQ (arg, "r2") || EQ (arg, "r3") ||
           EQ (arg, "r4") || EQ (arg, "r5") || EQ (arg, "r6") || EQ (arg, "r7"))
    {
      buf[2] = arg[1];
      arg = buf;
    }
  emitcode ("push", arg);
}

/*-----------------------------------------------------------------*/
/* emitpop - pop something from internal stack                     */
/*-----------------------------------------------------------------*/
static void
emitpop (const char *arg)
{
  if (!arg)
    mcs251AdjustStackPointer (-1);
  else
    emitcode ("pop", arg);
  _G.stack.pushed--;
  wassertl (_G.stack.pushed >= 0, "stack underflow");
}

/* XCH A,Rn only encodes R0-R7.  Preserve the same semantics for the fixed
   R8-R15 byte registers without borrowing another live register. */
static void
emitAccumulatorExchange (const char *operand)
{
  if (isFixedByteRegisterOperand (operand))
    {
      emitpush ("acc");
      MOVA (operand);
      emitpop (operand);
    }
  else
    emitcode ("xch", "a,%s", operand);
}

/*-----------------------------------------------------------------*/
/* pushB - saves register B if necessary                           */
/*-----------------------------------------------------------------*/
static bool
pushB (void)
{
  bool pushedB = FALSE;

  if (BINUSE)
    {
      emitpush ("b");
//    printf("B was in use !\n");
      pushedB = TRUE;
    }
  else
    {
      OPINB++;
    }
  return pushedB;
}

/*-----------------------------------------------------------------*/
/* popB - restores value of register B if necessary                */
/*-----------------------------------------------------------------*/
static void
popB (bool pushedB)
{
  if (pushedB)
    {
      emitpop ("b");
    }
  else
    {
      OPINB--;
    }
}

static void
emitRestrictedAccumulatorOp (const char *instruction, const char *source)
{
  if (isFixedByteRegisterOperand (source))
    {
      bool pushedB = pushB ();

      MOVB (source);
      emitcode (instruction, "a,b");
      popB (pushedB);
    }
  else
    emitcode (instruction, "a,%s", source);
}

/*-----------------------------------------------------------------*/
/* pushReg - saves register                                        */
/*-----------------------------------------------------------------*/
static bool
pushReg (int index, bool bits_pushed)
{
  const reg_info *reg = REG_WITH_INDEX (index);
  if (reg->type == REG_BIT)
    {
      if (!bits_pushed)
        emitpush (reg->base);
      return TRUE;
    }
  else
    emitpush (reg->dname);
  return bits_pushed;
}

/*-----------------------------------------------------------------*/
/* popReg - restores register                                      */
/*-----------------------------------------------------------------*/
static bool
popReg (int index, bool bits_popped)
{
  const reg_info *reg = REG_WITH_INDEX (index);
  if (reg->type == REG_BIT)
    {
      if (!bits_popped)
        emitpop (reg->base);
      return TRUE;
    }
  else
    emitpop (reg->dname);
  return bits_popped;
}

static void
saveFixedByteRegisters (const symbol *function, bool save_all)
{
  int i;

  for (i = MCS251_BANK_REG_COUNT; i < MCS251_BYTE_REG_COUNT; ++i)
    /* R10 and R11 are saved through their B and ACC aliases. */
    if (i != R10_IDX && i != R11_IDX &&
        (save_all || bitVectBitValue (function->regsUsed, i)))
      pushReg (i, TRUE);
}

static void
restoreFixedByteRegisters (const symbol *function, bool restore_all)
{
  int i;

  for (i = MCS251_BYTE_REG_COUNT - 1;
       i >= MCS251_BANK_REG_COUNT; --i)
    if (i != R10_IDX && i != R11_IDX &&
        (restore_all || bitVectBitValue (function->regsUsed, i)))
      popReg (i, TRUE);
}

#if 0
/*-----------------------------------------------------------------*/
/* showR0R1status - helper for debugging getFreePtr failures       */
/*-----------------------------------------------------------------*/
static void
showR0R1status(iCode * ic)
{
  bool r0iu, r1iu;
  bool r0ou, r1ou;

  r0iu = bitVectBitValue (ic->rUsed, R0_IDX);
  r1iu = bitVectBitValue (ic->rUsed, R1_IDX);
  printf ("ic->rUsed = [");
  if (r0iu)
    if (r1iu)
      printf("r0,r1");
    else
      printf("r0");
  else
    if (r1iu)
      printf("r1");
  printf("] ");

  r0ou = bitVectBitValue (ic->rMask, R0_IDX);
  r1ou = bitVectBitValue (ic->rMask, R1_IDX);
  printf ("ic->rMask = [");
  if (r0ou)
    if (r1ou)
      printf("r0,r1");
    else
      printf("r0");
  else
    if (r1ou)
      printf("r1");
  printf("]\n");
}
#endif

/*-----------------------------------------------------------------*/
/* getFreePtr - returns r0 or r1 whichever is free or can be pushed */
/*-----------------------------------------------------------------*/
static reg_info *
getFreePtr (iCode * ic, asmop * aop, bool result)
{
  bool r0iu, r1iu;
  bool r0ou, r1ou;

  /* the logic: if r0 & r1 used in the instruction
     then we are in trouble otherwise */

  /* first check if r0 & r1 are used by this
     instruction, in which case we are in trouble */
  r0iu = bitVectBitValue (ic->rUsed, R0_IDX);
  r1iu = bitVectBitValue (ic->rUsed, R1_IDX);
  if (r0iu && r1iu)
    {
      goto endOfWorld;
    }

  r0ou = bitVectBitValue (ic->rMask, R0_IDX);
  r1ou = bitVectBitValue (ic->rMask, R1_IDX);

  /* if no usage of r0 then return it */
  if (!r0iu && !r0ou)
    {
      ic->rUsed = bitVectSetBit (ic->rUsed, R0_IDX);
      aop->type = AOP_R0;

      return aop->aopu.aop_ptr = REG_WITH_INDEX (R0_IDX);
    }

  /* if no usage of r1 then return it */
  if (!r1iu && !r1ou)
    {
      ic->rUsed = bitVectSetBit (ic->rUsed, R1_IDX);
      aop->type = AOP_R1;

      return aop->aopu.aop_ptr = REG_WITH_INDEX (R1_IDX);
    }

  /* now we know they both have usage */
  /* if r0 not used in this instruction */
  if (!r0iu)
    {
      /* push it if not already pushed */
      if ((ic->op == IPUSH) || (ic->op == PCALL))
        {
          MOVB (REG_WITH_INDEX (R0_IDX)->dname);
          R0INB++;
        }
      else if (!_G.r0Pushed)
        {
          emitpush (REG_WITH_INDEX (R0_IDX)->dname);
          _G.r0Pushed++;
        }

      ic->rUsed = bitVectSetBit (ic->rUsed, R0_IDX);
      aop->type = AOP_R0;

      return aop->aopu.aop_ptr = REG_WITH_INDEX (R0_IDX);
    }

  /* if r1 not used then */

  if (!r1iu)
    {
      /* push it if not already pushed */
      if ((ic->op == IPUSH) || (ic->op == PCALL))
        {
          MOVB (REG_WITH_INDEX (R1_IDX)->dname);
          R1INB++;
        }
      else if (!_G.r1Pushed)
        {
          emitpush (REG_WITH_INDEX (R1_IDX)->dname);
          _G.r1Pushed++;
        }

      ic->rUsed = bitVectSetBit (ic->rUsed, R1_IDX);
      aop->type = AOP_R1;
      return REG_WITH_INDEX (R1_IDX);
    }

endOfWorld:
  /* I said end of world, but not quite end of world yet */
  /* if this is a result then we can push it on the stack */
  if (result)
    {
      aop->type = AOP_STK;
      return NULL;
    }
  /* in the case that result AND left AND right needs a pointer reg
     we can safely use the result's */
  if (bitVectBitValue (mcs251_rUmaskForOp (IC_RESULT (ic)), R0_IDX) &&
    (!OP_SYMBOL (IC_RESULT (ic)) || OP_SYMBOL (IC_RESULT (ic))->regs[getSize (operandType (IC_RESULT (ic))) - 1]->rIdx == R0_IDX))
    {
      aop->type = AOP_R0;
      return REG_WITH_INDEX (R0_IDX);
    }
  if (bitVectBitValue (mcs251_rUmaskForOp (IC_RESULT (ic)), R1_IDX) &&
    (!OP_SYMBOL (IC_RESULT (ic)) || OP_SYMBOL (IC_RESULT (ic))->regs[getSize (operandType (IC_RESULT (ic))) - 1]->rIdx == R1_IDX))
    {
      aop->type = AOP_R1;
      return REG_WITH_INDEX (R1_IDX);
    }

  /* now this is REALLY the end of the world */
  werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "getFreePtr should never reach here");
  exit (EXIT_FAILURE);
}


/*-----------------------------------------------------------------*/
/* mcs251_getTempRegs - initialize an array of pointers to GPR registers  */
/*               that are not in use. Returns 1 if the requested   */
/*               number of registers were available, 0 otherwise.  */
/*-----------------------------------------------------------------*/
int
mcs251_getTempRegs (reg_info ** tempRegs, int size, iCode * ic)
{
  bitVect *freeRegs;
  int i;
  int offset;

  if (!ic)
    ic = genLine.lineElement.ic;
  if (!ic)
    return 0;
  if (!_G.currentFunc)
    return 0;

  freeRegs = newBitVect (8);
  bitVectSetBit (freeRegs, R2_IDX);
  bitVectSetBit (freeRegs, R3_IDX);
  bitVectSetBit (freeRegs, R4_IDX);
  bitVectSetBit (freeRegs, R5_IDX);
  bitVectSetBit (freeRegs, R6_IDX);
  bitVectSetBit (freeRegs, R7_IDX);

  if (IFFUNC_CALLEESAVES (_G.currentFunc->type))
    {
      bitVect *newfreeRegs;
      newfreeRegs = bitVectIntersect (freeRegs, _G.currentFunc->regsUsed);
      freeBitVect (freeRegs);
      freeRegs = newfreeRegs;
    }
  freeRegs = bitVectCplAnd (freeRegs, ic->rMask);

  offset = 0;
  for (i = 0; i < freeRegs->size; i++)
    {
      if (bitVectBitValue (freeRegs, i))
        tempRegs[offset++] = REG_WITH_INDEX (i);
      if (offset >= size)
        {
          freeBitVect (freeRegs);
          return 1;
        }
    }

  freeBitVect (freeRegs);
  return 0;
}

/*-----------------------------------------------------------------*/
/* aopIsLitVal - asmop from offset is val                          */
/*-----------------------------------------------------------------*/
static bool
aopIsLitVal (const asmop *aop, int offset, int size, unsigned long long int val)
{
  wassert_bt (size <= sizeof (unsigned long long int)); // Make sure we are not testing outside of argument val.

  for(; size; size--, offset++)
    {
      unsigned char b = val & 0xff;
      val >>= 8;

      // Leading zeroes
      if (aop->size <= offset && !b && aop->type != AOP_LIT)
        continue;

      // Information from generalized constant propagation analysis
      if (!aop->valinfo.anything && (offset * 8 < sizeof (aop->valinfo.knownbitsmask) * CHAR_BIT) &&
        ((aop->valinfo.knownbitsmask >> (offset * 8)) & 0xff) == 0xff &&
        ((aop->valinfo.knownbits >> (offset * 8)) & 0xff) == b)
        continue;

      if (aop->type != AOP_LIT)
        return (false);

      if (byteOfVal (aop->aopu.aop_lit, offset) != b)
        return (false);
    }

  return (true);
}

/*-----------------------------------------------------------------*/
/* newAsmop - creates a new asmOp                                  */
/*-----------------------------------------------------------------*/
static asmop *
newAsmop (short type)
{
  asmop *aop;

  aop = Safe_calloc (1, sizeof (asmop));
  aop->type = type;
  aop->aop_litimmd_is_gptr = false;
  aop->aop_lit_is_funcptr = false;
  aop->aop_is_volatile = false;
  aop->aop_is_aggregate = false;
  aop->allocated = 1;
  memset (aop->regs, -1, sizeof(aop->regs));
  aop->valinfo.anything = true;
  return aop;
}

static void
aopSetOperandType (asmop *aop, const operand *op)
{
  aop->aop_is_aggregate = IS_AGGREGATE (operandType (op));
}

/*-----------------------------------------------------------------*/
/* pointerCode - returns the code for a pointer type               */
/*-----------------------------------------------------------------*/
static int
pointerCode (sym_link * etype)
{
  return PTR_TYPE (SPEC_OCLS (etype));
}

/*-----------------------------------------------------------------*/
/* leftRightUseAcc - returns size of accumulator use by operands   */
/*-----------------------------------------------------------------*/
static int
leftRightUseAcc (iCode * ic)
{
  operand *op;
  int size;
  int accuseSize = 0;
  int accuse = 0;

  if (!ic)
    {
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "null iCode pointer");
      return 0;
    }

  if (ic->op == IFX)
    {
      op = IC_COND (ic);
      if (IS_OP_ACCUSE (op))
        {
          accuse = 1;
          size = getSize (OP_SYMBOL (op)->type);
          if (size > accuseSize)
            accuseSize = size;
        }
    }
  else if (ic->op == JUMPTABLE)
    {
      op = IC_JTCOND (ic);
      if (IS_OP_ACCUSE (op))
        {
          accuse = 1;
          size = getSize (OP_SYMBOL (op)->type);
          if (size > accuseSize)
            accuseSize = size;
        }
    }
  else
    {
      op = IC_LEFT (ic);
      if (IS_OP_ACCUSE (op))
        {
          accuse = 1;
          size = getSize (OP_SYMBOL (op)->type);
          if (size > accuseSize)
            accuseSize = size;
        }
      op = IC_RIGHT (ic);
      if (IS_OP_ACCUSE (op))
        {
          accuse = 1;
          size = getSize (OP_SYMBOL (op)->type);
          if (size > accuseSize)
            accuseSize = size;
        }
    }

  if (accuseSize)
    return accuseSize;
  else
    return accuse;
}

/*-----------------------------------------------------------------*/
/* stackoffset - stack offset for symbol                           */
/*-----------------------------------------------------------------*/
static int
stackoffset (symbol * sym)
{
  int offset = sym->stack;
  if (options.omitFramePtr)
    {
      if (SPEC_OCLS (sym->etype)->paged)
        offset -= _G.stack.xoffset + _G.stack.xpushed;
      else
        offset -= _G.stack.offset + _G.stack.pushed;
    }
  if (sym->stack < 0)
    offset -= _G.stack.param_offset;
  return offset;
}

/* Format a MCS251 indexed hardware-stack operand.  The STC32G implementation
   provides 16 KiB of region-00 EDATA, which is fully covered by the signed
   displacement of @DRk+dis16. */
static void
mcs251FormatStackOperand (char *buffer, size_t size, int displacement)
{
  wassert (displacement >= -32768 && displacement <= 32767);

  if (displacement > 0)
    SNPRINTF (buffer, size, "@spx+0x%04x", displacement);
  else if (displacement < 0)
    SNPRINTF (buffer, size, "@spx-0x%04x", -displacement);
  else
    SNPRINTF (buffer, size, "@spx");
}

/* Materialize a stack object's flat address before changing SPX, then place
   its native big-endian bytes on the hardware stack.  Callers can pop the
   bytes into an arbitrary result without DPX aliasing that result's storage. */
static void
mcs251PushStackAddress (symbol *sym, int extraOffset, int size)
{
  int displacement = stackoffset (sym) + extraOffset;

  wassert (sym->onStack);
  emitcode ("mov", "dpx,spx");
  if (displacement > 0)
    emitcode ("add", "dpx,#%d", displacement);
  else if (displacement < 0)
    emitcode ("sub", "dpx,#%d", -displacement);

  for (int offset = size - 1; offset >= 0; --offset)
    {
      if (offset == 0)
        emitpush ("dpl");
      else if (offset == 1)
        emitpush ("dph");
      else if (offset == 2)
        emitpush ("dpxl");
      else
        emitpush (zero);
    }
}

/*-----------------------------------------------------------------*/
/* aopPtrForSym - pointer for symbol                               */
/*-----------------------------------------------------------------*/
static void
aopPtrForSym (symbol * sym, bool accuse, int offset, asmop * aop, iCode * ic)
{
  char *base;
  struct dbuf_s tmpBuf;
  dbuf_init (&tmpBuf, 1024);
  if (sym->onStack)
    {
      dbuf_printf (&tmpBuf, "%s", SYM_BP (sym));
    }
  else
    {
      dbuf_printf (&tmpBuf, "#%s", sym->rname);
    }
  base = dbuf_detach_c_str (&tmpBuf);

  offset += stackoffset (sym);

  if (abs (offset) >= 248)
    werrorfl (ic->filename, ic->lineno, W_LIT_OVERFLOW);

  if ((abs (offset) < 3) || (accuse && (abs (offset) < 4)))
    {
      emitcode ("mov", "%s,%s", aop->aopu.aop_ptr->name, base);
      while (offset < 0)
        {
          emitcode ("dec", aop->aopu.aop_ptr->name);
          offset++;
        }
      while (offset > 0)
        {
          emitcode ("inc", aop->aopu.aop_ptr->name);
          offset--;
        }
    }
  else
    {
      if (accuse)
        {
          emitAccumulatorExchange (aop->aopu.aop_ptr->name);
          emitcode ("mov", "a,%s", base);
          emitcode ("add", "a,#0x%02x", offset & 0xff);
          emitAccumulatorExchange (aop->aopu.aop_ptr->name);
        }
      else
        {
          emitcode ("mov", "a,%s", base);
          emitcode ("add", "a,#0x%02x", offset & 0xff);
          emitcode ("mov", "%s,a", aop->aopu.aop_ptr->name);
        }
    }
  aop->paged = SPEC_OCLS (sym->etype)->paged;
  dbuf_free (base);
}

/*-----------------------------------------------------------------*/
/* aopForSym - for a true symbol                                   */
/*-----------------------------------------------------------------*/
static asmop *
aopForSym (iCode * ic, symbol * sym, bool result)
{
  asmop *aop;
  memmap *space;
  bool accuse = leftRightUseAcc (ic) || _G.accInUse;

  wassertl (ic != NULL, "Got a null iCode");
  wassertl (sym != NULL, "Got a null symbol");

  space = SPEC_OCLS (sym->etype);

  /* if already has one */
  if (sym->aop)
    {
      sym->aop->allocated++;
      return sym->aop;
    }

  /* assign depending on the storage class */
  /* MCS251 automatic objects live on the complete SPX hardware stack.  Do not
     truncate their address through the inherited R0/R1 byte pointers. */
  if (sym->onStack && !space->paged)
    {
      sym->aop = aop = newAsmop (AOP_MCS251_STK);
      aop->aopu.aop_sym = sym;
      aop->size = getSize (sym->type);
      aop->aop_is_volatile = IS_VOLATILE (sym->type);
      return aop;
    }

  /* if it is on the stack or indirectly addressable */
  /* space we need to assign either r0 or r1 to it   */
  if (sym->onStack || sym->iaccess)
    {
      sym->aop = aop = newAsmop (0);
      aop->aopu.aop_ptr = getFreePtr (ic, aop, result);
      aop->size = getSize (sym->type);

      /* now assign the address of the variable to
         the pointer register */
      if (aop->type != AOP_STK)
        {
          aopPtrForSym (sym, accuse, 0, aop, ic);
        }
      else
        {
          aop->aopu.aop_sym = sym;
        }
      aop->aop_is_volatile = IS_VOLATILE (sym->type);
      return aop;
    }

  /* if in bit space */
  if (IN_BITSPACE (space))
    {
      sym->aop = aop = newAsmop (AOP_CRY);
      aop->aopu.aop_dir = sym->rname;
      aop->size = getSize (sym->type);
      aop->aop_is_volatile = IS_VOLATILE (sym->type);
      return aop;
    }
  /* if it is in direct space */
  if (IN_DIRSPACE (space))
    {
      //printf("aopForSym, using AOP_DIR for %s (%x)\n", sym->name, sym);
      //printTypeChainRaw(sym->type, NULL);
      //printf("space = %s\n", space ? space->sname : "NULL");() && (aop->size > 1))
      sym->aop = aop = newAsmop (SPEC_SCLS (getSpec (sym->type)) == S_SFR ? AOP_SFR : AOP_DIR);
      aop->aopu.aop_dir = sym->rname;
      aop->size = getSize (sym->type);
      aop->aop_is_volatile = IS_VOLATILE (sym->type);
      return aop;
    }

  /* special case for a function */
  if (IS_FUNC (sym->type))
    {
      sym->aop = aop = newAsmop (AOP_IMMD);
      aop->aopu.aop_immd.aop_immd1 = Safe_strdup (sym->rname);
      aop->size = getSize (sym->type);
      aop->aop_litimmd_is_gptr = (aop->size == GPTRSIZE && (IS_GENPTR (sym->type) || IFFUNC_ISBANKEDCALL (sym->type)));
      aop->aop_is_volatile = IS_VOLATILE (sym->type);
      return aop;
    }

  /* only remaining is far space */
  /* in which case DPTR gets the address */
  sym->aop = aop = newAsmop (AOP_DPTR);
  aop->aopu.aop_sym = sym;

  /* MCS251 AOP_DPTR reads and writes materialize the complete far address at
     their point of use.  Loading it here can clobber a value still held in
     DPTR, such as a 16-bit function return that is about to be spilled. */
  aop->aop_is_volatile = IS_VOLATILE (sym->type);
  aop->size = getSize (sym->type);

  /* if it is in code space */
  if (IN_CODESPACE (space))
    aop->code = 1;

  return aop;
}

/*-----------------------------------------------------------------*/
/* aopForRemat - rematerializes an object                          */
/*-----------------------------------------------------------------*/
static asmop *
aopForRemat (symbol *sym)
{
  iCode *ic = sym->rematiCode;
  asmop *aop = newAsmop (AOP_IMMD);
  int ptr_type = 0;
  int val = 0;
  sym_link *from_type = NULL;
  const char *from_name = NULL;
  struct dbuf_s dbuf;

  for (;;)
    {
      if (ic->op == '+')
        {
          val += (int) operandLitValue (IC_RIGHT (ic));
          ic = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
        }
      else if (ic->op == '-')
        {
          val -= (int) operandLitValue (IC_RIGHT (ic));
          ic = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
        }
      else if (IS_CAST_ICODE (ic))
        {
          from_type = operandType (IC_RIGHT (ic));
          from_name = IS_SYMOP (IC_RIGHT (ic)) ? OP_SYMBOL (IC_RIGHT (ic))->name : NULL;
          aop->aopu.aop_immd.from_cast_remat = 1;
          ic = OP_SYMBOL (IC_RIGHT (ic))->rematiCode;
        }
      else
        {
          break;
        }
    }

  dbuf_init (&dbuf, 128);
  if (val)
    {
      dbuf_printf (&dbuf, "(%s %c 0x%04x)", OP_SYMBOL (IC_LEFT (ic))->rname, val >= 0 ? '+' : '-', abs (val) & 0xffff);
    }
  else
    {
      dbuf_append_str (&dbuf, OP_SYMBOL (IC_LEFT (ic))->rname);
    }

  aop->aopu.aop_immd.aop_immd1 = dbuf_detach_c_str (&dbuf);
  aop->aopu.aop_immd.mcs251_pdata_page =
    from_type && IS_PTR (from_type) &&
    DCL_TYPE (from_type) == PPOINTER;
  /* set immd2 field if required */
  if (GPTRSIZE > FARPTRSIZE && aop->aopu.aop_immd.from_cast_remat)
    {
      ptr_type = pointerTypeToGPByte (DCL_TYPE (from_type), from_name, sym->name);
      dbuf_init (&dbuf, 128);
      dbuf_tprintf (&dbuf, "#!constbyte", (unsigned)ptr_type);
      aop->aopu.aop_immd.aop_immd2 = dbuf_detach_c_str (&dbuf);
    }

  return aop;
}

/*-----------------------------------------------------------------*/
/* regsInCommon - two operands have some registers in common       */
/*-----------------------------------------------------------------*/
static bool
regsInCommon (operand * op1, operand * op2)
{
  symbol *sym1, *sym2;
  int i;

  /* if they have registers in common */
  if (!IS_SYMOP (op1) || !IS_SYMOP (op2))
    return FALSE;

  sym1 = OP_SYMBOL (op1);
  sym2 = OP_SYMBOL (op2);

  if (sym1->nRegs == 0 || sym2->nRegs == 0)
    return FALSE;

  for (i = 0; i < sym1->nRegs; i++)
    {
      int j;
      if (!sym1->regs[i])
        continue;

      for (j = 0; j < sym2->nRegs; j++)
        {
          if (!sym2->regs[j])
            continue;

          if (sym2->regs[j] == sym1->regs[i])
            return TRUE;
        }
    }

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* operandsEqu - equivalent                                        */
/*-----------------------------------------------------------------*/
static bool
operandsEqu (operand * op1, operand * op2)
{
  symbol *sym1, *sym2;

  /* if they're not symbols */
  if (!IS_SYMOP (op1) || !IS_SYMOP (op2))
    return FALSE;

  sym1 = OP_SYMBOL (op1);
  sym2 = OP_SYMBOL (op2);

  /* if both are itemps & one is spilt
     and the other is not then false */
  if (IS_ITEMP (op1) && IS_ITEMP (op2) && sym1->isspilt != sym2->isspilt)
    return FALSE;

  /* if they are the same */
  if (sym1 == sym2)
    return TRUE;

  /* if they have the same rname */
  if (sym1->rname[0] && sym2->rname[0] && EQ (sym1->rname, sym2->rname) && !(IS_PARM (op2) && IS_ITEMP (op1)))
    return TRUE;

  /* if left is a tmp & right is not */
  if (IS_ITEMP (op1) && !IS_ITEMP (op2) && sym1->isspilt && (sym1->usl.spillLoc == sym2))
    return TRUE;

  if (IS_ITEMP (op2) && !IS_ITEMP (op1) && sym2->isspilt && sym1->level > 0 && (sym2->usl.spillLoc == sym1))
    return TRUE;

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* sameByte - two asmops have the same address at given offsets    */
/*-----------------------------------------------------------------*/
static bool
sameByte (asmop * aop1, int off1, asmop * aop2, int off2)
{
  if (aop1 == aop2 && off1 == off2)
    return TRUE;

  if (aop1->type != AOP_REG && aop1->type != AOP_CRY)
    return FALSE;

  if (aop1->type != aop2->type)
    return FALSE;

  if (aop1->aopu.aop_reg[off1] != aop2->aopu.aop_reg[off2])
    return FALSE;

  return TRUE;
}

/*-----------------------------------------------------------------*/
/* sameRegs - two asmops have the same registers                   */
/*-----------------------------------------------------------------*/
static bool
sameRegs (asmop * aop1, asmop * aop2)
{
  int i;

  if (aop1 == aop2)
    return TRUE;

  if (aop1->type != AOP_REG && aop1->type != AOP_CRY)
    return FALSE;

  if (aop1->type != aop2->type)
    return FALSE;

  if (aop1->size != aop2->size)
    return FALSE;

  for (i = 0; i < aop1->size; i++)
    if (aop1->aopu.aop_reg[i] != aop2->aopu.aop_reg[i])
      return FALSE;

  return TRUE;
}

/*-----------------------------------------------------------------*/
/* opIsGptr: returns non-zero if the passed operand is             */
/* a generic pointer type.                                         */
/*-----------------------------------------------------------------*/
static bool
opIsGptr (const operand *op)
{
  return (op && getSize (operandType (op)) == GPTRSIZE && (IS_GENPTR (operandType (op)) || IFFUNC_ISBANKEDCALL (operandType (op))));
}

/*-----------------------------------------------------------------*/
/* aopOp - allocates an asmop for an operand  :                    */
/*-----------------------------------------------------------------*/
static void
aopOp (operand *op, iCode *ic, bool result)
{
  asmop *aop;
  symbol *sym;
  int i;

  if (!op)
    return;

  /* if this a literal */
  if (IS_OP_LITERAL (op))
    {
      op->aop = aop = newAsmop (AOP_LIT);
      aop->aopu.aop_lit = OP_VALUE (op);
      aop->size = getSize (operandType (op));
      aop->aop_lit_is_funcptr = IS_FUNCPTR (operandType (op));
      aop->aop_litimmd_is_gptr = opIsGptr (op);
      aop->aop_is_volatile = isOperandVolatile (op, false);
      aop->valinfo = getOperandValinfo (ic, op);
      aopSetOperandType (aop, op);
      return;
    }

  /* if already has a asmop then continue */
  if (op->aop)
    {
      op->aop->allocated++;
      aopSetOperandType (op->aop, op);
      return;
    }

  /* if this is a true symbol */
  if (IS_TRUE_SYMOP (op))
    {
      op->aop = aopForSym (ic, OP_SYMBOL (op), result);
      aopSetOperandType (op->aop, op);
      if (!result)
        op->aop->valinfo = getOperandValinfo (ic, op);
      return;
    }

  /* this is a temporary : this has
     only five choices :
     a) register
     b) spillocation
     c) rematerialize
     d) conditional
     e) can be a return use only */

  sym = OP_SYMBOL (op);

  /* if the type is a conditional */
  if (sym->regType == REG_CND)
    {
      sym->aop = op->aop = aop = newAsmop (AOP_CRY);
      aop->aopu.aop_dir = "c";
      aop->size = sym->ruonly ? 1 : 0;
      aopSetOperandType (aop, op);
      return;
    }

  /* if it is spilt then two situations
     a) is rematerialize
     b) has a spill location */
  if (sym->isspilt || sym->nRegs == 0)
    {
      /* rematerialize it NOW */
      if (sym->remat)
        {
          sym->aop = op->aop = aop = aopForRemat (sym);
          aop->size = operandSize (op);
          aop->aop_litimmd_is_gptr = (aop->size == GPTRSIZE && (IS_GENPTR (sym->type) || IFFUNC_ISBANKEDCALL (sym->type)));
          aopSetOperandType (aop, op);
          return;
        }

      if (sym->accuse)
        {
          int i;
          sym->aop = op->aop = aop = newAsmop (AOP_ACC);
          aop->size = getSize (sym->type);
          for (i = 0; i < aop->size; i++)
            {
              aop->aopu.aop_reg[i] = &mcs251_regs[i ? B_IDX : A_IDX];
              aop->regs[i ? B_IDX : A_IDX] = i;
            }
          aopSetOperandType (aop, op);
          return;
        }

      if (sym->ruonly) // Weird: this is used even in functions returning void?
        {
          unsigned i;

          sym->aop = op->aop = aop = newAsmop (AOP_STR);
          aop->size = getSize (sym->type);
          for (i = 0; i < aop->size; i++)
            aop->aopu.aop_str[i] = mcs251_fReturn[i];
          aopSetOperandType (aop, op);
          return;
        }

      if (sym->isspilt && sym->usl.spillLoc)
        {
          asmop *oldAsmOp = NULL;

          if (getSize (sym->type) != getSize (sym->usl.spillLoc->type))
            {
              /* force a new aop if sizes differ */
              oldAsmOp = sym->usl.spillLoc->aop;
              sym->usl.spillLoc->aop = NULL;
            }
          sym->aop = op->aop = aop = aopForSym (ic, sym->usl.spillLoc, result);
          if (!result)
            aop->valinfo = getOperandValinfo (ic, op);
          if (getSize (sym->type) != getSize (sym->usl.spillLoc->type))
            {
              /* Don't reuse the new aop, go with the last one */
              sym->usl.spillLoc->aop = oldAsmOp;
            }
          aop->size = getSize (sym->type);
          aopSetOperandType (aop, op);
          return;
        }

      /* else must be a dummy iTemp */
      sym->aop = op->aop = aop = newAsmop (AOP_DUMMY);
      aop->size = getSize (sym->type);
      aopSetOperandType (aop, op);
      return;
    }

  /* if the type is a bit register */
  if (sym->regType == REG_BIT && sym->regs[0]->type == REG_BIT)
    {
      sym->aop = op->aop = aop = newAsmop (AOP_CRY);
      aop->size = sym->nRegs;   //1???
      aop->aopu.aop_reg[0] = sym->regs[0];
      aop->aopu.aop_dir = sym->regs[0]->name;
      aopSetOperandType (aop, op);
      return;
    }

  /* must be in a register */
  sym->aop = op->aop = aop = newAsmop (AOP_REG);
  aop->size = sym->nRegs;
  for (i = 0; i < sym->nRegs; i++)
    {
      aop->aopu.aop_reg[i] = sym->regs[i];
      aop->regs[sym->regs[i]->rIdx] = i;
    }
  aopSetOperandType (aop, op);
}

/*-----------------------------------------------------------------*/
/* freeAsmop - free up the asmop given to an operand               */
/*-----------------------------------------------------------------*/
static void
freeAsmop (operand * op, asmop * aaop, iCode * ic, bool pop)
{
  asmop *aop;
  int sz;
  symbol *sym;

  if (!op)
    aop = aaop;
  else
    aop = op->aop;

  if (!aop)
    return;

  if (!aop->allocated)
    goto dealloc;

  aop->allocated--;

  /* depending on the asmop type only three cases need work
     AOP_R0, AOP_R1 & AOP_STK */
  switch (aop->type)
    {
    case AOP_R0:
      if (R0INB)
        {
          emitcode ("mov", "r0,b");
          R0INB--;
        }
      else if (_G.r0Pushed)
        {
          if (pop)
            {
              emitpop ("ar0");
              _G.r0Pushed--;
            }
        }
      bitVectUnSetBit (ic->rUsed, R0_IDX);
      break;

    case AOP_R1:emitcode(";", "freeAsmop r1 %d", pop);
      if (R1INB)
        {
          emitcode ("mov", "r1,b");
          R1INB--;
        }
      else if (_G.r1Pushed)
        {
          if (pop)
            {
              emitpop ("ar1");
              _G.r1Pushed--;
            }
        }
      bitVectUnSetBit (ic->rUsed, R1_IDX);
      break;

    case AOP_STK:
      sz = aop->size;
      sym = aop->aopu.aop_sym;
      bitVectUnSetBit (ic->rUsed, R0_IDX);
      bitVectUnSetBit (ic->rUsed, R1_IDX);

      getFreePtr (ic, aop, FALSE);

      aopPtrForSym (sym, FALSE, aop->size - 1, aop, ic);

      while (sz--)
        {
          emitpop ("acc");
          if (aop->paged)
            emitcode ("movx", "@%s,a", aop->aopu.aop_ptr->name);
          else
            emitcode ("mov", "@%s,a", aop->aopu.aop_ptr->name);
          if (!sz)
            break;
          emitcode ("dec", "%s", aop->aopu.aop_ptr->name);
        }
      op->aop = aop;
      freeAsmop (op, NULL, ic, TRUE);
      if (_G.r1Pushed)
        {
          emitpop ("ar1");
          _G.r1Pushed--;
        }
      if (_G.r0Pushed)
        {
          emitpop ("ar0");
          _G.r0Pushed--;
        }
      break;
    }

dealloc:
  /* all other cases just dealloc */
  aop->allocated = 0;
  if (op)
    {
      op->aop = NULL;
      if (IS_SYMOP (op))
        {
          OP_SYMBOL (op)->aop = NULL;
          /* if the symbol has a spill */
          if (SPIL_LOC (op))
            SPIL_LOC (op)->aop = NULL;
        }
    }
}

/*------------------------------------------------------------------*/
/* freeForBranchAsmop - partial free up of Asmop for a branch; just */
/*                      pop r0 or r1 off stack if pushed            */
/*------------------------------------------------------------------*/
static void
freeForBranchAsmop (operand * op, iCode * ic)
{
  asmop *aop;

  if (!op)
    return;

  aop = op->aop;

  if (!aop)
    return;

  if (!aop->allocated)
    return;

  switch (aop->type)
    {
    case AOP_R0:
      if (R0INB)
        {
          emitcode ("mov", "r0,b");
        }
      else if (_G.r0Pushed)
        {
          emitcode ("pop", "ar0");      /* without pushed-- */
        }
      break;

    case AOP_R1:
      if (R1INB)
        {
          emitcode ("mov", "r1,b");
        }
      else if (_G.r1Pushed)
        {
          emitcode ("pop", "ar1");      /* without pushed-- */
        }
      break;

    case AOP_STK:
    {
      int sz = aop->size;

      emitcode ("mov", "b,r0");
      aopPtrForSym (aop->aopu.aop_sym, FALSE, 0, aop, ic);

      while (sz--)
        {
          emitcode ("pop", "acc");    /* without pushed-- */
          if (aop->paged)
            emitcode ("movx", "@r0,a");
          else
            emitcode ("mov", "@r0,a");
          if (!sz)
            break;
          emitcode ("dec", "r0");
        }
      emitcode ("mov", "r0,b");
    }
    }
}

/*------------------------------------------------------------------*/
/* freeForBranchAsmops - partial free up of 3 Asmops for a branch;  */
/*                       just pop r0 or r1 off stack if pushed      */
/*------------------------------------------------------------------*/
static void
freeForBranchAsmops (operand * op1, operand * op2, operand * op3, iCode * ic)
{
  if (op1)
    freeForBranchAsmop (op1, ic);
  if (op2)
    freeForBranchAsmop (op2, ic);
  if (op3)
    freeForBranchAsmop (op3, ic);
}

// Get asmop for registers containing the return type of function
// Returns 0 if the function does not have a return value or it is not returned in registers.
static asmop *
aopRet (sym_link *ftype)
{
  wassert (IS_FUNC (ftype));

  int size = getSize (ftype->next);

  const bool bigreturn = (size > 8) || IS_STRUCT (ftype->next);

  if (bigreturn)
    return (0);

  switch (size)
    {
    case 1:
      return (ASMOP_DPL);
    case 2:
      return (ASMOP_DPTR);
    case 3:
      return (ASMOP_BDPTR);
    case 4:
      return (ASMOP_ABDPTR);
    case 5:
      return (ASMOP_R4ABDPTR);
    case 6:
      return (ASMOP_R5R4ABDPTR);
    case 7:
      return (ASMOP_R6R5R4ABDPTR);
    case 8:
      return (ASMOP_R7R6R5R4ABDPTR);
    default:
      return (0);
    }
}

// Get asmop for registers containing a parameter
// Returns 0 if the parameter is not passed in a register.
static asmop *
aopArg (sym_link *ftype, int i)
{
  wassert (IS_FUNC (ftype));

  if (FUNC_HASVARARGS (ftype))
    return 0;

  // For struct return keep regs free for pushing hidden parameter.
  if (IS_STRUCT (ftype->next))
    return 0;

  value *args = FUNC_ARGS(ftype);
  int j;
  value *arg;

  for (j = 1, arg = args; j < i; j++, arg = arg->next)
    wassert (arg);

  if (IS_SPEC (arg->type) && SPEC_NOUN (arg->type) == V_BIT)
    {
      wassert (0); // todo: special handling for bits.
    }

  // struct arguments are never passed in registers.
  if (IS_STRUCT (arg->type))
    return (0);

  // Only first non-struct, non-bit argument is passed in registers.
  for (value *arg2 = args; arg2 != arg; arg2 = arg2->next)
    if (!IS_STRUCT (arg2->type) && !(IS_SPEC (arg2->type) && SPEC_NOUN (arg2->type) == V_BIT))
      return (0);

  switch (getSize (arg->type))
    {
    case 1:
      return (ASMOP_DPL);
    case 2:
      return (ASMOP_DPTR);
    case 3:
      return (ASMOP_BDPTR);
    case 4:
      return (ASMOP_ABDPTR);
    case 5:
      return (ASMOP_R4ABDPTR);
    case 6:
      return (ASMOP_R5R4ABDPTR);
    case 7:
      return (ASMOP_R6R5R4ABDPTR);
    case 8:
      return (ASMOP_R7R6R5R4ABDPTR);
    default:
      return (0);
    }
}

/*-----------------------------------------------------------------*/
/* swapOperands - swap two operands                                */
/*-----------------------------------------------------------------*/
static void
swapOperands (operand ** left, operand ** right)
{
  operand *t = *right;
  *right = *left;
  *left = t;
}

void
mcs251_setAopR01Offset (asmop *aop, int offset)
{
  while (offset > aop->coff)
    {
      emitcode ("inc", "%s", aop->aopu.aop_ptr->name);
      aop->coff++;
    }

  while (offset < aop->coff)
    {
      emitcode ("dec", "%s", aop->aopu.aop_ptr->name);
      aop->coff--;
    }
}

/*-----------------------------------------------------------------*/
/* aopGetUsesAcc - indicates ahead of time whether aopGet() will   */
/*                 clobber the accumulator                         */
/*-----------------------------------------------------------------*/
static bool
aopGetUsesAcc (const asmop* aop, int offset)
{
  if (offset > (aop->size - 1))
    return FALSE;

  switch (aop->type)
    {

    case AOP_R0:
    case AOP_R1:
      if (aop->paged)
        return TRUE;
      return FALSE;
    case AOP_DPTR:
      return TRUE;
    case AOP_MCS251_STK:
      return TRUE;
    case AOP_IMMD:
      return FALSE;
    case AOP_DIR:
    case AOP_SFR:
      return false;
    case AOP_REG:
      wassert (!EQ (aop->aopu.aop_reg[offset]->name, "a"));
      return FALSE;
    case AOP_CRY:
      return TRUE;
    case AOP_ACC:
      if (offset)
        return FALSE;
      return TRUE;
    case AOP_LIT:
      return FALSE;
    case AOP_STR:
      if (EQ (aop->aopu.aop_str[offset], "a"))
        return TRUE;
      return FALSE;
    case AOP_DUMMY:
      return FALSE;
    default:
      /* Error case --- will have been caught already */
      wassert (0);
      return FALSE;
    }
}

/* Map the generator's least-significant-byte-first offsets to the physical
   byte address of a scalar object in the native big-endian MCS251 ABI. */
static int
mcs251ScalarByteOffset (const asmop *aop, int offset)
{
  if (!aop->aop_is_aggregate && aop->size > 1)
    return aop->size - offset - 1;
  return offset;
}

static int
mcs251PointerByteOffset (sym_link *type, int physicalOffset, int size)
{
  if (size > 1 && !IS_AGGREGATE (type))
    return size - physicalOffset - 1;
  return physicalOffset;
}

/*-------------------------------------------------------------------*/
/* aopGet - for fetching value of the aop                            */
/*-------------------------------------------------------------------*/
/*
 * NOTE: function returns a pointer to a reusable dynamically allocated
 * buffer, which should never be freed!
 * Subsequent call to aopGet() will rewrite the result of the previous
 * call, so the content of the result should be copied to an other
 * location, usually using Safe_strdup(), in order to perserve it.
 */
static const char *
aopGet (asmop *aop, int offset, bool bit16, bool dname)
{
  static struct dbuf_s dbuf = { 0 };

  if (dbuf_is_initialized (&dbuf))
    {
      /* reuse the dynamically allocated buffer */
      dbuf_set_length (&dbuf, 0);
    }
  else
    {
      /* first time: initialize the dynamically allocated buffer */
      dbuf_init (&dbuf, 128);
    }

  /* offset is greater than
     size then zero */
  if (offset > (aop->size - 1) && aop->type != AOP_LIT)
    {
      dbuf_append_str (&dbuf, zero);
    }
  else
    {
      /* depending on type */
      switch (aop->type)
        {
        case AOP_DUMMY:
          dbuf_append_str (&dbuf, zero);
          break;

        case AOP_R0:
        case AOP_R1:
          mcs251_setAopR01Offset (aop, offset);

          if (aop->paged)
            {
              emitcode ("movx", "a,@%s", aop->aopu.aop_ptr->name);
              dbuf_append_str (&dbuf, dname ? "acc" : "a");
            }
          else
            {
              dbuf_printf (&dbuf, "@%s", aop->aopu.aop_ptr->name);
            }
          break;

        case AOP_DPTR:
          {
            int physicalOffset = mcs251ScalarByteOffset (aop, offset);

          if (aop->aopu.aop_sym)
            {
              mcs251LoadFarSymbol (aop, physicalOffset);
              loadFarPointerByte (aop->code);
            }
          else
            {
              while (physicalOffset > aop->coff)
                {
                  incrementFarPointer ();
                  aop->coff++;
                }

              while (physicalOffset < aop->coff)
                {
                  decrementFarPointer ();
                  aop->coff--;
                }

              aop->coff = physicalOffset;
              loadFarPointerByte (aop->code);
            }
          dbuf_append_str (&dbuf, dname ? "acc" : "a");
          }
          break;

        case AOP_MCS251_STK:
          {
            char stackOperand[32];
            int physicalOffset = mcs251ScalarByteOffset (aop, offset);

            mcs251FormatStackOperand (stackOperand, sizeof (stackOperand),
                                    stackoffset (aop->aopu.aop_sym) +
                                    physicalOffset);
            emitcode ("mov", "a,%s", stackOperand);
            dbuf_append_str (&dbuf, dname ? "acc" : "a");
          }
          break;

        case AOP_IMMD:
          if (aop->aopu.aop_immd.mcs251_pdata_page && offset == 1)
            {
              dbuf_append_str (&dbuf, "P2");
            }
          else if (aop->aopu.aop_immd.mcs251_pdata_page && offset == 2)
            {
              dbuf_append_str (&dbuf, "0xeb"); /* STC MXAX */
            }
          else if (GPTRSIZE > FARPTRSIZE && aop->aopu.aop_immd.from_cast_remat && aop->aop_litimmd_is_gptr && offset == GPTRSIZE - 1)
            {
              dbuf_printf (&dbuf, "%s", aop->aopu.aop_immd.aop_immd2);
            }
          else if (bit16)
            {
              dbuf_printf (&dbuf, "#%s", aop->aopu.aop_immd.aop_immd1);
            }
          else if (offset)
            {
              dbuf_printf (&dbuf, "#(%s >> %d)", aop->aopu.aop_immd.aop_immd1, offset * 8);
            }
          else
            {
              dbuf_printf (&dbuf, "#%s", aop->aopu.aop_immd.aop_immd1);
            }
          break;

        case AOP_DIR:
        case AOP_SFR:
          {
            int physicalOffset = aop->type == AOP_DIR ?
              mcs251ScalarByteOffset (aop, offset) : offset;

            if (aop->type == AOP_SFR && aop->size > 1)
              {
                dbuf_printf (&dbuf, "((%s >> %d) & 0xFF)", aop->aopu.aop_dir, offset * 8);
              }
            else if (physicalOffset)
              {
                dbuf_printf (&dbuf, "(%s + %d)", aop->aopu.aop_dir, physicalOffset);
              }
            else
              {
                dbuf_printf (&dbuf, "%s", aop->aopu.aop_dir);
              }
          }
          break;

        case AOP_ACC:
        case AOP_REG:
          dbuf_append_str (&dbuf, dname ? aop->aopu.aop_reg[offset]->dname : aop->aopu.aop_reg[offset]->name);
          break;

        case AOP_CRY:
          if (!EQ (aop->aopu.aop_dir, "c"))
            emitcode ("mov", "c,%s", aop->aopu.aop_dir);
          emitcode ("clr", "a");
          emitcode ("rlc", "a");
          dbuf_append_str (&dbuf, dname ? "acc" : "a");
          break;

        case AOP_LIT:
          if (GPTRSIZE > FARPTRSIZE && aop->aop_litimmd_is_gptr && aop->aop_lit_is_funcptr && offset == GPTRSIZE - 1)
            {
              dbuf_append_str (&dbuf, aopLiteralGptr (NULL, aop->aopu.aop_lit));
            }
          else
            {
              int size = 1 + (bit16 ? 1 : 0);
              dbuf_append_str (&dbuf, aopLiteralLong (aop->aopu.aop_lit, offset, size));
            }
          break;

        case AOP_STR:
          aop->coff = offset;
          if (EQ (aop->aopu.aop_str[offset], "a") && dname)
            dbuf_append_str (&dbuf, "acc");
          else
            dbuf_append_str (&dbuf, aop->aopu.aop_str[offset]);
          break;

        default:
          dbuf_destroy (&dbuf);
          werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "aopget got unsupported aop->type");
          exit (EXIT_FAILURE);
        }
    }
  return dbuf_c_str (&dbuf);
}

static const char *
opGet (operand *op, int offset, bool bit16, bool dname)
{
  return (aopGet (op->aop, offset, bit16, dname));
}

/*-----------------------------------------------------------------*/
/* aopPutUsesAcc - indicates ahead of time whether aopPut() will   */
/*                 clobber the accumulator                         */
/*-----------------------------------------------------------------*/
static bool
aopPutUsesAcc (const asmop* aop, const char *s, int offset)
{
  if (offset > (aop->size - 1))
    return FALSE;

  switch (aop->type)
    {
    case AOP_DUMMY:
      return TRUE;
    case AOP_DIR:
    case AOP_SFR:
      return false;
    case AOP_REG:
      wassert (!EQ (aop->aopu.aop_reg[offset]->name, "a"));
      return FALSE;
    case AOP_DPTR:
      return TRUE;
    case AOP_MCS251_STK:
      return TRUE;
    case AOP_R0:
    case AOP_R1:
      return ((aop->paged) || (*s == '@'));
    case AOP_STK:
      return (*s == '@');
    case AOP_CRY:
      return (!aop->aopu.aop_dir || !EQ (s, aop->aopu.aop_dir));
    case AOP_STR:
      return FALSE;
    case AOP_IMMD:
      return FALSE;
    case AOP_ACC:
      return FALSE;
    default:
      /* Error case --- will have been caught already */
      wassert (0);
      return FALSE;
    }
}

/*-----------------------------------------------------------------*/
/* aopPut- puts a string for a aop and indicates if acc is in use */
/*-----------------------------------------------------------------*/
static bool
aopPut (asmop *aop, const char *s, int offset)
{
  bool accuse = false;
  static struct dbuf_s dbuf = { 0 };

  if (dbuf_is_initialized (&dbuf))
    {
      /* reuse the dynamically allocated buffer */
      dbuf_set_length (&dbuf, 0);
    }
  else
    {
      /* first time: initialize the dynamically allocated buffer */
      dbuf_init (&dbuf, 128);
    }

  if (aop->size && offset > (aop->size - 1))
    {
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "opPutgot offset > aop->size");
      exit (EXIT_FAILURE);
    }

  /* will assign value to value */
  /* depending on where it is ofcourse */
  switch (aop->type)
    {
    case AOP_DUMMY:
      MOVA (s);                 /* read s in case it was volatile */
      accuse = TRUE;
      break;

    case AOP_DIR:
    case AOP_SFR:
      {
        int physicalOffset = aop->type == AOP_DIR ?
          mcs251ScalarByteOffset (aop, offset) : offset;

        if (aop->type == AOP_SFR && aop->size > 1)
          {
            dbuf_printf (&dbuf, "((%s >> %d) & 0xFF)", aop->aopu.aop_dir, offset * 8);
          }
        else if (physicalOffset)
          {
            dbuf_printf (&dbuf, "(%s + %d)", aop->aopu.aop_dir, physicalOffset);
          }
        else
          {
            dbuf_append_str (&dbuf, aop->aopu.aop_dir);
          }

        if (!EQ (dbuf_c_str (&dbuf), s) || aop->aop_is_volatile)
          {
            emitcode ("mov", "%s,%s", dbuf_c_str (&dbuf), s);
          }
        if (EQ (dbuf_c_str (&dbuf), "acc"))
          {
            accuse = TRUE;
          }
      }
      break;

    case AOP_ACC:
      accuse = TRUE;
      aop->coff = offset;
      if (!offset && EQ (s, "acc") && !aop->aop_is_volatile)
        break;

    case AOP_REG:
      if (!EQ (aop->aopu.aop_reg[offset]->name, s) && !EQ (aop->aopu.aop_reg[offset]->dname, s))
        {
          if (*s == '@' ||
              EQ (s, "r0") || EQ (s, "r1") || EQ (s, "r2") || EQ (s, "r3") ||
              EQ (s, "r4") || EQ (s, "r5") || EQ (s, "r6") || EQ (s, "r7"))
            {
              emitcode ("mov", "%s,%s", aop->aopu.aop_reg[offset]->dname, s);
            }
          else
            {
              emitcode ("mov", "%s,%s", aop->aopu.aop_reg[offset]->name, s);
            }
        }
      break;

    case AOP_DPTR:
      {
      int physicalOffset = mcs251ScalarByteOffset (aop, offset);

      if (aop->code)
        {
          werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "opPutwriting to code space");
          exit (EXIT_FAILURE);
        }

      /* if not in accumulator */
      MOVA (s);

      if (aop->aopu.aop_sym)
        {
          mcs251LoadFarSymbol (aop, physicalOffset);
          storeFarPointerByte ();
          break;
        }

      while (physicalOffset > aop->coff)
        {
          aop->coff++;
          incrementFarPointer ();
        }

      while (physicalOffset < aop->coff)
        {
          aop->coff--;
          decrementFarPointer ();
        }

      aop->coff = physicalOffset;

      storeFarPointerByte ();
      }
      break;

    case AOP_MCS251_STK:
      {
        char stackOperand[32];
        int physicalOffset = mcs251ScalarByteOffset (aop, offset);

        MOVA (s);
        mcs251FormatStackOperand (stackOperand, sizeof (stackOperand),
                                stackoffset (aop->aopu.aop_sym) +
                                physicalOffset);
        emitcode ("mov", "%s,a", stackOperand);
        accuse = TRUE;
      }
      break;

    case AOP_R0:
    case AOP_R1:
      /* Get source in A before inc/dec of r0/r1 */
      /* in case source needed different offset  */
      if (aop->paged || *s == '@')
        {
          MOVA (s);
        }

      mcs251_setAopR01Offset (aop, offset);

      if (aop->paged)
        {
          /* source already in A */
          emitcode ("movx", "@%s,a", aop->aopu.aop_ptr->name);
        }
      else if (*s == '@')
        {
          /* source already in A */
          emitcode ("mov", "@%s,a", aop->aopu.aop_ptr->name);
        }
      else if (EQ (s, "r0") || EQ (s, "r1") || EQ (s, "r2") || EQ (s, "r3") ||
               EQ (s, "r4") || EQ (s, "r5") || EQ (s, "r6") || EQ (s, "r7"))
        {
          dbuf_printf (&dbuf, "a%s", s);
          emitcode ("mov", "@%s,%s", aop->aopu.aop_ptr->name, dbuf_c_str (&dbuf));
        }
      else
        {
          emitcode ("mov", "@%s,%s", aop->aopu.aop_ptr->name, s);
        }
      break;

    case AOP_STK:
      emitpush (s);
      break;

    case AOP_CRY:
      // source is no literal and not in carry
      if (!EQ (s, zero) && !EQ (s, one) && !EQ (s, "c"))
        {
          MOVA (s);
          /* set C, if a >= 1 */
          emitcode ("add", "a,#!constbyte", 0xffu);
          s = "c";
        }
      // now source is zero, one or carry

      /* if result no bit variable */
      if (!aop->aopu.aop_dir)
        {
          if (EQ (s, "c"))
            {
              /* inefficient: move carry into A and use jz/jnz */
              emitcode ("clr", "a");
              emitcode ("rlc", "a");
              accuse = TRUE;
            }
          else
            {
              MOVA (s);
              accuse = TRUE;
            }
        }
      else if (EQ (s, zero))
        emitcode ("clr", "%s", aop->aopu.aop_dir);
      else if (EQ (s, one))
        emitcode ("setb", "%s", aop->aopu.aop_dir);
      else if (!EQ (s, aop->aopu.aop_dir))
        emitcode ("mov", "%s,c", aop->aopu.aop_dir);
      break;

    case AOP_STR:
      aop->coff = offset;
      if (!EQ (aop->aopu.aop_str[offset], s) || aop->aop_is_volatile)
        emitcode ("mov", "%s,%s", aop->aopu.aop_str[offset], s);
      break;

    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "opPutgot unsupported aop->type");
      exit (EXIT_FAILURE);
    }

  return accuse;
}

static bool
opPut (operand *result, const char *s, int offset)
{
  wassert (isOperandVolatile (result, FALSE) == result->aop->aop_is_volatile);

  return (aopPut (result->aop, s, offset));
}

/*--------------------------------------------------------------------*/
/* loadDptrFromOperand - load dptr (and optionally B) from operand op */
/*--------------------------------------------------------------------*/
static void
loadDptrFromOperand (operand *op, bool loadBToo)
{
  if (AOP_SIZE (op) >= 3)
    {
      switch (AOP_TYPE (op))
        {
        case AOP_STR:
          /* DPL/DPH are already in place for register arguments. */
          emitcode ("mov", "dpxl,%s", opGet (op, 2, FALSE, FALSE));
          return;

        case AOP_IMMD:
        case AOP_LIT:
          if (AOP_TYPE (op) == AOP_IMMD &&
              AOP (op)->aopu.aop_immd.mcs251_pdata_page)
            {
              emitcode ("mov", "dpl,%s", opGet (op, 0, FALSE, FALSE));
              emitcode ("mov", "dph,%s", opGet (op, 1, FALSE, FALSE));
              emitcode ("mov", "dpxl,%s", opGet (op, 2, FALSE, FALSE));
            }
          else
            {
              /* Check the DPTR cache: if dptr already points to the
                 same far symbol+offset, skip the reload.
                 opGet returns a pointer to a static buffer, so we must
                 copy the strings before the next opGet call. */
              const char *dptr_name = opGet (op, 0, TRUE, FALSE);
              char dptr_buf[64] = {0};
              if (dptr_name && dptr_name[0])
                strncpy (dptr_buf, dptr_name, sizeof(dptr_buf) - 1);
              const char *dpxl_name = opGet (op, 2, FALSE, FALSE);
              char dpxl_buf[64] = {0};
              if (dpxl_name && dpxl_name[0])
                strncpy (dpxl_buf, dpxl_name, sizeof(dpxl_buf) - 1);

              if (_G.dptrCache.valid &&
                  dptr_buf[0] && dpxl_buf[0] &&
                  !strcmp (dptr_buf, _G.dptrCache.symbol) &&
                  !strcmp (dpxl_buf, _G.dptrCache.dpxlSymbol))
                {
                  /* dptr and dpxl already hold the right values.
                     Nothing to emit. */
                }
              else if (_G.dptrCache.valid &&
                  dptr_buf[0] &&
                  !strcmp (dptr_buf, _G.dptrCache.symbol))
                {
                  /* dptr already holds the right low 16 bits.
                     Only dpxl may need updating. */
                  emitcode ("mov", "dpxl,%s", dpxl_buf);
                  strncpy (_G.dptrCache.dpxlSymbol, dpxl_buf,
                           sizeof(_G.dptrCache.dpxlSymbol) - 1);
                }
              else if (_G.dptrCache.valid &&
                  dptr_buf[0])
                {                  /* Try inc dpx for sequential addresses of the same base.
                     Parse the symbol and offset from both the cached and
                     new dptr strings.  Format: "NAME" or "(NAME + 0xOFF)". */
                  char cached_base[48] = {0};
                  unsigned long cached_off = 0;
                  char new_base[48] = {0};
                  unsigned long new_off = 0;
                  bool cached_parsed = false;
                  bool new_parsed = false;

                  if (sscanf (_G.dptrCache.symbol, "(%47[^+ ] + 0x%lx)", cached_base, &cached_off) == 2)
                    cached_parsed = true;
                  else if (sscanf (_G.dptrCache.symbol, "%47s", cached_base) == 1) {
                    cached_off = 0;
                    cached_parsed = true;
                  }

                  if (sscanf (dptr_buf, "(%47[^+ ] + 0x%lx)", new_base, &new_off) == 2)
                    new_parsed = true;
                  else if (sscanf (dptr_buf, "%47s", new_base) == 1) {
                    new_off = 0;
                    new_parsed = true;
                  }

                  /* Fallback: manual parse if sscanf failed */
                  if (!cached_parsed) {
                    const char *s = _G.dptrCache.symbol;
                    if (s[0] == '#') s++;  /* skip immediate prefix */
                    const char *plus = strchr (s, '+');
                    if (plus) {
                      size_t len = plus - s;
                      while (len > 0 && s[len-1] == ' ') len--;
                      if (len > 1 && s[0] == '(') {
                        memcpy (cached_base, s + 1, len - 1);
                        cached_base[len-1] = '\0';
                        cached_off = strtoul (plus + 1, NULL, 16);
                        cached_parsed = true;
                      }
                    } else {
                      strncpy (cached_base, s, sizeof(cached_base) - 1);
                      cached_off = 0;
                      cached_parsed = true;
                    }
                  }
                  if (!new_parsed) {
                    const char *s = dptr_buf;
                    if (s[0] == '#') s++;  /* skip immediate prefix */
                    const char *plus = strchr (s, '+');
                    if (plus) {
                      size_t len = plus - s;
                      while (len > 0 && s[len-1] == ' ') len--;
                      if (len > 1 && s[0] == '(') {
                        memcpy (new_base, s + 1, len - 1);
                        new_base[len-1] = '\0';
                        new_off = strtoul (plus + 1, NULL, 16);
                        new_parsed = true;
                      }
                    } else {
                      strncpy (new_base, s, sizeof(new_base) - 1);
                      new_off = 0;
                      new_parsed = true;
                    }
                  }

                  if (cached_parsed && new_parsed &&
                      !strcmp (cached_base, new_base) &&
                      new_off > cached_off && new_off <= cached_off + 4)
                    {
                      /* Same base, sequential small forward offset.
                         Use inc dpx instead of reloading. */
                      unsigned long steps = new_off - cached_off;                      D (emitcode (";", "DPTR cache: inc dpx by %lu", steps));
                      while (steps--)
                        emitcode ("inc", "dpx");
                      /* Only dpxl might need update after inc */
                      if (strcmp (dpxl_buf, _G.dptrCache.dpxlSymbol))
                        emitcode ("mov", "dpxl,%s", dpxl_buf);
                      /* Update cache to new position */
                      strncpy (_G.dptrCache.symbol, dptr_buf,
                               sizeof(_G.dptrCache.symbol) - 1);
                      strncpy (_G.dptrCache.dpxlSymbol, dpxl_buf,
                               sizeof(_G.dptrCache.dpxlSymbol) - 1);
                    }
                  else
                    {
                      if (cached_parsed && new_parsed)
                        fprintf (stderr, "DPTR no-match: cached=(%s,0x%lx) new=(%s,0x%lx)\n",
                                 cached_base, cached_off, new_base, new_off);
                      emitcode ("mov", "dptr,%s", dptr_buf);
                      emitcode ("mov", "dpxl,%s", dpxl_buf);
                      /* Update the cache. */
                      strncpy (_G.dptrCache.symbol, dptr_buf,
                               sizeof(_G.dptrCache.symbol) - 1);
                      strncpy (_G.dptrCache.dpxlSymbol, dpxl_buf,
                               sizeof(_G.dptrCache.dpxlSymbol) - 1);
                      _G.dptrCache.valid = true;
                    }
                }
              else
                {
                  /* Cache not valid or no match: full reload. */
                  emitcode ("mov", "dptr,%s", dptr_buf);
                  emitcode ("mov", "dpxl,%s", dpxl_buf);
                  strncpy (_G.dptrCache.symbol, dptr_buf,
                           sizeof(_G.dptrCache.symbol) - 1);
                  strncpy (_G.dptrCache.dpxlSymbol, dpxl_buf,
                           sizeof(_G.dptrCache.dpxlSymbol) - 1);
                  _G.dptrCache.valid = true;
                }
            }
          return;

        case AOP_DPTR:
          /* The address of the pointer object itself occupies DPX.  Read
             all bytes before replacing that register with the pointee. */
          emitpush (opGet (op, 0, FALSE, FALSE));
          emitpush (opGet (op, 1, FALSE, FALSE));
          emitpush (opGet (op, 2, FALSE, FALSE));
          emitpop ("dpxl");
          emitpop ("dph");
          emitpop ("dpl");
          return;

        default:
          emitcode ("mov", "dpl,%s", opGet (op, 0, FALSE, FALSE));
          emitcode ("mov", "dph,%s", opGet (op, 1, FALSE, FALSE));
          emitcode ("mov", "dpxl,%s", opGet (op, 2, FALSE, FALSE));
          return;
        }
    }

  if (AOP_TYPE (op) != AOP_STR)
    {
      /* if this is rematerializable */
      if (AOP_TYPE (op) == AOP_IMMD)
        {
          emitcode ("mov", "dptr,%s", opGet (op, 0, TRUE, FALSE));
          if (loadBToo)
            {
              if (AOP (op)->aopu.aop_immd.from_cast_remat)
                emitcode ("mov", "b,%s", opGet (op, AOP_SIZE (op) - 1, FALSE, FALSE));
              else
                {
                  wassertl (FALSE, "need pointerCode");
                  emitcode (";", "mov b,???");
                  /* genPointerGet and genPointerSet originally did different
                   ** things for this case. Both seem wrong.
                   ** from genPointerGet:
                   **  emitcode ("mov", "b,#%d", pointerCode (retype));
                   ** from genPointerSet:
                   **  emitcode ("mov", "b,%s + 1", opGet (result, 0, TRUE, FALSE));
                   */
                }
            }
        }
      else if (AOP_TYPE (op) == AOP_LIT)
        {
          emitcode ("mov", "dptr,%s", opGet (op, 0, TRUE, FALSE));
          if (loadBToo)
            emitcode ("mov", "b,%s", opGet (op, AOP_SIZE (op) - 1, FALSE, FALSE));
        }
      else if (AOP_TYPE (op) == AOP_DPTR)
        {
          emitpush (opGet (op, 0, FALSE, FALSE));
          if (loadBToo)
            {
              emitpush (opGet (op, 1, FALSE, FALSE));
              emitcode ("mov", "b,%s", opGet (op, AOP_SIZE (op) - 1, FALSE, FALSE));
              emitpop ("dph");
            }
          else
            {
              emitcode ("mov", "dph,%s", opGet (op, 1, FALSE, FALSE));
            }
          emitpop ("dpl");
        }
      else
        {
          /* we need to get it byte by byte */
          emitcode ("mov", "dpl,%s", opGet (op, 0, FALSE, FALSE));
          emitcode ("mov", "dph,%s", opGet (op, 1, FALSE, FALSE));
          if (loadBToo)
            emitcode ("mov", "b,%s", opGet (op, AOP_SIZE (op) - 1, FALSE, FALSE));
        }
    }
}

/*-----------------------------------------------------------------*/
/* reAdjustPreg - points a register back to where it should        */
/*-----------------------------------------------------------------*/
static void
reAdjustPreg (asmop * aop)
{
  if ((aop->coff == 0) || (aop->size <= 1))
    return;

  switch (aop->type)
    {
    case AOP_R0:
    case AOP_R1:
      while (aop->coff--)
        emitcode ("dec", "%s", aop->aopu.aop_ptr->name);
      break;
    case AOP_DPTR:
      while (aop->coff--)
        decrementFarPointer ();
      break;
    }
  aop->coff = 0;
}

/*-----------------------------------------------------------------*/
/* getDataSize - get the operand data size                         */
/*-----------------------------------------------------------------*/
static int
getDataSize (operand * op)
{
  int size = AOP_SIZE (op);

  if (size == GPTRSIZE)
    {
      sym_link *type = operandType (op);
      if (IS_GENPTR (type) && GPTRSIZE > FARPTRSIZE)
        {
          /* generic pointer; arithmetic operations
           * should ignore the high byte (pointer type).
           */
          size--;
        }
    }
  return size;
}

/*-----------------------------------------------------------------*/
/* aopDir - aop at offset can be addressed directly                */
/*-----------------------------------------------------------------*/
static bool
aopDir (const asmop *aop, int offset)
{
  if (offset >= aop->size || offset < 0)
    return (false);

  return (aop->type == AOP_DIR || aop->type == AOP_SFR || aop->type == AOP_REG || aop->type == AOP_ACC);
}

/*-----------------------------------------------------------------*/
/* aopInReg - asmop from offset in the register                    */
/*-----------------------------------------------------------------*/
static bool
aopInReg (const asmop *aop, int offset, short rIdx)
{
  if (offset >= aop->size || offset < 0)
    return (false);

  if (aop->type == AOP_ACC)
    return (offset ? (rIdx == B_IDX) : (rIdx == A_IDX));

  if (aop->type != AOP_REG)
    return (false);

  // todo enable when we have DPTR_IDX
  //if (rIdx == DPTR_IDX)
  //  return (aopInReg (aop, offset, DPL_IDX) && aopInReg (aop, offset + 1, DPH_IDX));

  return (aop->aopu.aop_reg[offset]->rIdx == rIdx);
}

/*-----------------------------------------------------------------*/
/* aopInRn - Check if byte of asmop is in any of R0, ..., R7.      */
/*-----------------------------------------------------------------*/
static bool aopInRn (const asmop *aop, int i)
{
  if (aop->type != AOP_REG  || i >= aop->size)
    return (false);
  int rIdx = aop->aopu.aop_reg[i]->rIdx;
  return (rIdx == R0_IDX || rIdx == R1_IDX || rIdx == R2_IDX || rIdx == R3_IDX || rIdx == R4_IDX || rIdx == R5_IDX || rIdx == R6_IDX || rIdx == R7_IDX);
}

/* CJNE retains the MCS-51 R0-R7 restriction in MCS-251 Source mode. */
static const char *
prepareCjneLeftOperand (const char *operand)
{
  int index = mcs251_regname_to_idx (operand);

  if (index >= R15_IDX && index <= R8_IDX)
    {
      MOVA (operand);
      return "a";
    }

  return operand;
}

static bool
isFixedByteRegisterOperand (const char *operand)
{
  int index = mcs251_regname_to_idx (operand);

  return index >= R15_IDX && index <= R8_IDX;
}

static void
emitCjneTlabel (const char *left, const char *right, int label)
{
  if (isFixedByteRegisterOperand (right))
    {
      emitcode ("cmp", "%s,%s", left, right);
      emitcode ("jne", "!tlabel", label);
    }
  else
    emitcode ("cjne", "%s,%s,!tlabel",
              prepareCjneLeftOperand (left), right, label);
}

static void
emitCjneNamedLabel (const char *left, const char *right, int label)
{
  if (isFixedByteRegisterOperand (right))
    {
      emitcode ("cmp", "%s,%s", left, right);
      emitcode ("jne", "%05d$", label);
    }
  else
    emitcode ("cjne", "%s,%s,%05d$",
              prepareCjneLeftOperand (left), right, label);
}

/*-----------------------------------------------------------------*/
/* outAcc - output Acc                                             */
/*-----------------------------------------------------------------*/
static void
outAcc (operand * result)
{
  int size, offset;
  size = getDataSize (result);
  if (size)
    {
      opPut (result, "a", 0);
      size--;
      offset = 1;
      /* unsigned or positive */
      while (size--)
        {
          opPut (result, zero, offset++);
        }
    }
}

/*-----------------------------------------------------------------*/
/* outBitC - output a bit C                                        */
/*-----------------------------------------------------------------*/
static void
outBitC (operand * result)
{
  /* if the result is bit */
  if (AOP_TYPE (result) == AOP_CRY)
    {
      opPut (result, "c", 0);
    }
  else if (AOP_TYPE (result) != AOP_DUMMY)
    {
      emitcode ("clr", "a");
      emitcode ("rlc", "a");
      outAcc (result);
    }
}

/*-----------------------------------------------------------------*/
/* toBoolean - emit code for orl a,operator(sizeop)                */
/*-----------------------------------------------------------------*/
static void
toBoolean (operand * oper)
{
  int size = AOP_SIZE (oper) - 1;
  int offset = 1;
  bool AccUsed;
  sym_link *type = operandType (oper);
  bool pushedB;

  /* always need B for float */
  AccUsed = IS_FLOAT (type);

  while (!AccUsed && size--)
    {
      AccUsed |= aopGetUsesAcc (oper->aop, offset++);
    }

  if (opIsGptr (oper))
    {
      /* assumes that banks never map to address 0x0000
         so it suffices to check dptr part only and ignore b */
      size = AOP_SIZE (oper) - 2;
    }
  else
    {
      size = AOP_SIZE (oper) - 1;
    }

  offset = 0;
  if (size && AccUsed && (AOP (oper)->type != AOP_ACC))
    {
      pushedB = pushB ();
      MOVB (opGet (oper, offset++, FALSE, FALSE));
      while (--size)
        {
          MOVA (opGet (oper, offset++, FALSE, FALSE));
          emitcode ("orl", "b,a");
        }
      MOVA (opGet (oper, offset++, FALSE, FALSE));
      if (IS_FLOAT (type))
        emitcode ("anl", "a,#0x7F");    //clear sign bit
      emitcode ("orl", "a,b");
      popB (pushedB);
    }
  else
    {
      MOVA (opGet (oper, offset++, FALSE, FALSE));
      while (size--)
        {
          emitcode ("orl", "a,%s", aopGet (oper->aop, offset, false, !aopInRn (oper->aop, offset)));
          offset++;
        }
    }
}

/*-----------------------------------------------------------------*/
/* toCarry - make boolean and move into carry                      */
/*-----------------------------------------------------------------*/
static void
toCarry (operand *oper)
{
  /* if the operand is a literal then
     we know what the value is */
  if (AOP_TYPE (oper) == AOP_LIT)
    {
      if ((int) operandLitValue (oper))
        SETC;
      else
        CLRC;
    }
  else if (AOP_TYPE (oper) == AOP_CRY)
    {
      if (!IS_OP_ACCUSE (oper))
        emitcode ("mov", "c,%s", oper->aop->aopu.aop_dir);
    }
  else if (IS_BOOL (operandType (oper)) || IS_BITFIELD (operandType (oper)) && SPEC_BLEN (getSpec (operandType (oper))) == 1)
    {
      MOVA (opGet (oper, 0, FALSE, FALSE));
      emitcode ("rrc", "a");
    }
  else
    {
      /* or the operand into a */
      toBoolean (oper);
      /* set C, if a >= 1 */
      emitcode ("add", "a,#0xff");
    }
}

/*-----------------------------------------------------------------*/
/* assignBit - assign operand to bit operand                       */
/*-----------------------------------------------------------------*/
static void
assignBit (operand * result, operand * right)
{
  emitcode (";", "assignBit");
  /* if the right side is a literal then
     we know what the value is */
  if (AOP_TYPE (right) == AOP_LIT)
    {
      if ((int) operandLitValue (right))
        opPut (result, one, 0);
      else
        opPut (result, zero, 0);
    }
  else
    {
      toCarry (right);
      outBitC (result);
    }
}

/*-------------------------------------------------------------------*/
/* xch_a_aopGet - for exchanging acc with value of the aop           */
/*-------------------------------------------------------------------*/
static const char *
xch_a_aopGet (operand * oper, int offset, bool dname)
{
  const char *l;

  if (aopGetUsesAcc (oper->aop, offset))
    {
      emitcode ("mov", "b,a");
      MOVA (opGet (oper, offset, false, dname));
      emitcode ("xch", "a,b");
      opPut (oper, "a", offset);
      emitcode ("xch", "a,b");
      l = "b";
    }
  else
    {
      l = opGet (oper, offset, false, dname);
      emitAccumulatorExchange (l);
    }
  return l;
}

/*-----------------------------------------------------------------*/
/* cheapMove - Copy a single byte from one asmop to another.       */
/*-----------------------------------------------------------------*/
static void
cheapMove (asmop *to, int to_offset, asmop *from, int from_offset, bool a_dead)
{
  if (aopInReg (to, to_offset, A_IDX))
    a_dead = true;

  if ((to->type == AOP_REG || to->type == AOP_ACC) && (from->type == AOP_REG || from->type == AOP_ACC) &&
    to->aopu.aop_reg[to_offset] == from->aopu.aop_reg[from_offset])
    return;

  if (to->type == AOP_STR && from->type == AOP_REG &&
    EQ (to->aopu.aop_str[to_offset], from->aopu.aop_reg[from_offset]->name))
    return;

  if (aopInReg (to, to_offset, A_IDX) && aopIsLitVal (from, from_offset, 1, 0))
    {
      emitcode ("clr", "a");
    }
  else if (aopInReg (to, to_offset, A_IDX) && aopInRn (from, from_offset))
    {
      emitcode ("mov", "a, %s", from->aopu.aop_reg[from_offset]->name);
    }
  else if (aopInReg (to, to_offset, A_IDX) && aopDir (from, from_offset))
    {
      emitcode ("mov", "a, %s", aopGet (from, from_offset, false, true));
    }
  else if (aopInReg (from, from_offset, A_IDX) && (aopInRn (to, to_offset) || aopInReg (to, to_offset, B_IDX) || aopInReg (to, to_offset, DPL_IDX) || aopInReg (to, to_offset, DPH_IDX)))
    {
      emitcode ("mov", "%s, a", to->aopu.aop_reg[to_offset]->name);
    }
  else if (aopInRn (to, to_offset) && (aopInRn (from, from_offset) || aopInReg (from, from_offset, B_IDX) || aopInReg (from, from_offset, DPL_IDX) || aopInReg (from, from_offset, DPH_IDX)))
    {
      emitcode ("mov", "%s, %s", to->aopu.aop_reg[to_offset]->name, from->aopu.aop_reg[from_offset]->dname);
    }
  else if (aopInReg (to, to_offset, A_IDX))
    {
      emitcode ("mov", "a, %s", aopGet (from, from_offset, false, false));
    }
  else if (aopDir (to, to_offset) && aopInRn (from, from_offset))
    {
      emitcode ("mov", "%s, %s", aopGet (to, to_offset, false, true), from->aopu.aop_reg[from_offset]->name);
    }
  else if (aopInReg (from, from_offset, A_IDX))
    {
      aopPut (to, "a", to_offset);
    }
  else if (aopInRn (to, to_offset) && (from->type == AOP_R0 || from->type == AOP_R1))
    {
      emitcode ("mov", "%s, %s", to->aopu.aop_reg[to_offset]->dname, aopGet (from, from_offset, false, true));
    }
  else if (aopInRn (to, to_offset) && (!aopGetUsesAcc (from, from_offset) || a_dead))
    {
      emitcode ("mov", "%s, %s", to->aopu.aop_reg[to_offset]->name, aopGet (from, from_offset, false, true));
    }
  else if ((aopInReg (to, to_offset, B_IDX) || aopInReg (to, to_offset, DPL_IDX) || aopInReg (to, to_offset, DPH_IDX)) && (!aopGetUsesAcc (from, from_offset) || a_dead))
    {
      emitcode ("mov", "%s, %s", to->aopu.aop_reg[to_offset]->dname, aopGet (from, from_offset, false, !aopGetUsesAcc (from, from_offset)));
    }
  else if ((to->type == AOP_R0 || to->type == AOP_R1) && !to->paged && aopDir (from, from_offset)) // Can't be part of the case below, since aopPut wants to pass the value through acc, which is less efficient.
    {
      mcs251_setAopR01Offset (to, to_offset);
      emitcode ("mov", "@%s, %s", to->aopu.aop_ptr->name, aopGet (from, from_offset, false, true));
    }
  else if ((aopDir (to, to_offset) && aopDir (from, from_offset) ||
    aopDir (to, to_offset) && (from->type == AOP_R0 || from->type == AOP_R1) && !from->paged))
    {
      aopPut (to, aopGet (from, from_offset, false, true), to_offset);
    }
  else if (a_dead)
    {
      cheapMove (ASMOP_A, 0, from, from_offset, true);
      cheapMove (to, to_offset, ASMOP_A, 0, true);
    }
  else if (aopInRn (from, from_offset))
    {
      emitAccumulatorExchange (from->aopu.aop_reg[from_offset]->name);
      cheapMove (to, to_offset, ASMOP_A, 0, false);
      emitAccumulatorExchange (from->aopu.aop_reg[from_offset]->name);
    }
  else if (aopInRn (to, to_offset))
    {
      emitAccumulatorExchange (to->aopu.aop_reg[to_offset]->name);
      cheapMove (ASMOP_A, 0, from, from_offset, true);
      emitAccumulatorExchange (to->aopu.aop_reg[to_offset]->name);
    }
  else if (!a_dead)
    {
      emitpush ("acc");
      cheapMove (to, to_offset, from, from_offset, true);
      emitpop ("acc");
    }
  else
    wassert (0);
}

/*-----------------------------------------------------------------*/
/* genCopy - Copy the value from one reg/stk asmop to another.     */
/*-----------------------------------------------------------------*/
static void
genCopy (asmop *result, int roffset, asmop *source, int soffset, int sizex, bool a_dead)
{
  int regsize, size, n = (sizex < source->size - soffset) ? sizex : (source->size - soffset);
  bool assigned[8] = {false, false, false, false, false, false, false, false};
  //bool a_free;
  //int cached_byte = -1;
  //bool pushed_a = false;

  wassertl_bt (n <= 8, "Invalid size for genCopy().");
  //wassertl_bt (aopRS (source), "Invalid source type.");
  //wassertl_bt (aopRS (result), "Invalid result type.");
  wassertl_bt ((result->type == AOP_REG || result->type == AOP_ACC) && (source->type == AOP_REG || source->type == AOP_ACC), "genCopy not yet implemented for non-register operands");

  //a_dead |= (result->regs[A_IDX] >= roffset && result->regs[A_IDX] < roffset + sizex); todo: enable when supported by struct asmop

  size = n;
  regsize = 0;
  for (int i = 0; i < n; i++)
    regsize += (source->type == AOP_REG);

  // Do nothing for coalesced bytes.
  for (int i = 0; i < n; i++)
    if (result->type == AOP_REG && source->type == AOP_REG && result->aopu.aop_reg[roffset + i] == source->aopu.aop_reg[soffset + i])
      {
        assigned[i] = true;
        regsize--;
        size--;
      }

  // Move everything from registers to the stack.
  wassert (source->type != AOP_REG || source->size <= 8);
  // todo

  // Copy (stack-to-stack) what we can with whatever free regs we have.
  // todo
  // genCopyStack ();

  // Now do the register shuffling.

  // Try to use xch (even triple is cheaper than the 4 mov via a needed otherwise, also does not require a to be free).
  for (int i = 0; i < sizex; i++)
    for (int j = 0; j < sizex; j++)
      {
        if (assigned[i] || assigned[j])
          continue;
        if (!(aopInReg (source, soffset + i, A_IDX) || aopInRn (source, soffset + i)) ||
          !(aopInReg (result, roffset + i, A_IDX) || aopInRn (result, roffset + i)) ||
          !(aopInReg (source, soffset + j, A_IDX) || aopInRn (source, soffset + j)) ||
          !(aopInReg (result, roffset + j, A_IDX) || aopInRn (result, roffset + j)))
          continue;
        if (source->aopu.aop_reg[soffset + i] != result->aopu.aop_reg[roffset + j] || source->aopu.aop_reg[soffset + j] != result->aopu.aop_reg[roffset + i])
          continue;
        if (aopInReg (source, soffset + i, A_IDX))
          {
            emitAccumulatorExchange (source->aopu.aop_reg[soffset + j]->name);
          }
        else if (aopInReg (result, roffset + i, A_IDX))
          {
            emitAccumulatorExchange (source->aopu.aop_reg[soffset + i]->name);
          }
        else
          {
            emitAccumulatorExchange (source->aopu.aop_reg[soffset + i]->name);
            emitAccumulatorExchange (source->aopu.aop_reg[soffset + j]->name);
            emitAccumulatorExchange (source->aopu.aop_reg[soffset + i]->name);
          }
        assigned[i] = assigned[j] = true;
        regsize -= 2;
        size -= 2;
      }

  while (regsize)
    {
      int i;

      // Find lowest byte that can be assigned and needs to be assigned.
      for (i = 0; i < n; i++)
        {
          if (assigned[i])
            continue;

          for (int j = 0; j < n; j++)
            {
              if (!assigned[j] && i != j && result->aopu.aop_reg[roffset + i] == source->aopu.aop_reg[soffset + j])
                goto skip_byte; // We can't write this one without overwriting the source.
            }

          break;                // Found byte that can be written safely.

skip_byte:
          ;
        }

      if (i < n)
        {
          cheapMove (result, roffset + i, source, soffset + i, false);       // We can safely assign a byte.
          regsize--;
          size--;
          assigned[i] = true;
          continue;
        }

      // No byte can be assigned safely (i.e. the assignment is a permutation).
      //if (!regalloc_dry_run)
      //  wassertl_bt (0, "Unimplemented.");
      //cost (180, 180);
      wassert (0);
      return;
    }

  // Copy (stack-to-stack) what we can with whatever free regs we have now.
  // todo
  // genCopyStack ();

  // Last, move everything else from stack to registers.
  wassert (result->type != AOP_REG || result->size <= 8);
  // todo

  wassertl_bt (size >= 0, "genCopy() copied more than there is to be copied.");
}

