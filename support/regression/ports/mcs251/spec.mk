# Regression-test specification for SDCC MCS251 on the STC32G QEMU machine.

PORT_BASE = mcs251
MCS251_MODEL ?= small
MCS251_STACK_AUTO ?= 0
MCS251_FLASH_BASE ?= 0xfc2800
MCS251_RESET_PC ?= 0xff0000

QEMU_MCS251 ?= $(HOME)/oss/qemu/builds/build-mcs251/qemu-system-mcs251
MCS251_LIBRARY_SUFFIX = $(if $(filter 1,$(MCS251_STACK_AUTO)),-stack-auto,)
MCS251_LIBDIR ?= $(top_builddir)/device/lib/build/mcs251-$(MCS251_MODEL)$(MCS251_LIBRARY_SUFFIX)

EMU = $(PYTHON) $(PORTS_DIR)/mcs251/run-qemu.py --qemu $(QEMU_MCS251)
EMU_PORT_FLAG =
EMU_FLAGS =
EMU_INPUT =

ifndef CROSSCOMPILING
  SDCCFLAGS += --nostdinc -I$(INC_DIR)/mcs51 -I$(top_srcdir)
  LINKFLAGS += --nostdlib -L$(MCS251_LIBDIR)
endif

SDCCFLAGS += -mmcs251 --model-$(MCS251_MODEL) --less-pedantic
ifeq ($(MCS251_STACK_AUTO),1)
  SDCCFLAGS += --stack-auto
endif
# MCS251 has a complete stack-based variadic ABI.  The regression harness
# disables variadic tests by default for historical targets; exercise them
# for this port instead.  MCS251_NO_VARARGS is a diagnostic escape hatch for
# QEMU revisions affected by the known signed @SPX+dis16 decoding bug; it is
# deliberately off by default so the normal lane still covers varargs.
ifneq ($(MCS251_NO_VARARGS),1)
  SDCCFLAGS := $(filter-out -DNO_VARARGS,$(SDCCFLAGS))
endif
LINKFLAGS += --code-loc $(MCS251_RESET_PC) \
	-Wl-b\ GSINIT0=$(MCS251_FLASH_BASE) \
	mcs251.lib libsdcc.lib liblong.lib libint.lib libfloat.lib liblonglong.lib

OBJEXT = .rel
# QEMU selects its Intel-HEX loader from the conventional .hex suffix.
BINEXT = .hex

.PRECIOUS: $(PORT_CASES_DIR)/%$(OBJEXT)

EXTRAS = $(PORT_CASES_DIR)/testfwk$(OBJEXT) \
	$(PORT_CASES_DIR)/support$(OBJEXT)
include $(srcdir)/fwk/lib/spec.mk

SPEC_LIB = $(PORTS_DIR)/mcs251/fwk.lib

_clean:
