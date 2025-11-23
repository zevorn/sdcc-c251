/*-------------------------------------------------------------------------
  genlshift.c - source file for left shift code generation for the MOS6502

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 1999, Jean-Louis VERN.jlvern@writeme.com
  Bug Fixes - Wojciech Stryjewski  wstryj1@tiger.lsu.edu (1999 v2.1.9a)
  Hacked for the HC08:
  Copyright (C) 2003, Erik Petrich
  Hacked for the MOS6502:
  Copyright (C) 2020, Steven Hugg  hugg@fasterlight.com
  Copyright (C) 2021-2025, Gabriele Gorla

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
#include "dbuf_string.h"

/**************************************************************************
 * AccLsh - left shift accumulator by known count
 *************************************************************************/
void
AccLsh (int shCount)
{
  int i;

  shCount &= 0x0007;            // shCount : 0..7

  if(shCount==7)
    {
      emit6502op ("ror", "a");
      loadRegFromConst(m6502_reg_a, 0);
      emit6502op ("ror", "a");
    }
  else if(shCount==6)
    {
      emit6502op ("ror", "a");
      emit6502op ("ror", "a");
      emit6502op ("ror", "a");
      emit6502op ("and", "#0xc0");
    }
  else
    {
      /* asl a is 2 cycles and 1 byte, so an unrolled loop is the   */
      /* fastest and shortest (shCount<6).                          */
      for (i = 0; i < shCount; i++)
	emit6502op ("asl", "a");
    }
}


/**************************************************************************
 * XAccLsh - left shift register pair XA by known count
 *************************************************************************/
void
XAccLsh (reg_info *msb_reg, int shCount)
{
  int i;

  shCount &= 0x000f;            // shCount : 0..15

  if (shCount >= 8)
    {
      AccLsh (shCount - 8);
      transferRegReg (m6502_reg_a, msb_reg, false);
      loadRegFromConst (m6502_reg_a, 0);
    }
  else if(shCount==7)
    {
      storeRegTempAlways(msb_reg, true);
      emit6502op ("lsr", TEMPFMT, getLastTempOfs() );
      dirtyRegTemp(getLastTempOfs() );
      rmwWithReg ("ror", m6502_reg_a);
      transferRegReg (m6502_reg_a, msb_reg, false);
      loadRegFromConst (m6502_reg_a, 0);
      rmwWithReg ("ror", m6502_reg_a);
      loadRegTemp(NULL);
    }
  else if(shCount!=0)
    {
      /* lsla/rolx is only 2 cycles and bytes, so an unrolled loop is often  */
      /* the fastest and shortest.                                           */
      storeRegTempAlways(msb_reg, true);
      for (i = 0; i < shCount; i++)
	{
	  rmwWithReg ("asl", m6502_reg_a);
	  emit6502op ("rol", TEMPFMT, getLastTempOfs() );
	  dirtyRegTemp(getLastTempOfs() );
	}
      loadRegTemp(msb_reg);
    }
}

/**************************************************************************
 * genlsh8 - left shift a one byte quantity by known count
 *************************************************************************/
static void
genlsh8 (operand * result, operand * left, int shCount)
{
  sym_link *resulttype = operandType (result);
  unsigned bytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedbyte = (bytemask != 0xff);
  bool needpulla = false;

  emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);
  if (shCount==0)
    return;

  if (!IS_AOP_A(AOP(result)) && sameRegs (AOP (left), AOP (result)) 
      && shCount<3 && aopCanShift(AOP(left)) && !maskedbyte)
    {
      while (shCount--)
        rmwWithAop ("asl", AOP (result), 0);
    }
  else
    {
      if(!IS_AOP_A(AOP(result)))
	needpulla = pushRegIfSurv (m6502_reg_a);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      AccLsh (shCount);
      if (maskedbyte)
	emit6502op ("and", IMMDFMT, bytemask);
      storeRegToAop (m6502_reg_a, AOP (result), 0);
      pullOrFreeReg (m6502_reg_a, needpulla);
    }
}

/**************************************************************************
 * genlsh16 - left shift two bytes by known amount != 0
 *************************************************************************/
