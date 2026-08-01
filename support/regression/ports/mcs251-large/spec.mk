# MCS251 large-model regression lane.  Keep its objects and results separate
# from the default small-model lane while sharing the QEMU harness.

MCS251_MODEL = large
include $(PORTS_DIR)/mcs251/spec.mk
