int main(void) {
    int i;
    int *p;
    int *q;
    int a[2];
    
    p = &a[0];
    q = p + 5;
    
    i = p - q;
    i++;
    return i;
}
