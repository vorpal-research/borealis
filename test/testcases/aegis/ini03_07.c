int main(void) {
    int a[10];
    
    int *p = a;
    int *q = a;
    
    int i;
    
    p += i;
    *p = -1;
    
    a[0] = 0;
    
    int result = *q;
    return result;
}
