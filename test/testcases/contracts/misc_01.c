// this file is here only for testing annotation parsing
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int* val = (int*)malloc(sizeof(int));
	int* e0x = val;

	// @assume (e0x != 0) == true
	// @stack-depth 0x0
	// @stack-depth 010
	// @stack-depth 890
	// @assert (2.15 == 2.15) && (.0 == 0.)

	*val = 2 + argv[0][0];

	return *val+1;
}
