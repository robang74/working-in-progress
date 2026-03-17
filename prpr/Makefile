# =============================================================================
#  Makefile for small static utilities built with musl-gcc
# =============================================================================

CC      := musl-gcc
CFLAGS  := -static -s -flto -Os -Wall
LDFLAGS := -static -s -flto
STRIP   := strip --strip-all

STRIP_FLAGS += --remove-section=.comment --remove-section=.note*
CFLAGS  += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

# Extra flags per program (only if needed)
# Example: -lpthread, -lm, -lz, --fast-math, etc.
EXTRA_FLAGS_mixtrd    := -lpthread -lutil
EXTRA_FLAGS_uchaos    := -lm --fast-math
EXTRA_FLAGS_flatz     := -I../minz/amalgamation/ ../minz/amalgamation/miniz.c
EXTRA_FLAGS_flatz     += -DMINIZ_NO_INFLATE -DMINIZ_NO_ZIP -DMINIZ_NO_ARCHIVE -DMINIZ_NO_STDIO

EXTRA_FLAGS_ALL := $(foreach t,$(TARGETS),$(EXTRA_FLAGS_$(t)))

# List of final executables (stem = basename without .c)
TARGETS :=  \
  uchaos    \
  prnt16    \
  strn64    \
  strsum    \
  mixtrd    \
  aprx10    \
  flatz     \
  prpr      \
  prpx

SRCS := $(addsuffix .c, $(TARGETS))
OBJS := $(addsuffix .o, $(TARGETS))

EXTRA_SRCS  := $(filter %.c, $(EXTRA_FLAGS_flatz))
TARGETS_ALL := uchaosbox $(TARGETS)

uchaosbox: uchaosbox.c $(OBJS)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS_ALL) $(LDFLAGS) $(EXTRA_SRCS) $(OBJS) -o $@ $<
	@du -k $@

# The magic: it redefines 'main' for each module to be 'target_main'
# This avoids "multiple definition of 'main'" errors during linking.
$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS_$*) $(LDFLAGS) -Dmain=$*_main $< -c

# Pattern: target depends on corresponding .c file
$(TARGETS): % : %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS_$*) $(LDFLAGS) -o $@ $<
	$(STRIP) $(STRIP_FLAGS) $@

# =============================================================================
#  Standard targets
# =============================================================================

.PHONY: all clean

all: $(TARGETS_ALL)
	@du -ks $(TARGETS) | sort -n

clean:
	rm -f $(TARGETS_ALL) $(addsuffix .o, $(TARGETS_ALL))

# Optional: rebuild everything if Makefile changes
$(TARGETS): Makefile
