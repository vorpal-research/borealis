#include <stdlib.h>
#include <string.h>

void borealis_assert(int);
void borealis_assume(int);
int borealis_nondet();

#define ASSERT(X) borealis_assert((int)(X))
#define ASSUME(X) borealis_assume((int)(X))
#define __NONDET__ borealis_nondet

int main() {

	char* mes = malloc(1);

	ASSERT(strlen(mes) == 666);

	free(mes);

}