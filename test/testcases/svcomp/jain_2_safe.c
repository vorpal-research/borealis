extern int nondet_int(void);
void borealis_assert(int cond);
#define __VERIFIER_assert(X) borealis_assert(X)
void main()
{
  int x,y;

  x = 1;
  y = 1;

  while(1)
    {
      x = x +2*nondet_int();
      y = y +2*nondet_int();


      __VERIFIER_assert(x+y!=1);
    }
}

