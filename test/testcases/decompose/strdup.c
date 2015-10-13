#include <stdio.h>

extern void use(const char*);

int main() {
	const char* hello = "Hello world";
	use(hello);

	use(strdup(hello));

	return 0;
}
