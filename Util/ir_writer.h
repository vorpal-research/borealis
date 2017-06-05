#ifndef IR_WRITER_H
#define IR_WRITER_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/TypeFinder.h>
#include <llvm/Support/FormattedStream.h>

#include "Util/slottracker.h"

namespace borealis {

class TypePrinting {
    TypePrinting(const TypePrinting &) LLVM_DELETED_FUNCTION;
    void operator=(const TypePrinting&) LLVM_DELETED_FUNCTION;
public:

    /// NamedTypes - The named types that are used by the current module.
    llvm::TypeFinder NamedTypes;

    /// NumberedTypes - The numbered types, along with their value.
    llvm::DenseMap<llvm::StructType*, unsigned> NumberedTypes;


    TypePrinting() {}
    explicit TypePrinting(const llvm::Module& M) { incorporateTypes(M); }
    ~TypePrinting() {}

    void incorporateTypes(const llvm::Module &M);

    void print(llvm::Type *Ty, llvm::raw_ostream &OS);

    void printStructBody(llvm::StructType *Ty, llvm::raw_ostream &OS);
};


class AssemblyWriter {
protected:
    llvm::formatted_raw_ostream &Out;
    const llvm::Module *TheModule;

private:
    std::unique_ptr<SlotTracker> ModuleSlotTracker;
    SlotTracker& Machine;
    std::unique_ptr<TypePrinting> ModuleTypePrinting;
    TypePrinting& TypePrinter;
    llvm::AssemblyAnnotationWriter *AnnotationWriter;
    llvm::SetVector<const llvm::Comdat *> Comdats;

public:
    /// Construct an AssemblyWriter with an external SlotTracker
    AssemblyWriter(llvm::formatted_raw_ostream &o, SlotTracker& Mac, TypePrinting& Tp,
                   const llvm::Module *M, llvm::AssemblyAnnotationWriter *AAW);

    /// Construct an AssemblyWriter with an internally allocated SlotTracker
    AssemblyWriter(llvm::formatted_raw_ostream &o, const llvm::Module *M,
                   llvm::AssemblyAnnotationWriter *AAW);

    virtual ~AssemblyWriter();

    void printMDNodeBody(const llvm::MDNode *MD);
    void printNamedMDNode(const llvm::NamedMDNode *NMD);

    void printModule(const llvm::Module *M);

    void writeOperand(const llvm::Value *Op, bool PrintType);
    void writeParamOperand(const llvm::Value *Operand, llvm::AttributeSet Attrs,unsigned Idx);
    void writeAtomic(llvm::AtomicOrdering Ordering, llvm::SynchronizationScope SynchScope);
    void writeAtomicCmpXchg(llvm::AtomicOrdering SuccessOrdering,
                            llvm::AtomicOrdering FailureOrdering,
                            llvm::SynchronizationScope SynchScope);

    void writeAllMDNodes();
    void writeMDNode(unsigned Slot, const llvm::MDNode *Node);

    void printTypeIdentities();
    void printGlobal(const llvm::GlobalVariable *GV);
    void printAlias(const llvm::GlobalAlias *GV);
    void printComdat(const llvm::Comdat *C);
    void printFunction(const llvm::Function *F);
    void printArgument(const llvm::Argument *FA, llvm::AttributeSet Attrs, unsigned Idx);
    void printBasicBlock(const llvm::BasicBlock *BB);
    void printInstructionLine(const llvm::Instruction &I);
    void printInstruction(const llvm::Instruction &I);

private:
    void init();

    // printInfoComment - Print a little comment after the instruction indicating
    // which slot it occupies.
    void printInfoComment(const llvm::Value &V);
};

void printModule(llvm::Module* self, llvm::raw_ostream &ROS, llvm::AssemblyAnnotationWriter *AAW, SlotTracker& St, TypePrinting& Tp);
void printNamedMDNode(llvm::NamedMDNode* self, llvm::raw_ostream &ROS, SlotTracker& St, TypePrinting& Tp);
void printComdat(llvm::Comdat* self, llvm::raw_ostream &ROS, SlotTracker& St, TypePrinting& Tp);
void printType(llvm::Type* self, llvm::raw_ostream &OS, TypePrinting& Tp);
void printValue(llvm::Value* self, llvm::raw_ostream &ROS, SlotTracker& St, TypePrinting& Tp);
void printValueAsOperand(llvm::Value* self, llvm::raw_ostream &O, bool PrintType, const llvm::Module *M, SlotTracker& St, TypePrinting& Tp);


} /* namespace borealis */

#endif //IR_WRITER_H
