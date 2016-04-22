#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

void bcc(struct cpu *cpu, WORD op)
{
  SLONG o;
  int w;

  ENTER;
  
  w = 0;
  o = (SLONG)(SBYTE)(op&0xff);
  if(!o) {
    o = fetch_instr(cpu);
    if(o&0x8000) o |= 0xffff0000;
    o += 2;
    w = 1;
  }
  o -= 4;
  cpu_clear_prefetch();
  switch((op&0xf00)>>8) {
  case 0: /* BRA */
    ADD_CYCLE(10);
    cpu->pc += o;
    break;
  case 1: /* BSR */
    ADD_CYCLE(18);
    cpu->a[7] -= 4;
    bus_write_long(cpu->a[7], cpu->pc);
    cpu->pc += o;
    break;
  case 2: /* BHI */
    if(!CHKC && !CHKZ) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 3: /* BLS */
    if(CHKC || CHKZ) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 4: /* BCC */
    if(!CHKC) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 5: /* BCS */
    if(CHKC) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 6: /* BNE */
    if(!CHKZ) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 7: /* BEQ */
    if(CHKZ) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 8: /* BVC */
    if(!CHKV) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 9: /* BVS */
    if(CHKV) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 10: /* BPL */
    if(!CHKN) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 11: /* BMI */
    if(CHKN) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 12: /* BGE */
    if((CHKN && CHKV) || (!CHKN && !CHKV)) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 13: /* BLT */
    if((CHKN && !CHKV) || (!CHKN && CHKV)) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 14: /* BGT */
    if((CHKN && CHKV && !CHKZ) || (!CHKN && !CHKV && !CHKZ)) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  case 15: /* BLE */
    if(CHKZ || (CHKN && !CHKV) || (!CHKN && CHKV)) {
      ADD_CYCLE(10);
      cpu->pc += o;
    } else {
      ADD_CYCLE(8);
      if(w) ADD_CYCLE(4);
    }
    break;
  }
  fprintf(stderr, "pc = %06x\n", cpu->pc);
  cpu_prefetch();
  cpu_prefetch();
}

static struct cprint *bcc_print(LONG addr, WORD op)
{
  LONG a;
  int s;
  static char *cond[16] = {
    "BRA", "BSR", "BHI", "BLS", "BCC", "BCS", "BNE", "BEQ",
    "BVC", "BVS", "BPL", "BMI", "BGE", "BLT", "BGT", "BLE"
  };
  struct cprint *ret;

  ret = cprint_alloc(addr);

  a = op&0xff;
  if(a&0x80) a |= 0xffffff00;

  if(a) {
    s = 1;
  } else {
    s = 0;
    a = bus_read_word_print(addr+ret->size);
    if(a&0x8000) a |= 0xffff0000;
  }

  a += addr+ret->size;

  if(!s) ret->size += 2;

  strcpy(ret->instr, cond[(op&0xf00)>>8]);
  if(s) strcat(ret->instr, ".S");
  
  cprint_set_label(a, NULL);
  if(cprint_find_label(a)) {
    sprintf(ret->data, "%s", cprint_find_label(a));
  } else {
    sprintf(ret->data, "$%x", a);
  }

  return ret;
}

void bcc_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x1000;i++) {
    instr[0x6000|i] = (void *)bcc;
    print[0x6000|i] = (void *)bcc_print;
  }
}




