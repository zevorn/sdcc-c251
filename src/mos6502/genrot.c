/*-------------------------------------------------------------------------
  genrot.c - source file for rotate code generation for the MOS6502

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 1999, Jean-Louis VERN.jlvern@writeme.com
  Bug Fixes - Wojciech Stryjewski  wstryj1@tiger.lsu.edu (1999 v2.1.9a)
  Hacked for the HC08:
  Copyright (C) 2003, Erik Petrich
  Hacked for the MOS6502:
  Copyright (C) 2020, Steven Hugg  hugg@fasterlight.com
  Copyright (C) 2021-2026, Gabriele Gorla

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
  -------------------------------------------------------------------------*/

#include "m6502.h"
#include "ralloc.h"
#include "gen.h"

/**************************************************************************
 * genRRC - rotate right with carry
 *************************************************************************/
static void
genRRC (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  bool resultInA = false;
  char *shift;

  emitComment (TRACEGEN, __func__);

  /* rotate right with carry */
  aopOp (left, ic);
  aopOp (result, ic);
  printIC (ic);

  if(IS_AOP_WITH_A(AOP(result))) resultInA=true;
  size = AOP_SIZE (result);
  offset = size - 1;

  shift = "lsr";
  if(IS_AOP_XA(AOP(left)))
    {
      storeRegTempAlways (m6502_reg_x, true);
      emit6502op ("lsr", TEMPFMT, getLastTempOfs() );
      emit6502op ("ror", "a");
      if(IS_AOP_XA(AOP(result)) )
        {
	  storeRegTemp (m6502_reg_a, true);
	  loadRegFromConst (m6502_reg_a, 0);
	  emit6502op("ror", "a");
	  emit6502op ("ora", TEMPFMT, getLastTempOfs()-1 );
	  transferRegReg (m6502_reg_a, m6502_reg_x, true);
	  loadRegTemp (m6502_reg_a);
	}
      else
	{
	  // optimization if the result is in DIR or EXT
	  storeRegToAop (m6502_reg_a, AOP(result), 0);
	  loadRegFromConst (m6502_reg_a, 0);
	  emit6502op ("ror", "a");
	  emit6502op ("ora", TEMPFMT, getLastTempOfs() );
	  storeRegToAop(m6502_reg_a, AOP(result), 1);
        }
      loadRegTemp (NULL);
      goto release;
    }
  else if (sameRegs (AOP (left), AOP (result)))
    {
      while (size--)
        {
          rmwWithAop (shift, AOP (result), offset--);
          shift = "ror";
        }
    }
  else
    {
      while (size--)
        {
          loadRegFromAop (m6502_reg_a, AOP (left), offset);
          rmwWithReg (shift, m6502_reg_a);
          storeRegToAop (m6502_reg_a, AOP (result), offset);
          shift = "ror";
          offset--;
        }
    }

  if(resultInA) storeRegTemp(m6502_reg_a, true);

  /* now we need to put the carry into the
     highest order byte of the result */
  offset = AOP_SIZE (result) - 1;
  loadRegFromConst(m6502_reg_a, 0);
  emit6502op ("ror", "a");
  accopWithAop ("ora", AOP (result), offset);
  storeRegToAop (m6502_reg_a, AOP (result), offset);

  if(resultInA) loadRegTemp(m6502_reg_a);

 release:
  //  pullOrFreeReg (m6502_reg_a, needpula);

  freeAsmop (left, NULL);
  freeAsmop (result, NULL);
}

/**************************************************************************
 * genRLC - generate code for rotate left with carry
 *************************************************************************/
static void
genRLC (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  char *shift;
  bool resultInA = false;
  bool needpulla = false;

  emitComment (TRACEGEN, __func__);

  /* rotate right with carry */
  aopOp (left, ic);
  aopOp (result, ic);
  printIC(ic);

  if(IS_AOP_WITH_A(AOP(result))) resultInA=true;
  size = AOP_SIZE (result);
  offset = 0;

  shift = "asl";
  if (!resultInA && sameRegs (AOP (left), AOP (result)))
    {
      while (size--)
	{
	  rmwWithAop (shift, AOP (result), offset++);
	  shift = "rol";
	}
    }
  else
    {
      while (size--)
	{
	  loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  rmwWithReg (shift, m6502_reg_a);
	  if(offset==0 && resultInA) storeRegTemp (m6502_reg_a, true);
	  else storeRegToAop (m6502_reg_a, AOP (result), offset);
	  shift = "rol";
	  offset++;
	}
    }

  /* now we need to put the carry into the
     lowest order byte of the result */
  needpulla=pushRegIfSurv(m6502_reg_a);
  offset = 0;
  loadRegFromConst(m6502_reg_a, 0);
  emit6502op ("rol", "a");
  if (resultInA)
    {
      emit6502op("ora", TEMPFMT, getLastTempOfs() );
      loadRegTemp(NULL);
    }
  else
    {
      accopWithAop ("ora", AOP (result), offset);
    }
  storeRegToAop (m6502_reg_a, AOP (result), offset);

  pullOrFreeReg (m6502_reg_a, needpulla);

  freeAsmop (left, NULL);
  freeAsmop (result, NULL);
}

