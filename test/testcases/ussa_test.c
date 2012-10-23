int main(int x) {
	int* a = (int*)x;
	int i = 0;

	if(x > 20) {
		a = 0xdeadbeef;
	}
	for(i = 0; i < 100; ++i) *a += i;

	return a;
}