static void
genlsh16 (operand * result, operand * left, int shCount)
{
  bool needpulla, needpullx;

  sym_link *resulttype = operandType (result);
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);

  if (shCount >= 8)
    {
      shCount -= 8;
      // TODO
      needpulla = pushRegIfSurv (m6502_reg_a);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      AccLsh (shCount);
      if (maskedtopbyte)
	emit6502op ("and", IMMDFMT, topbytemask);
      storeRegToAop (m6502_reg_a, AOP (result), 1);
      storeConstToAop (0, AOP (result), 0);
      pullOrFreeReg (m6502_reg_a, needpulla);
      goto done; // Top byte is 0, doesn't need masking.
    }
  else if(IS_AOP_XA(AOP(result)))
    {
      if(shCount==1 && !IS_AOP_XA(AOP(left)))
        {
          loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  emit6502op ("asl", "a");
          m6502_dirtyReg(m6502_reg_a);
          fastSaveA();
          loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  emit6502op ("rol", "a");
          transferRegReg(m6502_reg_a, m6502_reg_x, true);
          fastRestoreA();
        }
      else
        {
	  /*  1 <= shCount <= 7 */
	  // TODO: count > 2 efficient?
	  loadRegFromAop (m6502_reg_xa, AOP (left), 0);
	  XAccLsh (m6502_reg_x, shCount);
	}
    }
  else if(aopCanShift(AOP(result)) && shCount <= 4)
    {
      if( sameRegs (AOP (left), AOP (result)))
	{
	  while (shCount--)
	    {
	      rmwWithAop ("asl", AOP (result), 0);
	      rmwWithAop ("rol", AOP (result), 1);
	    }
	}
      else
	{
	  needpulla = storeRegTempIfSurv (m6502_reg_a);
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  emit6502op ("asl", "a");
	  storeRegToAop (m6502_reg_a, AOP (result), 0);
	  loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  emit6502op ("rol", "a");
	  while(--shCount)
	    {
	      rmwWithAop ("asl", AOP (result), 0);
	      emit6502op ("rol", "a");
	    }
	  storeRegToAop (m6502_reg_a, AOP (result), 1);
	  loadOrFreeRegTemp (m6502_reg_a, needpulla);
	}
    }
  else
    {
      needpulla = storeRegTempIfSurv (m6502_reg_a);
      needpullx = storeRegTempIfSurv (m6502_reg_x);
      loadRegFromAop (m6502_reg_xa, AOP (left), 0);
      XAccLsh (m6502_reg_x, shCount);
      storeRegToFullAop (m6502_reg_xa, AOP (result), 0);
      loadOrFreeRegTemp (m6502_reg_x, needpullx);
      loadOrFreeRegTemp (m6502_reg_a, needpulla);
    }

  if (maskedtopbyte)
    {
      bool in_a = (result->aop->type == AOP_REG && result->aop->aopu.aop_reg[1]->rIdx == A_IDX);
      bool needpull = false;
      if (!in_a)
	{
	  needpull = pushRegIfUsed (m6502_reg_a);
	  loadRegFromAop (m6502_reg_a, result->aop, 1);
	}
      emit6502op ("and", IMMDFMT, topbytemask);
      if (!in_a)
	{
	  storeRegToAop (m6502_reg_a, result->aop, 1);
	  pullOrFreeReg (m6502_reg_a, needpull);
	}
    }
 done:
  ;
}

/**************************************************************************
 * shiftLLongInPlace - shift left one long in place
 *
 * @param result pointer to the dst aop
 * @param shift  number of shifts (must be >=0 and <8)
 * @param ofs    LSB to begin the shift (must be >=0 and <4)
 * @param msb_in_a MSB is already in A
 *************************************************************************/