/**************************************************************************
 * genRotX - rotate a 16/32 bit value by a known amount
 *************************************************************************/
static void
genRotX(iCode *ic, int shCount)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);
  bool resultInXA = false;
  bool needpulla = false;
  int i, size;

  //emitComment (TRACEGEN, __func__);
  aopOp (left, ic);
  aopOp (result, ic);
  printIC(ic);

  size = AOP_SIZE (result);

  if(IS_AOP_XA(AOP(result)))
    resultInXA=true;

  if(!resultInXA)
    needpulla=pushRegIfSurv(m6502_reg_a);

  emitComment (TRACEGEN, "%s - size=%d shCount=%d (res in XA:%c)",__func__, size, shCount,
               resultInXA?'Y':'N');

  if(resultInXA)
    {
      if(shCount<8)
	{
	  emitComment (TRACEGEN|VVDBG, "%s - in A",__func__);
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  storeRegTempAlways(m6502_reg_a, true);
	  dirtyRegTemp (getLastTempOfs());
	  loadRegFromAop (m6502_reg_a, AOP (left), 1);
	}
      else
	{
	  shCount-=8;
	  emitComment (TRACEGEN|VVDBG, "%s - shCount>=8",__func__);
	  // try reversing
#if 0
	  loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  storeRegTempAlways(m6502_reg_x, true);
	  dirtyRegTemp (getLastTempOfs());
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
#else
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  storeRegTempAlways(m6502_reg_x, true);
	  dirtyRegTemp (getLastTempOfs());
#endif
	}

      for(i=0;i<shCount;i++)
	{
	  m6502_emitCmp(m6502_reg_a, 0x80);
	  emit6502op("rol", TEMPFMT, getLastTempOfs() );
	  rmwWithReg ("rol", m6502_reg_a);
	}

      transferRegReg(m6502_reg_a, m6502_reg_x, true);
      loadRegTemp(m6502_reg_a);
    }
  else
    {
      /////////////////////////////////////////////////////////
      // not XA
      /////////////////////////////////////////////////////////
      int msb = size-1;
      int offset;
      int ror = false;
      if((shCount%8)>4)
        {
	  emitComment (TRACEGEN|VVDBG, "%s - enable ROR",__func__);
          shCount+=8;
          shCount%=(8*size);
          ror=true;
        }

      if(shCount<8)
	{
	  for(offset=0;offset<size-1;offset++)
	    transferAopAop (AOP(left), offset, AOP (result), offset);
	  loadRegFromAop (m6502_reg_a, AOP (left), msb);
	}
      else if(shCount<16)
	{
	  shCount-=8;
	  loadRegFromAop (m6502_reg_a, AOP (left), msb-1);
	  for(offset=msb-1;offset>=0;offset--)
	    transferAopAop (AOP(left), (offset-1+size)%size, AOP (result), offset);
	} 
      else if(shCount<24)
	{
	  shCount-=16;
	  if(!sameRegs (AOP (left), AOP (result)))
	    {
	      for(offset=0;offset<size-1;offset++)
		transferAopAop (AOP(left), (offset+2)%size, AOP (result), offset);
	      loadRegFromAop (m6502_reg_a, AOP (left), (msb+2)%size);
	    }
	  else
	    {
	      //FIXME: only works for 32-bit
	      loadRegFromAop (m6502_reg_a, AOP (left), 0);
	      transferAopAop (AOP(left), 2, AOP (result), 0);
	      storeRegToAop (m6502_reg_a, AOP (result), 2);
	      loadRegFromAop (m6502_reg_a, AOP (left), 1);
	      transferAopAop (AOP(left), 3, AOP (result), 1);
	    }
	}
      else
	{
	  shCount-=24;
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  for(offset=1;offset<size;offset++)
	    transferAopAop (AOP(left), offset, AOP (result), offset-1);

	  //      for(offset=0;offset<size-1;offset++)
	  //        transferAopAop (AOP(left), offset, AOP (result), offset);
	  //      loadRegFromAop (m6502_reg_a, AOP (left), msb);

	}

      if(IS_AOP_XA(AOP(left)))
	m6502_freeReg(m6502_reg_x);

      if(ror)
	{
	  // rotate right
	  shCount=8-shCount;
	  storeRegToAop (m6502_reg_a, AOP (result), msb);

	  while(shCount--)
	    {
	      loadRegFromAop (m6502_reg_a, AOP (result), 0);
	      rmwWithReg ("lsr", m6502_reg_a);

	      for(offset=size-1;offset>=0;offset--)
		rmwWithAop ("ror", AOP (result), offset);

	    }

	}
      else
	{
	  for(i=0;i<shCount;i++)
	    {
	      m6502_emitCmp(m6502_reg_a, 0x80);
	      for(offset=0;offset<size-1;offset++)
		rmwWithAop ("rol", AOP (result), offset);
	      rmwWithReg ("rol", m6502_reg_a);
	    }

	  storeRegToAop (m6502_reg_a, AOP (result), msb);

	}
    }

  pullOrFreeReg (m6502_reg_a, needpulla);

  freeAsmop (left, NULL);
  freeAsmop (result, NULL);
}

