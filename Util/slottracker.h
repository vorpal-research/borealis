/*
 * slottracker.h
 *
 *  Created on: Sep 21, 2012
 *      Author: ice-phoenix
 */

#ifndef SLOTTRACKER_H_
#define SLOTTRACKER_H_

#include <llvm/Assembly/AssemblyAnnotationWriter.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Assembly/Writer.h>
#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/InlineAsm.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Operator.h>
#include <llvm/ValueSymbolTable.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CFG.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Dwarf.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <cctype>

//===----------------------------------------------------------------------===//
// SlotTracker Class: Enumerate slot numbers for unnamed values
//===----------------------------------------------------------------------===//

namespace borealis {

/// This class provides computation of slot numbers for LLVM Assembly writing.
///
class SlotTracker {
public:
  /// ValueMap - A mapping of Values to slot numbers.
  typedef llvm::DenseMap<const llvm::Value*, unsigned> ValueMap;

private:
  /// TheModule - The module for which we are holding slot numbers.
  const llvm::Module* TheModule;

  /// TheFunction - The function for which we are holding slot numbers.
  const llvm::Function* TheFunction;
  bool FunctionProcessed;

  /// mMap - The slot map for the module level data.
  ValueMap mMap;
  unsigned mNext;

  /// fMap - The slot map for the function level data.
  ValueMap fMap;
  unsigned fNext;

  /// mdnMap - Map for MDNodes.
  llvm::DenseMap<const llvm::MDNode*, unsigned> mdnMap;
  unsigned mdnNext;
public:
  /// Construct from a module
  explicit SlotTracker(const llvm::Module *M);
  /// Construct from a function, starting out in incorp state.
  explicit SlotTracker(const llvm::Function *F);

  /// Return the slot number of the specified value in its type
  /// plane. If something is not in the SlotTracker, return -1.
  int getLocalSlot(const llvm::Value *V);
  int getGlobalSlot(const llvm::GlobalValue *V);
  int getMetadataSlot(const llvm::MDNode *N);

  std::string getLocalName(const llvm::Value *V);

  /// If you'd like to deal with a function instead of just a module, use
  /// this method to get its data into the SlotTracker.
  void incorporateFunction(const llvm::Function *F) {
    TheFunction = F;
    FunctionProcessed = false;
  }

  /// After calling incorporateFunction, use this method to remove the
  /// most recently incorporated function from the SlotTracker. This
  /// will reset the state of the machine back to just the module contents.
  void purgeFunction();

  /// MDNode map iterators.
  typedef llvm::DenseMap<const llvm::MDNode*, unsigned>::iterator mdn_iterator;
  mdn_iterator mdn_begin() { return mdnMap.begin(); }
  mdn_iterator mdn_end() { return mdnMap.end(); }
  unsigned mdn_size() const { return mdnMap.size(); }
  bool mdn_empty() const { return mdnMap.empty(); }

  /// This function does the actual initialization.
  inline void initialize();

  // Implementation Details
private:
  /// CreateModuleSlot - Insert the specified GlobalValue* into the slot table.
  void CreateModuleSlot(const llvm::GlobalValue *V);

  /// CreateMetadataSlot - Insert the specified MDNode* into the slot table.
  void CreateMetadataSlot(const llvm::MDNode *N);

  /// CreateFunctionSlot - Insert the specified Value* into the slot table.
  void CreateFunctionSlot(const llvm::Value *V);

  /// Add all of the module level global variables (and their initializers)
  /// and function declarations, but not the contents of those functions.
  void processModule();

  /// Add all of the functions arguments, basic blocks, and instructions.
  void processFunction();

  SlotTracker(const SlotTracker &);  // DO NOT IMPLEMENT
  void operator=(const SlotTracker &);  // DO NOT IMPLEMENT
};

SlotTracker *createSlotTracker(const llvm::Value *V);

} /* namespace borealis */

#endif /* SLOTTRACKER_H_ */
