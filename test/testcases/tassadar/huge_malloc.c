#include <stdlib.h>

int borealis_nondet();

int main() {

	int* m = malloc(2000000000ULL);

	m[23] = 42;

	return m[borealis_nondet()];

}