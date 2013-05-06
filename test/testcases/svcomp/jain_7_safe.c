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
      x = x +1048576*nondet_int();
      y = y +2097152*nondet_int();
      z = z +4194304*nondet_int();

      __VERIFIER_assert(4*x-2*y+z!=1048576);
    }
}