static void
shiftLLongInPlace (operand * result, int shift, int ofs, bool msb_in_a)
{
  int i;

  if(shift==0)
    return;

  if(!msb_in_a && shift==1 /*|| (shift==2 && AOP_TYPE(result)==AOP_DIR) */ )
    {
      while(shift--)
	{
	  rmwWithAop ("asl", AOP(result), ofs);
	  for(i=ofs+1; i<4; i++)
	    rmwWithAop ("rol", AOP(result), i);
	}
    }
  else
    {
#if 1
      if(!msb_in_a)
        loadRegFromAop (m6502_reg_a, AOP (result), 3);
      while(shift)
	{
	  if(ofs<3)
	    rmwWithAop ("asl", AOP(result), ofs);
	  if(ofs<2)
	    rmwWithAop ("rol", AOP(result), ofs+1);
	  if(ofs<1)
	    rmwWithAop ("rol", AOP(result), ofs+2);

	  if(ofs==3)
	    rmwWithReg ("asl", m6502_reg_a);
	  else
	    rmwWithReg ("rol", m6502_reg_a);
	  --shift;
	}
      if(!msb_in_a)
        storeRegToAop (m6502_reg_a, AOP (result), 3);

#else
      if(!msb_in_a)
	loadRegFromAop (m6502_reg_a, AOP (result), ofs);

      while(shift)
	{
	  rmwWithReg ("asl", m6502_reg_a);
	  for(i=ofs+1;i<4;i++)
	    rmwWithAop ("rol", AOP(result), i);

	  --shift;
	}
      if(!msb_in_a)
	storeRegToAop (m6502_reg_a, AOP (result), ofs);
#endif
    }
}

/**************************************************************************
 * shiftLLong1 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=24 and <32)
 *************************************************************************/
static void
shiftLLong1 (operand * left, operand * result, int shift)
{
  bool needpulla = false;

  wassertl(shift>=24, "shiftLLong1 - shift<24");
  wassertl(shift<32,  "shiftLLong1 - shift>=32");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==24)
    {
      //      transferAopAop (AOP (left), 0, AOP (result), 3);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
      storeConstToAop(0, AOP (result), 0);
      storeConstToAop(0, AOP (result), 1);
      storeConstToAop(0, AOP (result), 2);
    } 
  else if(shift==31)
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      rmwWithReg ("lsr", m6502_reg_a);
      loadRegFromConst (m6502_reg_a, 0);
      storeRegToAop (m6502_reg_a, AOP (result) , 2); // out of order store to save a load
      storeRegToAop (m6502_reg_a, AOP (result), 1); // out of order store to save a load
      storeRegToAop (m6502_reg_a, AOP (result), 0); // out of order store to save a load
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  else if(shift==30)
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      rmwWithReg ("ror", m6502_reg_a);
      rmwWithReg ("ror", m6502_reg_a);
      rmwWithReg ("ror", m6502_reg_a);
      emit6502op ("and", IMMDFMT, 0xc0);
      storeConstToAop (0, AOP (result), 2);
      storeConstToAop (0, AOP (result), 1);
      storeConstToAop (0, AOP (result), 0);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  else
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      storeConstToAop (0, AOP (result), 2);
      storeConstToAop (0, AOP (result), 1);
      storeConstToAop (0, AOP (result), 0);
      shiftLLongInPlace (result, shift-24, 3, true);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftLLong2 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=16 and <24)
 *************************************************************************/
static void
shiftLLong2 (operand * left, operand * result, int shift)
{
  bool needpulla = false;

  wassertl(shift>=16, "shiftLLong2 - shift<16");
  wassertl(shift<24,  "shiftLLong2 - shift>23");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==16)
    {
      //     transferAopAop (AOP (left), 0, AOP (result), 2);
      //     transferAopAop (AOP (left), 1, AOP (result), 3);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      storeRegToAop (m6502_reg_a, AOP (result), 2);
      loadRegFromAop (m6502_reg_a, AOP (left), 1);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
      storeConstToAop (0, AOP (result), 1);
      storeConstToAop (0, AOP (result), 0);
    }
  else if(shift>21)
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 1);
      rmwWithReg ("lsr", m6502_reg_a);
      if(shift==22)
	m6502_pushReg(m6502_reg_a, false);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
      loadRegFromConst (m6502_reg_a, 0);
      storeRegToAop (m6502_reg_a, AOP (result), 1); // out of order store to save a load
      storeRegToAop (m6502_reg_a, AOP (result), 0); // out of order store to save a load
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 2);
      if(shift==22)
	{
	  m6502_pullReg(m6502_reg_a);
	  rmwWithReg ("lsr", m6502_reg_a);
	  rmwWithAop ("ror", AOP(result), 3);
	  rmwWithAop ("ror", AOP(result), 2);
	}
    }
  else
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      rmwWithReg ("asl", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 2);
      loadRegFromAop (m6502_reg_a, AOP (left), 1);
      rmwWithReg ("rol", m6502_reg_a);
      //storeRegToAop (m6502_reg_a, AOP (result), 3);
      storeConstToAop (0, AOP (result), 1);
      storeConstToAop (0, AOP (result), 0);
      shiftLLongInPlace (result, shift-17, 2, true);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
    }

  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftLLong3 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=8 and <16)
 *************************************************************************/
