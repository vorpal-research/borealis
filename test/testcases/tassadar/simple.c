
int borealis_nondet();

int main() {
	int a[220] = {0};

	a[0] = 42;
	a[2] = 1;
	a[3] = 2;

	a[221] = 4;

	return a[borealis_nondet()];
}