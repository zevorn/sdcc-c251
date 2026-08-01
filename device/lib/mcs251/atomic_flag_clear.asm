;--------------------------------------------------------------------------
; atomic_flag_clear.asm - MCS251 C11 atomic flag
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

	.area HOME    (CODE)
	.area GSINIT0 (CODE)
	.area GSINIT1 (CODE)
	.area GSINIT2 (CODE)
	.area GSINIT3 (CODE)
	.area GSINIT4 (CODE)
	.area GSINIT5 (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)
	.area CSEG    (CODE)

	.area HOME    (CODE)

_atomic_flag_clear::
	mov	dpxl,b
	mov	c,ea
	clr	ea
	clr	a
	mov	@dpx,a
	mov	ea,c
	eret
