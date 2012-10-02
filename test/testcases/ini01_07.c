char str[10] = "hello";

int main(void)
{
   // Не должно быть INI-01: str[6] = 0
   return str[6];
}