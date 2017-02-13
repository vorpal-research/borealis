//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_INTERPRETER_H
#define BOREALIS_INTERPRETER_H

#include "Interpreter/Environment.h"
#include "Interpreter/IR/Module.h"

namespace borealis {
namespace absint {

class Interpreter {
public:

    Interpreter(const llvm::Module* module);

    void run();

private:

    Environment environment_;
    Module module_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERPRETER_H
