#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "prefs.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

static LONG ROMSIZE;
static LONG ROMBASE;

#define ROMSIZE_NEW 262144
#define ROMBASE_NEW 0xe00000
#define ROMSIZE_OLD 196608
#define ROMBASE_OLD 0xfc0000

#define ROMSIZE2 8
#define ROMBASE2 0

static BYTE *memory;
static BYTE *memory2;

HANDLE_DIAGNOSTICS(rom)

static BYTE *real(LONG addr)
{

  if(addr < ROMBASE)
    return memory2+addr-ROMBASE2;
  else
    return memory+addr-ROMBASE;
}

static BYTE rom_read_byte(LONG addr)
{
  return *(real(addr));
}

static WORD rom_read_word(LONG addr)
{
  return (rom_read_byte(addr)<<8)|rom_read_byte(addr+1);
}

static int rom_state_collect(struct mmu_state *state)
{
  if(!strcmp("ROM0", state->id)) {
    state->size = ROMSIZE;
    state->data = xmalloc(state->size);
    if(state->data == NULL)
      return STATE_INVALID;
    memcpy(state->data, memory, state->size);
  } else {
    state->size = ROMSIZE2;
    state->data = xmalloc(state->size);
    if(state->data == NULL)
      return STATE_INVALID;
    memcpy(state->data, memory2, state->size);
  }
  return STATE_VALID;
}

static void rom_state_restore(struct mmu_state *state)
{
  long size;
  
  size = state->size;
  if(!strcmp("ROM0", state->id)) {
    if(state->size > ROMSIZE) size = ROMSIZE;
    memcpy(memory, state->data, size);
  } else {
    if(state->size > ROMSIZE2) size = ROMSIZE2;
    memcpy(memory2, state->data, size);
  }
}

void rom_init()
{
  struct mmu *rom,*rom2;
  FILE *f;

  f = fopen(prefs.tosimage, "rb");
  if(!f) {
    FATAL("Could not open TOS image file\n");
  }

  fseek(f, 0, SEEK_END);
  ROMSIZE = ftell(f);
  rewind(f);
  switch(ROMSIZE) {
  case ROMSIZE_OLD:
    ROMBASE = ROMBASE_OLD;
    break;
  case ROMSIZE_NEW:
    ROMBASE = ROMBASE_NEW;
    break;
  default:
    FATAL("Unknown ROM image format");
  }

  memory = xmalloc(sizeof(BYTE) * ROMSIZE);
  if(!memory) {
    return;
  }
  rom = mmu_create("ROM0", "ROM");
  
  rom->start = ROMBASE;
  rom->size = ROMSIZE;
  rom->read_byte = rom_read_byte;
  rom->read_word = rom_read_word;
  rom->dev.state_collect = rom_state_collect;
  rom->dev.state_restore = rom_state_restore;
  rom->dev.diagnostics = rom_diagnostics;

  mmu_register(rom);

  if(fread(memory, 1, ROMSIZE, f) != ROMSIZE) {
    FATAL("Error reading TOS image file");
  }
  fclose(f);
  
  memory2 = xmalloc(sizeof(BYTE) * ROMSIZE2);
  if(!memory2) {
    return;
  }
  rom2 = mmu_create("ROM1", "First 8 bytes of memory");

  memcpy(memory2, memory, ROMSIZE2);

  rom2->start = ROMBASE2; /* First 8 bytes of memory is ROM */
  rom2->size = ROMSIZE2;
  rom2->read_byte = rom_read_byte;
  rom2->read_word = rom_read_word;
  rom2->dev.state_collect = rom_state_collect;
  rom2->dev.state_restore = rom_state_restore;
  rom2->dev.diagnostics = rom_diagnostics;

  mmu_register(rom2);
}
