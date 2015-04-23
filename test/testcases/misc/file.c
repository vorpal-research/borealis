#include <stdio.h>
#include <stdlib.h>

int main() {

	FILE* in = fopen("file.txt", "r");

	fclose(in);
	return fgetc(in);
	
}
