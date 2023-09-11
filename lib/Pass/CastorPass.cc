/*
 * Castor Record/Replay Pass
 */

#define CASTOR_DEBUG_TYPE "castor"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "CastorPass.h"

namespace {

bool doBasicBlock(Module &M, BasicBlock &BB);
bool doAtomicRMW(Module &M, AtomicRMWInst *I);
bool doAtomicCmpXchg(Module &M, AtomicCmpXchgInst *I);
bool doLoad(Module &M, LoadInst *I);
bool doStore(Module &M, StoreInst *I);
bool doCall(Module &M, CallInst *I, int *rem);

bool runPass(Module &M) 
{
    bool mod = false;

    for (auto &F : M) {
	for (auto &BB : F) {
	    mod |= doBasicBlock(M, BB);
	}
    }

    return mod;
}

// Basic Block Routines
bool doBasicBlock(Module &M, BasicBlock &BB)
{
    bool mod = false;
    int rem = 0;
    std::vector<Instruction *> tbr;
    Instruction *ri;

    for (auto &Inst : BB) {
	rem = 0;
	// Handle Atomics and Volatile load/store
	AtomicRMWInst *ARMW = dyn_cast<AtomicRMWInst>(&Inst);
	if (ARMW) {
	    mod |= doAtomicRMW(M, ARMW);
	}

	AtomicCmpXchgInst *AXCHG = dyn_cast<AtomicCmpXchgInst>(&Inst);
	if (AXCHG) {
	    mod |= doAtomicCmpXchg(M, AXCHG);
	}

	LoadInst *LI = dyn_cast<LoadInst>(&Inst);
	if (LI) {
	    mod |= doLoad(M, LI);
	}

	StoreInst *SI = dyn_cast<StoreInst>(&Inst);
	if (SI) {
	    mod |= doStore(M, SI);
	}

	// Handle RDTSC, RDRAND, inline asm, etc.
	CallInst *CI = dyn_cast<CallInst>(&Inst);
	if (CI) {
	    mod |= doCall(M, CI, &rem);
	    if (rem) {
		tbr.push_back(CI);
	    }
	}
    }

    while (tbr.size() > 0) {
	ri = (Instruction *)(tbr.back());
	tbr.pop_back();
	ri->eraseFromParent();
    }

    return mod;
}

bool doAtomicRMW(Module &M, AtomicRMWInst *I)
{
    errs() << "ATOMIC RMW\n";

    LLVMContext &ctx = M.getContext();
    Instruction *ni = I->getNextNode();

    Value *addr = I->getPointerOperand();

    IRBuilder<> before(I);
    IRBuilder<> after(ni);

    FunctionCallee load_begin = M.getOrInsertFunction("__castor_rmw_begin",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
    FunctionCallee load_end = M.getOrInsertFunction("__castor_rmw_end",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
    Value *tmpAddr = before.CreatePtrToInt(addr, Type::getInt64Ty(ctx));

    before.CreateCall(load_begin, {tmpAddr});
    after.CreateCall(load_end, {tmpAddr});

    return true;
}

bool doAtomicCmpXchg(Module &M, AtomicCmpXchgInst *I)
{
    errs() << "ATOMIC CMP XCHG\n";

    LLVMContext &ctx = M.getContext();
    Instruction *ni = I->getNextNode();

    Value *addr = I->getPointerOperand();

    IRBuilder<> before(I);
    IRBuilder<> after(ni);

    FunctionCallee load_begin = M.getOrInsertFunction("__castor_cmpxchg_begin",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
    FunctionCallee load_end = M.getOrInsertFunction("__castor_cmpxchg_end",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
    Value *tmpAddr = before.CreatePtrToInt(addr, Type::getInt64Ty(ctx));

    before.CreateCall(load_begin, {tmpAddr});
    after.CreateCall(load_end, {tmpAddr});

    return true;
}

bool doLoad(Module &M, LoadInst *I)
{
    if (I->getOrdering() != AtomicOrdering::NotAtomic || I->isVolatile()) {
	errs() << "ATOMIC LOAD\n";
	LLVMContext &ctx = M.getContext();
	Instruction *ni = I->getNextNode();

	Value *addr = I->getPointerOperand();

	IRBuilder<> before(I);
	IRBuilder<> after(ni);

	FunctionCallee load_begin = M.getOrInsertFunction("__castor_load_begin",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
	FunctionCallee load_end = M.getOrInsertFunction("__castor_load_end",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
	Value *tmpAddr = before.CreatePtrToInt(addr, Type::getInt64Ty(ctx));

	before.CreateCall(load_begin, {tmpAddr});
	after.CreateCall(load_end, {tmpAddr});

	return true;
    }

    return false;
}

bool doStore(Module &M, StoreInst *I)
{
    if (I->getOrdering() != AtomicOrdering::NotAtomic || I->isVolatile()) {
	errs() << "ATOMIC STORE\n";
	LLVMContext &ctx = M.getContext();
	Instruction *ni = I->getNextNode();

	Value *addr = I->getPointerOperand();

	IRBuilder<> before(I);
	IRBuilder<> after(ni);

	FunctionCallee store_begin = M.getOrInsertFunction("__castor_store_begin",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
	FunctionCallee store_end = M.getOrInsertFunction("__castor_store_end",
				Type::getVoidTy(ctx),
				Type::getInt64Ty(ctx));
	Value *tmpAddr = before.CreatePtrToInt(addr, Type::getInt64Ty(ctx));

	before.CreateCall(store_begin, {tmpAddr});
	after.CreateCall(store_end, {tmpAddr});

	return true;
    }

    return false;
}

bool doCall(Module &M, CallInst *I, int *rem)
{
    *rem = 0;

    if (I->isInlineAsm()) {
	bool needRR = false;
	errs() << "Inline Assembly\n";
	Value *f = I->getCalledOperand();
	InlineAsm *ia = dyn_cast<InlineAsm>(f);
	InlineAsm::ConstraintInfoVector civ = ia->ParseConstraints();

	for (auto &c : civ) {
	    if (c.Type != InlineAsm::isClobber) {
		continue;
	    }
	    for (auto &i : c.Codes) {
		if (i == "{memory}")
		    needRR = true;
	    }
	}

	if (needRR) {
	    LLVMContext &ctx = M.getContext();
	    Instruction *ni = I->getNextNode();

	    IRBuilder<> before(I);
	    IRBuilder<> after(ni);

	    FunctionCallee asm_begin = M.getOrInsertFunction("__castor_asm_begin",
				Type::getVoidTy(ctx));
	    FunctionCallee asm_end = M.getOrInsertFunction("__castor_asm_end",
				Type::getVoidTy(ctx));

	    before.CreateCall(asm_begin);
	    after.CreateCall(asm_end);

	    return true;
	}

	return false;
    }

    Function *f = I->getCalledFunction();
    if (!f)
	return false;

    if (f->getName().equals("llvm.readcyclecounter")) {
	errs() << "RDTSC\n";
	LLVMContext &ctx = M.getContext();
	Instruction *ni = I->getNextNode();

	IRBuilder<> after(ni);

	FunctionCallee castor_rdtsc = M.getOrInsertFunction("__castor_rdtsc",
				Type::getInt64Ty(ctx));
	Value *rdtsc = after.CreateCall(castor_rdtsc);

	for (auto &U : I->uses()) {
	    User *user = U.getUser();
	    user->setOperand(U.getOperandNo(), rdtsc);
	}

	*rem = 1;
	return true;
    }

    // XXX: RDRAND

    return false;
}


bool Castor::runOnModule(Module &M)
{
    return runPass(M);
}

struct CastorPass : public PassInfoMixin<CastorPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
	if (runPass(M)) {
		return PreservedAnalyses::none();
	} else {
  		return PreservedAnalyses::all();
	}
  }
};

} //namespace

/* New PM Registration */
llvm::PassPluginLibraryInfo getCastorPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "castor", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](llvm::ModulePassManager &MPM, OptimizationLevel ol) {
                   	MPM.addPass(CastorPass());
                    	return true;
                });
          }};
}

#ifndef LLVM_BYE_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getCastorPluginInfo();
}
#endif

/* Legacy PM Registration */
char Castor::ID = 0;
static RegisterPass<Castor> CP("castor", "Castor Record/Replay Pass");


static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
      PM.add(new Castor());
}

static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);


