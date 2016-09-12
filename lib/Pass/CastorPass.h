/*
 * Castor Record/Replay Pass
 */

#ifndef CASTOR_H
#define CASTOR_H

using namespace llvm;

namespace {

class Castor : public ModulePass {
public:
    static char ID;
    Castor() : ModulePass(ID) { }
    bool runOnModule(Module &M);
private:
    // Basic Blocks
    bool doBasicBlock(Module &M, BasicBlock &BB);
    // Load/Store
    bool doAtomicRMW(Module &M, AtomicRMWInst *I);
    bool doAtomicCmpXchg(Module &M, AtomicCmpXchgInst *I);
    bool doLoad(Module &M, LoadInst *I);
    bool doStore(Module &M, StoreInst *I);
    // Calls
    bool doCall(Module &M, CallInst *CI);
};

}

#endif /* CASTOR_H */

