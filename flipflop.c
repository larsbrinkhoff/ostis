#include "flipflop.h"

struct flipflop {
  int d, e, q;
};

void flipflop_d(struct flipflop *flipflop, int x)
{
  flipflop->d = x;
}

void flipflop_e(struct flipflop *flipflop, int x)
{
  if(x && !flipflop->e)
    flipflop->q = flipflop->d;
  flipflop->e = x;
}

int flipflop_q(struct flipflop *flipflop)
{
  return flipflop->q;
}

void flipflop_clr(struct flipflop *flipflop)
{
  flipflop->q = 0;
}

void flipflop_set(struct flipflop *flipflop)
{
  flipflop->q = 1;
}
