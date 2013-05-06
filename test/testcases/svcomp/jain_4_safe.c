extern int nondet_int(void);
void borealis_assert(int cond);
#define __VERIFIER_assert(X) borealis_assert(X)
void main()
{
  int x,y,z;

  x=0;
  y=0;
  z=0;

  while(1)
    {
      x = x +4*nondet_int();
      y = y +4*nondet_int();
      z = z +8*nondet_int();

      __VERIFIER_assert(x+y+z!=1);
    }
}

