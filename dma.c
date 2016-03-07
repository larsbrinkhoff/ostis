#include "common.h"
#include "cpu.h"
#include "mmu.h"
#include "fdc.h"
#include "state.h"
#include "diag.h"

HANDLE_DIAGNOSTICS(dma);

#define DMABASE 0xff8600
#define DMASIZE 16


#define DMA_READ  0
#define DMA_WRITE 1

#define STATUS_ERROR        1
#define STATUS_NO_ERROR     0
#define STATUS_SEC_ZERO     0
#define STATUS_SEC_NOT_ZERO 2
#define STATUS_NO_FDC_DRQ   0
#define STATUS_FDC_DRQ      4

static BYTE dma_addr_low = 0;
static BYTE dma_addr_med = 0;
static BYTE dma_addr_high = 0;
static WORD dma_mode = 0;
static BYTE dma_sector_reg = 0;
static BYTE dma_status = 0;
static int dma_direction = DMA_READ;
static int dma_activate = 0;
static int word_write = 0;

#define MODE_FDCS  ((~dma_mode)&(1<<3))
#define MODE_HDCS  (dma_mode&(1<<3))
#define MODE_FDC   ((~dma_mode)&(1<<4))
#define MODE_SEC   (dma_mode&(1<<4))
#define MODE_HDRQ  ((~dma_mode)&(1<<7))
#define MODE_FDRQ  (dma_mode&(1<<7))
#define MODE_READ  ((~dma_mode)&(1<<8))
#define MODE_WRITE (dma_mode&(1<<8))

#define MODE_FDC_REG (dma_mode&0x6)
#define MODE_FDC_STATUS  0
#define MODE_FDC_CONTROL 0
#define MODE_FDC_TRACK   0
#define MODE_FDC_SECTOR  0

#define FDC_REG_CONTROL 0
#define FDC_REG_SECTOR  2


static LONG dma_address()
{
  return (dma_addr_high<<16)|(dma_addr_med<<8)|dma_addr_low;
}

static void dma_reset(int new_direction)
{
  dma_direction = new_direction;
  dma_sector_reg = 0;
  dma_activate = 0;
  dma_status = STATUS_NO_ERROR | STATUS_SEC_ZERO | STATUS_NO_FDC_DRQ;
}

static void dma_handle_mode()
{
  if(dma_direction == DMA_READ && MODE_WRITE) {
    TRACE("dma_reset: WRITE");
    dma_reset(DMA_WRITE);
  }
  if(dma_direction == DMA_WRITE && MODE_READ) {
    TRACE("dma_reset: READ");
    dma_reset(DMA_READ);
  }
}

BYTE dma_read_byte(LONG addr)
{
  switch(addr) {
  case 0xff8604:
    if(MODE_SEC) {
      return 0;
    } else {
      return 0xff;
    }
  case 0xff8605:
    if(MODE_SEC) {
      return dma_sector_reg;
    } else {
      if(MODE_FDCS == 0) {
        TRACE("Calling fdc_get_register(%d)", MODE_FDC_REG>>1);
        return fdc_get_register(MODE_FDC_REG>>1);
      } else {
        DEBUG("HD not implemented");
      }
      return 0;
    }
  case 0xff8606:
    return 0;
  case 0xff8607:
    return dma_status;
  case 0xff8609:
    return dma_addr_high;
  case 0xff860b:
    return dma_addr_med;
  case 0xff860d:
    return dma_addr_low;
  }

  return 0xff;
}

WORD dma_read_word(LONG addr)
{
  return (dma_read_byte(addr)<<8)|dma_read_byte(addr+1);
}

LONG dma_read_long(LONG addr)
{
  return (dma_read_word(addr)<<16)|dma_read_word(addr+1);
}

void dma_write_byte(LONG addr, BYTE data)
{
  switch(addr) {
  case 0xff8604:
    /* Upper byte is ignored */
    break;
  case 0xff8605:
    if(MODE_SEC) {
      TRACE("Setting dma_sector_reg == %d", data);
      dma_sector_reg = data;
      dma_activate = 1;
    } else {
      if(MODE_FDCS == 0 && MODE_FDRQ) {
        TRACE("Calling fdc_set_register(%d, %02x)", MODE_FDC_REG>>1, data);
        fdc_set_register(MODE_FDC_REG>>1, data);
      }
    }
    break;
  case 0xff8606:
    dma_mode = (dma_mode&0xff)|(data<<8);
    if(!word_write) {
      dma_handle_mode();
    }
    break;
  case 0xff8607:
    dma_mode = (dma_mode&0xff00)|data;
    if(!word_write) {
      dma_handle_mode();
    }
    break;
  case 0xff8609:
    dma_addr_high = data&0x3f; /* Top 2 bits unused */
    TRACE("(HIGH) DMA address now == %06x", dma_address());
    break;
  case 0xff860b:
    dma_addr_med = data;
    TRACE("(MED)  DMA address now == %06x", dma_address());
    break;
  case 0xff860d:
    dma_addr_low = data;
    TRACE("(LOW)  DMA address now == %06x", dma_address());
    break;
  }
}

void dma_write_word(LONG addr, WORD data)
{
  word_write = 1;
  dma_write_byte(addr, (data&0xff00)>>8);
  dma_write_byte(addr+1, data&0xff);
  word_write = 0;
  if(addr == 0xff8606) {
    dma_handle_mode();
  }
}

void dma_write_long(LONG addr, LONG data)
{
  dma_write_word(addr, (data&0xffff0000)>>16);
  dma_write_word(addr+2, data&0xffff);
}

static int dma_state_collect(struct mmu_state *state)
{
  state->size = 0;
  return STATE_VALID;
}

static void dma_state_restore(struct mmu_state *state)
{
}


void dma_do_interrupts(struct cpu *cpu)
{
  if(dma_activate) {
    if((MODE_FDCS == 0) && dma_direction == DMA_READ) {
      fdc_prepare_read(dma_address(), dma_sector_reg);
    }
    dma_activate = 0;
  }
}

void dma_init()
{
  struct mmu *dma;

  dma = (struct mmu *)malloc(sizeof(struct mmu));
  if(!dma) {
    return;
  }

  dma->start = DMABASE;
  dma->size = DMASIZE;
  memcpy(dma->id, "DMA0", 4);
  dma->name = strdup("DMA");
  dma->read_byte = dma_read_byte;
  dma->read_word = dma_read_word;
  dma->read_long = dma_read_long;
  dma->write_byte = dma_write_byte;
  dma->write_word = dma_write_word;
  dma->write_long = dma_write_long;
  dma->state_collect = dma_state_collect;
  dma->state_restore = dma_state_restore;
  dma->diagnostics = dma_diagnostics;

  dma_reset(DMA_READ);

  mmu_register(dma);
}

