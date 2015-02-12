
#include <string.h>
#include <stdlib.h>

int borealis_nondet();


int main() {
	int a[220] = {0};
	int x[22000];

	a[0] = 42;
	a[2] = 1;
	a[3] = 2;

	a[219] = 4;

	a[218] = 249;


	int* b = malloc(250*sizeof(int));

	b[42] = 218;

	b[a[b[42]] + 1] = a[219];

	return b[borealis_nondet()];
}