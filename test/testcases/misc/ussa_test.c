int main(int argc, char* argv[]) {
	int* a = (int*)argc;
	int i = 0;

	if(argc > 20) {
		a = (int*)0xdeadbeef;
	}
	for(i = 0; i < 100; ++i) *a += i;

	return (int)a;
}
