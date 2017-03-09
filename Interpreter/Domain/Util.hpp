//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_UTILS_HPP
#define BOREALIS_UTILS_HPP

namespace borealis {
namespace util {

llvm::APSInt umin(const llvm::APInt& lhv, const llvm::APInt& rhv) {
    return llvm::APSInt((lhv.ult(rhv)) ? lhv : rhv);
}

llvm::APSInt smin(const llvm::APInt& lhv, const llvm::APInt& rhv) {
    return llvm::APSInt((lhv.slt(rhv)) ? lhv : rhv);
}

llvm::APSInt umax(const llvm::APInt& lhv, const llvm::APInt& rhv) {
    return llvm::APSInt((lhv.ugt(rhv)) ? lhv : rhv);
}

llvm::APSInt smax(const llvm::APInt& lhv, const llvm::APInt& rhv) {
    return llvm::APSInt((lhv.sgt(rhv)) ? lhv : rhv);
}

llvm::APSInt min(const llvm::APSInt& lhv, const llvm::APSInt& rhv) {
    return (lhv < rhv) ? lhv : rhv;
}

llvm::APSInt max(const llvm::APSInt& lhv, const llvm::APSInt& rhv) {
    return (lhv > rhv) ? lhv : rhv;
}


}   /* namespace util */
}   /* namespace borealis */

#endif //BOREALIS_UTILS_HPP
