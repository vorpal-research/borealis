extern int nondet_int(void);

void borealis_assert(int cond);
#define __VERIFIER_assert(X) borealis_assert(X)

void main()
{
  int y;

  y = 1;

  while(1)
    {
      y = y +2*nondet_int();


      borealis_assert (y!=0);
	
    }
}

