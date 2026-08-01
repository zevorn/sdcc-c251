;--------------------------------------------------------------------------
; crtxclear.asm - clear MCS251 flat 24-bit PSEG and XSEG
;
; Copyright (c) 2026 Chao Liu
;
; This library is free software; you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by the
; Free Software Foundation; either version 2, or (at your option) any
; later version.
;
; As a special exception, if you link this library with other files, some
; of which are compiled with SDCC, to produce an executable, this library
; does not by itself cause the resulting executable to be covered by the
; GNU General Public License.
;--------------------------------------------------------------------------

	.area CSEG    (CODE)
	.area GSINIT0 (CODE)
	.area GSINIT1 (CODE)
	.area GSINIT2 (CODE)
	.area GSINIT3 (CODE)
	.area GSINIT4 (CODE)
	.area GSINIT5 (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)

	.area GSINIT4 (CODE)

; DPX is the destination pointer and DR12 is a 24-bit byte count.  Keeping
; PSEG and XSEG as separate loops preserves the inherited linker layout
; while removing its 16-bit address and length assumptions.
__mcs51_genXRAMCLEAR::
	mov	dptr,#s_PSEG
	mov	dpxl,#(s_PSEG >> 16)
	mov	dr12,#0
	mov	wr14,#l_PSEG
	mov	r13,#(l_PSEG >> 16)
	cmp	dr12,#0
	je	00002$
	clr	a
00001$:
	mov	@dpx,a
	inc	dpx
	dec	dr12
	jne	00001$
00002$:
	mov	dptr,#s_XSEG
	mov	dpxl,#(s_XSEG >> 16)
	mov	dr12,#0
	mov	wr14,#l_XSEG
	mov	r13,#(l_XSEG >> 16)
	cmp	dr12,#0
	je	00004$
	clr	a
00003$:
	mov	@dpx,a
	inc	dpx
	dec	dr12
	jne	00003$
00004$:
