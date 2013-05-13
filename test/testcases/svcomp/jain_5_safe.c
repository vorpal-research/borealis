extern int nondet_int(void);
void borealis_assert(int cond);
#define __VERIFIER_assert(X) borealis_assert(X)
void main()
{
  int x,y;

  x=0;
  y=4;


  while(1)
    {
      x = x + y;
      y = y +4;
      
      
      __VERIFIER_assert(x!=30);
    }
}

