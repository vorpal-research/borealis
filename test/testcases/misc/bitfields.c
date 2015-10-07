//
// Created by ice-phoenix on 9/25/15.
//

struct {
    int yes : 1;
    int no : 1;
} a;

int main(int argc, char** argv) {
    return 1 == argc ? a.yes : a.no;
}
