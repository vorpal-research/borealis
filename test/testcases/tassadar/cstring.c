#include <string.h>
#include <stdio.h>

int borealis_nondet();

void borealis_assert(int);

int main() {
	const char* message = "hello world";
	//printf(message);

	char buffer[22];

	memcpy(buffer, message, 12);

	borealis_assert(buffer[3] == 'l');
	borealis_assert(buffer[6] == 'w');

	return buffer[borealis_nondet()];
}