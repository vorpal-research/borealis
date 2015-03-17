#include <string.h>
#include <stdlib.h>

void borealis_assert(int);

int main() {
	char* sss = malloc(255);

	memcpy(sss, "Hello world", 12);

	borealis_assert(strlen(sss) == 11);
	sss[4] = '\0';
	borealis_assert(strlen(sss) == 4);

	free(sss);

}