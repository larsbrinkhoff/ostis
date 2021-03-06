#ifndef INSTR_H
#define INSTR_H

void move_to_sr_init(void *[], void *[]);
void reset_init(void *[], void *[]);
void cmpi_init(void *[], void *[]);
void bcc_init(void *[], void *[]);
void lea_init(void *[], void *[]);
void suba_init(void *[], void *[]);
void jmp_init(void *[], void *[]);
void move_init(void *[], void *[]);
void btst_init(void *[], void *[]);
void moveq_init(void *[], void *[]);
void cmp_init(void *[], void *[]);
void dbcc_init(void *[], void *[]);
void clr_init(void *[], void *[]);
void movea_init(void *[], void *[]);
void add_init(void *[], void *[]);
void cmpa_init(void *[], void *[]);
void lsr_init(void *[], void *[]);
void adda_init(void *[], void *[]);
void addq_init(void *[], void *[]);
void sub_init(void *[], void *[]);
void scc_init(void *[], void *[]);
void movep_init(void *[], void *[]);
void movem_init(void *[], void *[]);
void rts_init(void *[], void *[]);
void and_init(void *[], void *[]);
void exg_init(void *[], void *[]);
void or_init(void *[], void *[]);
void subq_init(void *[], void *[]);
void bclr_init(void *[], void *[]);
void asl_init(void *[], void *[]);
void addi_init(void *[], void *[]);
void bset_init(void *[], void *[]);
void move_from_sr_init(void *[], void *[]);
void ori_init(void *[], void *[]);
void ori_to_sr_init(void *[], void *[]);
void andi_init(void *[], void *[]);
void andi_to_sr_init(void *[], void *[]);
void move_usp_init(void *[], void *[]);
void lsl_init(void *[], void *[]);
void trap_init(void *[], void *[]);
void tst_init(void *[], void *[]);
void jsr_init(void *[], void *[]);
void mulu_init(void *[], void *[]);
void divu_init(void *[], void *[]);
void link_init(void *[], void *[]);
void rte_init(void *[], void *[]);
void rtr_init(void *[], void *[]);
void unlk_init(void *[], void *[]);
void move_from_ccr_init(void *[], void *[]);
void movec_init(void *[], void *[]);
void swap_init(void *[], void *[]);
void pea_init(void *[], void *[]);
void ext_init(void *[], void *[]);
void muls_init(void *[], void *[]);
void asr_init(void *[], void *[]);
void eor_init(void *[], void *[]);
void eori_init(void *[], void *[]);
void eori_to_sr_init(void *[], void *[]);
void nop_init(void *[], void *[]);
void linef_init(void *[], void *[]);
void divs_init(void *[], void *[]);
void rol_init(void *[], void *[]);
void roxl_init(void *[], void *[]);
void not_init(void *[], void *[]);
void ror_init(void *[], void *[]);
void neg_init(void *[], void *[]);
void subi_init(void *[], void *[]);
void move_to_ccr_init(void *[], void *[]);
void ori_to_ccr_init(void *[], void *[]);
void addx_init(void *[], void *[]);
void cmpm_init(void *[], void *[]);
void subx_init(void *[], void *[]);
void eori_to_ccr_init(void *[], void *[]);
void negx_init(void *[], void *[]);
void linea_init(void *[], void *[]);
void roxr_init(void *[], void *[]);
void bchg_init(void *[], void *[]);
void abcd_init(void *[], void *[]);
void stop_init(void *[], void *[]);
void sbcd_init(void *[], void *[]);
void tas_init(void *[], void *[]);
void andi_to_ccr_init(void *[], void *[]);

/*
 * Unimplemented:
 * 
 * NBCD
 * CHK
 * TRAPV
 * 
 */

#endif