static void
shiftLLong3 (operand * left, operand * result, int shift)
{
  bool needpulla = false;
  bool needloadx = false;

  wassertl(shift>=8, "shiftLLong3 - shift<8");
  wassertl(shift<16,  "shiftLLong3 - shift>=16");

  //  if(shift==8 && AOP_TYPE(left)!=AOP_SOF &&  AOP_TYPE(result)!=AOP_SOF)
  //    reg=getFreeByteReg();
  //  if(!reg)
  //    reg=m6502_reg_a;

  needpulla = pushRegIfUsed (m6502_reg_a);

  //  idx=getFreeIdxReg();
  //  if(!idx)
  //    idx=m6502_reg_x;

  if(shift==8)
    {
      //      transferAopAop (AOP (left), 2, AOP (result), 3);
      //      transferAopAop (AOP (left), 1, AOP (result), 2);
      //      transferAopAop (AOP (left), 0, AOP (result), 1);
      loadRegFromAop (m6502_reg_a, AOP (left), 2);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
      loadRegFromAop (m6502_reg_a, AOP (left), 1);
      storeRegToAop (m6502_reg_a, AOP (result), 2);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_freeReg(m6502_reg_a);
      storeConstToAop (0, AOP (result), 0);
    }
  else if(shift>13)
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 2);
      rmwWithReg ("lsr", m6502_reg_a);
      if(shift==14)
	m6502_pushReg(m6502_reg_a, false);
      loadRegFromAop (m6502_reg_a, AOP (left), 1);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 2);
      loadRegFromConst (m6502_reg_a, 0);
      storeRegToAop (m6502_reg_a, AOP (result), 0); // out of order store to save a load
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 1);
      if(shift==14)
	{
	  m6502_pullReg(m6502_reg_a);
	  rmwWithReg ("lsr", m6502_reg_a);
	  rmwWithAop ("ror", AOP(result), 3);
	  rmwWithAop ("ror", AOP(result), 2);
	  rmwWithAop ("ror", AOP(result), 1);
	}
    }
  else 
    {
      if(!sameRegs (AOP (left), AOP (result)))
	{ 
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  rmwWithReg ("asl", m6502_reg_a);
	  storeRegToAop (m6502_reg_a, AOP (result), 1);
	  loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  rmwWithReg ("rol", m6502_reg_a);
	  storeRegToAop (m6502_reg_a, AOP (result), 2);
	  loadRegFromAop (m6502_reg_a, AOP (left), 2);
	  rmwWithReg ("rol", m6502_reg_a);
	  //	  storeRegToAop (m6502_reg_a, AOP (result), 3);
	  storeConstToAop (0, AOP (result), 0);
	}
      else
	{
	  needloadx = storeRegTempIfUsed (m6502_reg_x);

	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  rmwWithReg ("asl", m6502_reg_a);
	  loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  storeRegToAop (m6502_reg_a, AOP (result), 1);
	  transferRegReg(m6502_reg_x, m6502_reg_a, true);
	  rmwWithReg ("rol", m6502_reg_a);
	  loadRegFromAop (m6502_reg_x, AOP (left), 2);
	  storeRegToAop (m6502_reg_a, AOP (result), 2);

	  transferRegReg(m6502_reg_x, m6502_reg_a, true);
	  rmwWithReg ("rol", m6502_reg_a);
	  //	  storeRegToAop (m6502_reg_a, AOP (result), 3);
	  storeConstToAop (0, AOP (result), 0);
	}
      shiftLLongInPlace (result, shift-9, 1, true);
      storeRegToAop (m6502_reg_a, AOP (result), 3);

    }
  loadOrFreeRegTemp (m6502_reg_x, needloadx);
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftLLong4 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >0 and <8)
 *************************************************************************/
static void
shiftLLong4 (operand * left, operand * result, int shift)
{
  bool needpulla = false;

  wassertl(shift>0, "shiftLLong4 - shift==0");
  wassertl(shift<8, "shiftLLong4 - shift>=8");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift>5)
    {
      loadRegFromAop (m6502_reg_a, AOP (left), 3);
      rmwWithReg ("lsr", m6502_reg_a);
      if(shift!=7)
        fastSaveA();
	//             storeRegTempAlways(m6502_reg_a, true);
      loadRegFromAop (m6502_reg_a, AOP (left), 2);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 3);
      loadRegFromAop (m6502_reg_a, AOP (left), 1);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 2);
      loadRegFromAop (m6502_reg_a, AOP (left), 0);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 1);
      loadRegFromConst (m6502_reg_a, 0);
      rmwWithReg ("ror", m6502_reg_a);
      storeRegToAop (m6502_reg_a, AOP (result), 0);
      if(shift!=7)
	{
          fastRestoreA();
	  //        loadRegTemp(m6502_reg_a);
	  while(shift!=7)
	    {
	      rmwWithReg ("lsr", m6502_reg_a);
	      //           emitRegTempOp("lsr", getLastTempOfs() );
	      rmwWithAop ("ror", AOP(result), 3);
	      rmwWithAop ("ror", AOP(result), 2);
	      rmwWithAop ("ror", AOP(result), 1);
	      rmwWithAop ("ror", AOP(result), 0);
              shift++;
	    }
	  //           loadRegTemp(NULL);
	}
    }
  else
    {
      if(!sameRegs (AOP (left), AOP (result)))
	{ 
	  loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  rmwWithReg ("asl", m6502_reg_a);
	  storeRegToAop (m6502_reg_a, AOP (result), 0);
	  loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  rmwWithReg ("rol", m6502_reg_a);
	  storeRegToAop (m6502_reg_a, AOP (result), 1);
	  loadRegFromAop (m6502_reg_a, AOP (left), 2);
	  rmwWithReg ("rol", m6502_reg_a);
	  storeRegToAop (m6502_reg_a, AOP (result), 2);
	  loadRegFromAop (m6502_reg_a, AOP (left), 3);
	  rmwWithReg ("rol", m6502_reg_a);
	  //	  storeRegToAop (m6502_reg_a, AOP (result), 3);
          shiftLLongInPlace (result, shift-1, 0, true);
          storeRegToAop (m6502_reg_a, AOP (result), 3);
	}
      else
        {
	  //  loadRegFromAop (m6502_reg_a, AOP (left), 3);
          shiftLLongInPlace (result, shift, 0, false);
	  //  storeRegToAop (m6502_reg_a, AOP (result), 3);
        }
    }
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genlsh32 - shift four byte by a known amount != 0
 *************************************************************************/
static void
genlsh32 (operand * result, operand * left, int shCount)
{
  emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);

  sym_link *resulttype = operandType (result);
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  wassertl(AOP_SIZE (result)==4, "genlsh32 - AOP_SIZE != 4");

  /* if shifting more that 3 bytes */
  if (shCount >= 24)
    {
      shiftLLong1 (left, result, shCount);
    }
  else if (shCount >= 16)
    {
      shiftLLong2 (left, result, shCount);
    }
  else if (shCount >= 8)
    {
      shiftLLong3 (left, result, shCount);
    }
  else
    {
      shiftLLong4 (left, result, shCount);
    }
  
  if (maskedtopbyte)
    {
      bool in_a = (result->aop->type == AOP_REG && result->aop->aopu.aop_reg[1]->rIdx == A_IDX);
      bool needpull = false;
      if (!in_a)
        {
          needpull = pushRegIfUsed (m6502_reg_a);
          loadRegFromAop (m6502_reg_a, result->aop, 1);
        }
      emit6502op ("and", IMMDFMT, topbytemask);
      if (!in_a)
        {
          storeRegToAop (m6502_reg_a, result->aop, 1);
          pullOrFreeReg (m6502_reg_a, needpull);
        }
    }
}

