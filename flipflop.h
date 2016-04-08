#ifndef SHIFTER_H
#define SHIFTER_H

struct flipflop;

extern void flipflop_d(struct flipflop *, int);
extern void flipflop_e(struct flipflop *, int);
extern void flipflop_clr(struct flipflop *);
extern void flipflop_set(struct flipflop *);
extern int flipflop_q(struct flipflop *);

#endif /* SHIFTER_H */
