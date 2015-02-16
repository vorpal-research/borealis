#include <stdlib.h>

#include <stdio.h>

int main() {
	
	int* i = malloc(sizeof(int));

	*i = 42;

	printf("%d", *i);

	free(i);
}