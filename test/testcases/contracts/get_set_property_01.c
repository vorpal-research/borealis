#include "defines.h"

// @requires argc > 13
int main(int argc, char** argv) {

	borealis_set_property("length", argv, (void*)argc);

	if ( ((int)borealis_get_property("length", argv)) <= 13 ) {
		borealis_action_defect("WTF-13", "Should not happen");
	}

	return 0;
}
