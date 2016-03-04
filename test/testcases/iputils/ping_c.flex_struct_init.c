#define __flexarr []

static struct {
    char c_value;
    __extension__ char c_data __flexarr;
} a = {0, };

int main(int argc, char** argv) {
    return a.c_value;
}
