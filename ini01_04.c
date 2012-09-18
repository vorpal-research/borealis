//
// INI01 - использование неинициализированной переменной
//

int main(void)
{
    char * ps;
    
    ps = "absracadabra";
    int res = ps[1]; // нет дефекта
    
    return 0;
}
