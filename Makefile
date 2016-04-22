CC=gcc
CFLAGS_BASE=`sdl2-config --cflags` `pkg-config SDL2_image --cflags` -I. -I.. -Wall -Werror -Wno-unused-function
LDFLAGS_BASE=

CFLAGS=$(CFLAGS_EXTRA) $(CFLAGS_BASE)
LDFLAGS=$(LDFLAGS_EXTRA) $(LDFLAGS_BASE)
export CFLAGS
export LDFLAGS

YACC=yacc -d
ifeq ($(shell uname -o),Cygwin)
	LEX=flex
else
	LEX=lex
endif
AR=ar
RANLIB=ranlib

PARSER=expr
PARSERFILE=expr.y expr.l
PARSERSRC=expr.tab.c lex.expr.c
PARSEROBJ=$(PARSERSRC:.c=.o)

SRC=mmu.c mmu_fallback.c ram.c rom.c cpu.c cartridge.c psg.c mfp.c shifter.c screen.c \
    midi.c ikbd.c dma.c fdc.c hdc.c rtc.c floppy.c floppy_st.c floppy_msa.c floppy_stx.c event.c \
    state.c prefs.c cprint.c diag.c acia.c glue.c clock.c flipflop.c
SRC_TEST=tests/test_main.c
OBJ=$(SRC:.c=.o)

EMU_SRC=main.c $(SRC)
EMU_OBJ=$(EMU_SRC:.c=.o)
EMU_TEST_SRC=$(EMU_SRC) $(SRC_TEST)
EMU_TEST_OBJ=$(EMU_TEST_SRC:.c=.o)

LIBCPU=cpu/libcpu.a
LIBCPUINSTR=cpuinstr/libcpuinstr.a
LIBDEBUG=debug/libdebug.a
LIBTESTS=tests/libtests.a

DEPS = $(EMU_OBJ) $(LIBCPU) $(LIBDEBUG) $(PARSEROBJ) $(LIBCPUINSTR)
DEPS_TEST = $(EMU_TEST_OBJ) $(LIBCPU) $(LIBDEBUG) $(PARSEROBJ) $(LIBTESTS) $(LIBCPUINSTR)

LIB=$(LIBCPU) $(LIBCPUINSTR) $(LIBDEBUG) `sdl2-config --libs` `pkg-config SDL2_image --libs` 

-include config.mk

all:	default

-include $(EMU_SRC:.c=.d)

%.o:	%.c Makefile cpu/cpu.mk debug/debug.mk config.mk
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

default:
	$(MAKE) ostis CFLAGS_EXTRA="-O3"

gdb:
	$(MAKE) ostis-gdb CFLAGS_EXTRA="-ggdb"

prof:
	$(MAKE) ostis-prof CFLAGS_EXTRA="-pg -O3 -fno-inline-small-functions" LDFLAGS_EXTRA="-pg"

test:
	$(MAKE) ostis-test CFLAGS_EXTRA="-ggdb -DTEST_BUILD"

ostis ostis-gdb ostis-prof: $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(EMU_OBJ) $(PARSEROBJ) $(LIB)

ostis-test:	$(DEPS_TEST)
	$(CC) $(LDFLAGS) -o $@ $(EMU_TEST_OBJ) $(PARSEROBJ) $(LIB) $(LIBTESTS)

$(PARSEROBJ):	$(PARSERSRC)

$(PARSERSRC): $(PARSERFILE)
	$(LEX) $(PARSER).l
	$(YACC) -b $(PARSER) --name-prefix=$(PARSER) $(PARSER).y

include cpu/cpu.mk
include cpuinstr/cpuinstr.mk
include debug/debug.mk
include tests/tests.mk

TAGS: $(EMU_SRC) $(CPU_SRC) $(DEBUG_SRC) $(CPUINSTR_SRC)
	etags $^

clean::
	rm -f *.o *.d *~ $(PARSERSRC) expr.tab.h ostis ostis-gdb ostis-test ostis-prof TAGS
