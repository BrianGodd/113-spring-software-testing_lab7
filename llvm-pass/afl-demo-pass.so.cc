#define AFL_LLVM_PASS

#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <set>
#include <iostream>

#include "llvm/Config/llvm-config.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"

#include "llvm/Passes/OptimizationLevel.h"

#include "afl-llvm-common.h"

using namespace llvm;

namespace {

class AFLDEMOPass : public PassInfoMixin<AFLDEMOPass> {

 public:
  AFLDEMOPass() {

  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  llvm::StringRef   GetCallInsFunctionName(CallInst *call);

 protected:

};

}  // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {

  return {

      LLVM_PLUGIN_API_VERSION, "AFLDEMOPass", "v0.1", [](PassBuilder &PB) {

        PB.registerOptimizerLastEPCallback(
            [](ModulePassManager &MPM, OptimizationLevel OL) {

              MPM.addPass(AFLDEMOPass());

            });

      }};

}

PreservedAnalyses AFLDEMOPass::run(Module &M, ModuleAnalysisManager &MAM) {

  LLVMContext &  C = M.getContext();
  Type *         VoidTy = Type::getVoidTy(C);
  FunctionCallee demo_crash = M.getOrInsertFunction("__demo_crash", VoidTy);

  for (auto &F : M) {

    llvm::StringRef fn = F.getName();

    if (fn.equals("_start") || fn.startswith("__libc_csu") ||
        fn.startswith("__afl_") || fn.startswith("__asan") ||
        fn.startswith("asan.") || fn.startswith("llvm."))
      continue;

    for (auto &BB : F) {

      for (auto &I : BB) {

        if (CallInst *call = dyn_cast<CallInst>(&I)) {

          if (GetCallInsFunctionName(call).equals("system")) {

            IRBuilder<> IRB(&I);
            IRB.CreateCall(demo_crash)->setMetadata(M.getMDKindID("nosanitize"),
                                                   MDNode::get(C, None));

          }

        }

      }

    }

  }

  return PreservedAnalyses::all();

}

llvm::StringRef AFLDEMOPass::GetCallInsFunctionName(CallInst *call) {

  if (Function *func = call->getCalledFunction()) {

    return func->getName();

  } else {

    // Indirect call
    return dyn_cast<Function>(call->getCalledOperand()->stripPointerCasts())
        ->getName();

  }

}

