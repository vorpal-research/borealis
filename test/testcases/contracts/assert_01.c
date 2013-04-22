#include <stdlib.h>

int main(int argc, char* argv[]) {
	// @assume argv != 0
	// @assume *argv != 0

	int* val = (int*)malloc(sizeof(int));
	// @assert val != 0

	// @assume val != 0
	*val = 2 + argv[0][0];

	return *val+1;
}
