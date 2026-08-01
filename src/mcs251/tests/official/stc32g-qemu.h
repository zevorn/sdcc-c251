#ifndef SDCC_MCS251_TEST_STC32G_QEMU_H
#define SDCC_MCS251_TEST_STC32G_QEMU_H

/* Minimal, independently written register view for peripherals implemented
   by QEMU's stc32g144k246-evb.  This is deliberately a test fixture, not a
   claim of complete STC32G144K246 device-header coverage. */

__sfr __at (0x80) P0;
__sfr __at (0x86) DPUST;
__sfr __at (0x88) TCON;
__sfr __at (0x89) TMOD;
__sfr __at (0x8a) TL0;
__sfr __at (0x8b) TL1;
__sfr __at (0x8c) TH0;
__sfr __at (0x8d) TH1;
__sfr __at (0x8e) AUXR;
__sfr __at (0x8f) INTCLKO;
__sfr __at (0x90) P1;
__sfr __at (0x91) P1M1;
__sfr __at (0x92) P1M0;
__sfr __at (0x93) P0M1;
__sfr __at (0x94) P0M0;
__sfr __at (0x95) P2M1;
__sfr __at (0x96) P2M0;
__sfr __at (0x98) SCON;
__sfr __at (0x99) SBUF;
__sfr __at (0xa0) P2;
__sfr __at (0xa8) IE;
__sfr __at (0xb0) P3;
__sfr __at (0xb1) P3M1;
__sfr __at (0xb2) P3M0;
__sfr __at (0xb3) P4M1;
__sfr __at (0xb4) P4M0;
__sfr __at (0xba) P_SW2;
__sfr __at (0xc0) P4;
__sfr __at (0xd8) DPUOP;
__sfr __at (0xc8) P5;
__sfr __at (0xc9) P5M1;
__sfr __at (0xca) P5M0;
__sfr __at (0xe1) P7M1;
__sfr __at (0xe2) P7M0;
__sfr __at (0xe8) P6;
__sfr __at (0xed) DMAIR;
__sfr __at (0xcb) P6M1;
__sfr __at (0xcc) P6M0;
__sfr __at (0xf8) P7;

__xdata __at (0x7efea0) volatile unsigned char TM0PS;
__xdata __at (0x7efea1) volatile unsigned char TM1PS;

#define TCON_IT0 0x01
#define TCON_IE0 0x02
#define TCON_IT1 0x04
#define TCON_IE1 0x08
#define TCON_TR0 0x10
#define TCON_TF0 0x20
#define TCON_TR1 0x40
#define TCON_TF1 0x80

#define IE_EX0 0x01
#define IE_ET0 0x02
#define IE_EX1 0x04
#define IE_ET1 0x08
#define IE_ES  0x10
#define IE_EA  0x80

#define SCON_RI  0x01
#define SCON_TI  0x02
#define SCON_REN 0x10

#define P_SW2_EAXFR 0x80

#endif
