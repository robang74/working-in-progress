# =============================================================================
#  Makefile for small static utilities built with musl-gcc
# =============================================================================

CC      := musl-gcc
CFLAGS  := -static -s -flto -Os -Wall
LDFLAGS := -static -s -flto

CFLAGS  += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

# Extra flags per program (only if needed)
# Example: -lpthread, -lm, -lz, --fast-math, etc.
EXTRA_FLAGS_mixtrd  := -lpthread
EXTRA_FLAGS_uchaos  := -lm --fast-math
EXTRA_FLAGS_flatz   := -I../minz/amalgamation/ ../minz/amalgamation/miniz.c
EXTRA_FLAGS_flatz   += -DMINIZ_NO_INFLATE -DMINIZ_NO_ZIP -DMINIZ_NO_ARCHIVE -DMINIZ_NO_STDIO

# List of final executables (stem = basename without .c)
TARGETS :=  \
  uchaos    \
  prnt16    \
  strn64    \
  strsum    \
  mixtrd    \
  flatz     \
  modaprx10 \
  prpr      \
  prpx

# Pattern: target depends on corresponding .c file
$(TARGETS): % : %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS_$*) $(LDFLAGS) -o $@ $<
	strip --strip-all --remove-section=.comment --remove-section=.note* $@

# =============================================================================
#  Standard targets
# =============================================================================

.PHONY: all clean

all: $(TARGETS)

# flatz: flatz.c ../minz/miniz_export.h

clean:
	rm -f $(TARGETS)

# Create dummy miniz_export.h if missing (needed by some miniz versions/forks)
../minz/miniz_export.h:
	@echo "/* Dummy miniz_export.h for static musl build */" > $@
	@echo "#ifndef MINIZ_EXPORT" >> $@
	@echo "#define MINIZ_EXPORT" >> $@
	@echo "#endif" >> $@

# Optional: rebuild everything if Makefile changes
$(TARGETS): Makefile
