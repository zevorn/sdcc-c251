;--------------------------------------------------------------------------
; crtxinit.asm - copy MCS251 XINIT to the flat 24-bit XISEG
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

	.area GSINIT3 (CODE)

; DR4 is the source pointer, DPX is the destination pointer, and DR12 is a
; 24-bit byte count (the fourth byte remains zero).  Native indirect moves
; and INC operate on the complete flat address, including 64 KiB carries.
__mcs51_genXINIT::
	mov	dptr,#s_XINIT
	mov	dpxl,#(s_XINIT >> 16)
	mov	dr4,dpx
	mov	dptr,#s_XISEG
	mov	dpxl,#(s_XISEG >> 16)
	mov	dr12,#0
	mov	wr14,#l_XINIT
	mov	r13,#(l_XINIT >> 16)
	cmp	dr12,#0
	je	00002$
00001$:
	mov	a,@dr4
	mov	@dpx,a
	inc	dr4
	inc	dpx
	dec	dr12
	jne	00001$
00002$:
