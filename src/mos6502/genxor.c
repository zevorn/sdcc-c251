/*-------------------------------------------------------------------------
  genxor.c - source file for XOR code generation for the MOS6502

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

#define NOP_MASK   0x00

/**************************************************************************
 * genXor - code for Exclusive or
 *************************************************************************/
void
m6502_genXor (iCode * ic, iCode * ifx)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset = 0;
  bool needpulla = false;
  bool isLit = false;
  unsigned long long lit = 0ull;
  unsigned int bytemask;
  int bitpos = -1;

  emitComment (TRACEGEN, __func__);

  aopOp (left, ic);
  aopOp (right, ic);
  aopOp (result, ic);
  printIC(ic);

  /* force literal on the right and reg on the left */
  if (AOP_TYPE (left) == AOP_LIT || AOP_TYPE (right) == AOP_REG)
    {
      if(!IS_AOP_WITH_A (AOP (left)))
	{
	  operand *tmp = right;
	  right = left;
	  left = tmp;
	}
    }

  size = (AOP_SIZE (left) >= AOP_SIZE (right)) ? AOP_SIZE (left) : AOP_SIZE (right);

  isLit = (AOP_TYPE (right) == AOP_LIT);

  if (isLit)
    {
      lit = ullFromVal (AOP (right)->aopu.aop_lit);
      lit &= litmask(size);
      bitpos = isLiteralBit (lit) - 1;
      emitComment (TRACEGEN|VVDBG, "  %s: lit=%04x bitpos=%d", __func__, lit, bitpos);
    }

  // test A for flags only
  if (AOP_TYPE (result) == AOP_CRY && IS_AOP_A (AOP (left)))
    {
      emitComment (TRACEGEN|VVDBG, "  %s: test A for flags", __func__);

      if( isLit && lit==NOP_MASK ) 
	{
	  emitCmp(m6502_reg_a, 0);
	  genIfxJump (ifx, "z");
	  goto release;
	}
    }

  // test for flags only (general case)
  if (AOP_TYPE (result) == AOP_CRY)
    {
      symbol *tlbl = safeNewiTempLabel (NULL);

      needpulla = storeRegTempIfSurv (m6502_reg_a);

      for(offset=0; offset<size; offset++)
	{
          bytemask = (isLit) ? (lit >> (offset * 8)) & 0xff : 0x100;

	  loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  if (bytemask != NOP_MASK)
	    accopWithAop ("eor", AOP (right), offset);

	  if (offset<size-1)
	    emitBranch ("bne", tlbl);
        }
      // FIXME: check bug1875933.c
      m6502_freeReg (m6502_reg_a);
      safeEmitLabel (tlbl);

      // TODO: better way to preserve flags?
      if (ifx)
	{
	  loadRegTempNoFlags (m6502_reg_a, needpulla);
	  genIfxJump (ifx, "z");
	}
      else
	{
	  if (needpulla)
            loadRegTemp (NULL);
	}
      goto release;
    }

  size = AOP_SIZE (result);

  unsigned int bmask0 = (isLit) ? ((lit >> (0 * 8)) & 0xff) : 0x100;
  unsigned int bmask1 = (isLit) ? ((lit >> (1 * 8)) & 0xff) : 0x100;

  if(IS_AOP_XA(AOP(result)))
    {
      emitComment (TRACEGEN|VVDBG, "  %s: XA", __func__);

      //    if(/*AOP_TYPE(right)==AOP_LIT &&*/ AOP_TYPE(left)!=AOP_SOF)
      {
        if (IS_AOP_A(AOP(left)))
          storeConstToAop(0x00, AOP(result), 1);
        else if (bmask1==NOP_MASK)
          transferAopAop(AOP(left), 1, AOP(result), 1);
        else if(IS_AOP_XA(AOP(left)) && m6502_reg_x->isLitConst && m6502_reg_x->litConst==0)
          transferAopAop(AOP(right), 1, AOP(result), 1);
        else
          {
            if(IS_AOP_XA(AOP(left)))
              {
		fastSaveA();
		needpulla=true;
	      }
            loadRegFromAop (m6502_reg_a, AOP (left), 1);
            accopWithAop ("eor", AOP (right), 1);
            storeRegToAop (m6502_reg_a, AOP (result), 1);          
          }

	{
          if(needpulla)
            fastRestoreA();
          else
	    {
	      loadRegFromAop (m6502_reg_a, AOP (left), 0);
	    }
	  if(bmask0!=NOP_MASK)
            accopWithAop ("eor", AOP (right), 0);
	}
        goto release;
      }
    }


  needpulla = fastSaveAIfSurv();
  //  if(needpulla)
  //    m6502_reg_a->isDead=true;

  emitComment (TRACEGEN|VVDBG, "  %s: general path", __func__);

  for(offset=0; offset<size; offset++)
    {
      bytemask = (isLit) ? ((lit >> (offset * 8)) & 0xff) : 0x100;

      if ( bytemask==NOP_MASK )
	{
	  transferAopAop(AOP(left), offset, AOP(result), offset);
	}
      else
	{
	  loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  accopWithAop ("eor", AOP (right), offset);
	  storeRegToAop (m6502_reg_a, AOP (result), offset);
	}
    }


  fastRestoreOrFreeA(needpulla);

 release:
  freeAsmop (left, NULL);
  freeAsmop (right, NULL);
  freeAsmop (result, NULL);
}