/**************************************************************************
 * genLeftShiftLiteral - left shifting by known count
 *************************************************************************/
static void
genLeftShiftLiteral (operand * left, operand * result, int shCount)
{
  bool restore_x = false;
  int size, offset;

  emitComment (TRACEGEN, __func__);

  size = AOP_SIZE (result);
  emitComment (TRACEGEN|VVDBG, "  %s - result size=%d, left size=%d",
               __func__, size, AOP_SIZE (left));

  if (shCount == 0)
    {
      genCopy (result, left);
    }
  else if (shCount >= (size * 8))
    {
      for(offset=0;offset<size; offset++)
        storeConstToAop (0, AOP (result), offset);
    } 
  else
    {
      if(AOP_TYPE(left)==AOP_SOF || AOP_TYPE(result)==AOP_SOF)
	restore_x=storeRegTempIfUsed(m6502_reg_x);

      switch (size)
	{
	case 1:
	  genlsh8 (result, left, shCount);
	  break;
	case 2:
	  genlsh16 (result, left, shCount);
	  break;
	case 4:
	  genlsh32 (result, left, shCount);
	  break;
	default:
	  emitcode("ERROR", "%s: Invalid operand size %d", __func__, size);
	  break;
	}
      loadOrFreeRegTemp(m6502_reg_x, restore_x);
    }
}

/**************************************************************************
 * genLeftShift - generates code for left shifting
 *************************************************************************/
