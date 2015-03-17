#include <string.h>
#include <stdio.h>

int borealis_nondet();

void borealis_assert(int);

int main() {
	const char* message = "hello world";
	//printf(message);

	char buffer[22];

	memcpy(buffer  + 7, message, 12);

	borealis_assert(buffer[10] == 'l');
	borealis_assert(buffer[13] == 'w');

	return buffer[borealis_nondet()];
}