/**************************************************************************
 * genRot8 - rotate a 8-bit value by a known amount
 *************************************************************************/
static void
genRot8(iCode *ic, int shCount)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  bool resultInA = false;
  bool needpulla = false;

  emitComment (TRACEGEN, __func__);
  emitComment (TRACEGEN, "%s - size=1 shCount=%d",__func__, shCount);

  aopOp (left, ic);
  aopOp (result, ic);
  printIC(ic);

  if(IS_AOP_WITH_A(AOP(result)))
    resultInA=true;

  if(!resultInA)
    needpulla=pushRegIfSurv(m6502_reg_a);

  loadRegFromAop (m6502_reg_a, AOP (left), 0);

  if(shCount>5)
    {
      // right
      shCount=8-shCount;

      while(shCount--)
	{
	  storeRegTempAlways (m6502_reg_a, true);
          dirtyRegTemp (getLastTempOfs());
	  emit6502op("lsr", TEMPFMT, getLastTempOfs() );
	  rmwWithReg ("ror", m6502_reg_a);
	  loadRegTemp(NULL);
	}
    }
  else
    {
      if(shCount>=4)
	{
	  shCount-=4;
	  rmwWithReg ("asl", m6502_reg_a);
	  emit6502op ("adc", "#0x80");
	  rmwWithReg ("rol", m6502_reg_a);
	  rmwWithReg ("asl", m6502_reg_a);
	  emit6502op ("adc", "#0x80");
	  rmwWithReg ("rol", m6502_reg_a);  
	}
      if(shCount>0)
	{
	  // left
	  while(shCount--)
	    {
	      m6502_emitCmp(m6502_reg_a, 0x80);
	      rmwWithReg ("rol", m6502_reg_a);
	    }
	}
    }
  storeRegToAop (m6502_reg_a, AOP (result), 0);
  pullOrFreeReg (m6502_reg_a, needpulla);

  freeAsmop (left, NULL);
  freeAsmop (result, NULL);
}

/**************************************************************************
 * genRot - generates code for rotation
 *************************************************************************/
void
m6502_genRot (iCode *ic)
{
  operand *left = IC_LEFT (ic);
  operand *right = IC_RIGHT (ic);
  unsigned int lbits = bitsForType (operandType (left));
  if (IS_OP_LITERAL (right) && lbits==8 )
    genRot8(ic, operandLitValueUll (right) % lbits );
  else if (IS_OP_LITERAL (right) && operandLitValueUll (right) % lbits ==  lbits - 1)
    genRRC (ic);
  else if (IS_OP_LITERAL (right) && operandLitValueUll (right) % lbits == 1)
    genRLC (ic);
  else if (IS_OP_LITERAL (right))
    genRotX(ic, operandLitValueUll (right) % lbits );
  else
    emitcode("ERROR", "%s - Unimplemented rotation (lbits=%d)", __func__, lbits);    
}