void
m6502_genLeftShift (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  symbol *tlbl, *tlbl1;
  reg_info *countreg = NULL;
  bool restore_a = false;
  bool restore_y = false;
  bool x_in_regtemp = false;

  emitComment (TRACEGEN, __func__);

  aopOp (right, ic);
  aopOp (left, ic);
  aopOp (result, ic);

  printIC(ic);

  sym_link *resulttype = operandType (result);
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  /* if the shift count is known then do it
     as efficiently as possible */
  if (AOP_TYPE (right) == AOP_LIT &&
      (getSize (operandType (result)) == 1 || getSize (operandType (result)) == 2 || getSize (operandType (result)) == 4))
    {
      int shCount = (int) ulFromVal (AOP (right)->aopu.aop_lit);
      genLeftShiftLiteral (left, result, shCount);
      goto release;
    }

  /* shift count is unknown then we have to form
     a loop get the loop count in X : Note: we take
     only the lower order byte since shifting
     more that 32 bits make no sense anyway, ( the
     largest size of an object can be only 32 bits ) */

  // TODO
#if 0
  if (sameRegs (AOP (right), AOP (result)) || regsInCommon (right, result) || IS_AOP_XA (AOP (result)) || isOperandVolatile (result, false))
    aopResult = forceZeropageAop (AOP (result), sameRegs (AOP (left), AOP (result)));
#endif

  size = AOP_SIZE (result);
  tlbl = safeNewiTempLabel (NULL);
  tlbl1 = safeNewiTempLabel (NULL);

  if (!m6502_reg_a->isDead && !IS_AOP_WITH_A (AOP (result)))
    {
      storeRegTemp(m6502_reg_a, true);
      restore_a=true;
    }

  /* find a count register */
  if (m6502_reg_y->isDead && !IS_AOP_WITH_Y (AOP (result)) && !IS_AOP_WITH_Y (AOP (left)))
    countreg = m6502_reg_y;
  else if (m6502_reg_x->isDead && !IS_AOP_WITH_X (AOP (result)) && !IS_AOP_WITH_X (AOP (left))
           && AOP_TYPE(left)!=AOP_SOF && AOP_TYPE(result)!=AOP_SOF )
    countreg = m6502_reg_x;
  else //if (!IS_AOP_WITH_Y (AOP (result)) && !IS_AOP_WITH_Y (AOP (left)))
    {
      // Y is live
      //	emitcode("ERROR", "%s: countreg is null", __func__);
      if(!m6502_reg_y->isDead && !IS_AOP_WITH_Y (AOP (result)))
        {
	  storeRegTemp(m6502_reg_y, true);
	  restore_y=true;
        }
      countreg = m6502_reg_y;
    }

  bool lsb_in_a = ( IS_AOP_XA (AOP (result)) || IS_AOP_XA (AOP (left)) || size==1);
  bool msb_in_x = lsb_in_a && AOP_TYPE(result)!=AOP_DIR;

  if(lsb_in_a)
    {
      if(size==2)
	{
	  loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  if(msb_in_x)
	    {
	      storeRegTempAlways(m6502_reg_x, true);
	      dirtyRegTemp (getLastTempOfs());
	      x_in_regtemp = true;
	    }
	  else
	    storeRegToAop (m6502_reg_x, AOP(result) , 1);
	}
    }
  else if (!sameRegs (AOP (left), AOP (result)))
    {
      for (offset=0; offset<size-1; offset++)
	transferAopAop (AOP (left), offset, AOP (result), offset);
    }

  int a_loc = ( lsb_in_a )? 0 : size-1;

  emitComment (TRACEGEN|VVDBG, "%s: load countreg", __func__);
  if(IS_AOP_WITH_A (AOP (right)))
    {
      loadRegFromAop (countreg, AOP (right), 0);
      loadRegFromAop (m6502_reg_a, AOP (left), a_loc);
    }
  else
    {
      loadRegFromAop (m6502_reg_a, AOP (left), a_loc);
      loadRegFromAop (countreg, AOP (right), 0);
    }

  m6502_useReg (countreg);
  if(IS_AOP_XA(AOP(right)))
    m6502_freeReg(m6502_reg_xa);

  emitCmp(countreg, 0);
  emitBranch ("beq", tlbl1);

  // FIXME: find a good solution for this
  //  if(IS_AOP_WITH_A (AOP (right)) && sameRegs (AOP (left), AOP (result)) )
  //    loadRegFromAop (m6502_reg_a, AOP (left), a_loc);

  safeEmitLabel (tlbl); // loop label

  if(lsb_in_a)
    {
      rmwWithReg ("asl", m6502_reg_a);
      if(size==2)
        {
	  if(msb_in_x)
	    emitRegTempOp("rol", getLastTempOfs() );
	  else
	    rmwWithAop ("rol", AOP (result), 1);
        }
    }
  else
    {
      rmwWithAop ("asl", AOP (result), 0);
      for (offset = 1; offset < size-1; offset++)
	rmwWithAop ("rol", AOP (result), offset);
      rmwWithReg ("rol", m6502_reg_a);
    }

  rmwWithReg("dec", countreg);
  emit6502op("bne", "%05d$", safeLabelNum (tlbl));

  if (x_in_regtemp)
    loadRegTemp(m6502_reg_x);

  safeEmitLabel (tlbl1); // end label

  if (maskedtopbyte)
    {
      if(lsb_in_a && size==2)
        {
          m6502_pushReg (m6502_reg_a, false);
	  if(msb_in_x)
	    transferRegReg(m6502_reg_x, m6502_reg_a, false);
	  else
	    loadRegFromAop (m6502_reg_a, AOP (left), 1);

          emit6502op ("and", IMMDFMT, topbytemask);

	  if(msb_in_x)
	    transferRegReg(m6502_reg_a, m6502_reg_x, false);
	  else
	    storeRegToAop (m6502_reg_a, AOP(result) , 1);
          m6502_pullReg (m6502_reg_a);
        }
      else
        emit6502op ("and", IMMDFMT, topbytemask);
    }

  if(lsb_in_a)
    {
      storeRegToAop (m6502_reg_a, AOP(result) , 0);
      if(size==2)
        {
	  if(msb_in_x)
	    storeRegToAop (m6502_reg_x, AOP(result) , 1);
        }
    }
  else
    storeRegToAop (m6502_reg_a, AOP(result) , size-1);

  // After loop, countreg is always 0
  m6502_dirtyReg(countreg);
  countreg->isLitConst = 1;
  countreg->litConst = 0;

  if(restore_y)
    loadRegTemp(m6502_reg_y);
  if(restore_a)
    loadRegTemp(m6502_reg_a);

 release:
  freeAsmop (right, NULL);
  freeAsmop (left, NULL);
  freeAsmop (result, NULL);
}

