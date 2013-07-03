#include "defines.h"

// @requires argc > 13
int main(int argc, char** argv) {

	borealis_action_defect("BUG-42", "Lorem ipsum");

	if (argc <= 13) {
		borealis_action_defect("WTF-13", "Should not happen");
	}

	return 0;
}
