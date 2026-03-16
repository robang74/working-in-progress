# =============================================================================
#  Makefile for small static utilities built with musl-gcc
# =============================================================================

CC          := musl-gcc
CFLAGS      := -Os -Wall -static -s
LDFLAGS     := -static -s

# Extra flags per program (only if needed)
# Example: -lpthread, -lm, -lz, --fast-math, etc.
EXTRA_FLAGS_mixtrd  := -lpthread
EXTRA_FLAGS_uchaos  := -lm --fast-math
EXTRA_FLAGS_flatz   := -lz

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

# =============================================================================
#  Standard targets
# =============================================================================

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

# Optional: rebuild everything if Makefile changes
$(TARGETS): Makefile
