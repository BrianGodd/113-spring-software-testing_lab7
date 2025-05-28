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
#include <set> // Required for std::set
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

// Define the set of sink functions
static const std::set<std::string> sinkFunctions = {
    "system", "popen",
    "execl", "execv", "execle", "execve", "execlp", "execvp", "execvpe",
    "ShellExecuteA", "ShellExecuteW",
    "CreateProcessA", "CreateProcessW"
};

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

  LLVMContext &C = M.getContext();
  Type *VoidTy = Type::getVoidTy(C);
  Type *CharStarTy = Type::getInt8PtrTy(C);

  // Get reference to __afl_check_cmd_injection
  FunctionCallee check_cmd_injection = M.getOrInsertFunction(
      "__afl_check_cmd_injection",
      FunctionType::get(VoidTy, {CharStarTy}, false));

  for (auto &F : M) {

    llvm::StringRef fn = F.getName();

    if (fn.equals("_start") || fn.startswith("__libc_csu") ||
        fn.startswith("__afl_") || fn.startswith("__asan") ||
        fn.startswith("asan.") || fn.startswith("llvm."))
      continue;

    for (auto &BB : F) {

      for (auto &I : BB) {

        if (CallInst *call = dyn_cast<CallInst>(&I)) {

          llvm::StringRef calledFuncName = GetCallInsFunctionName(call);
          std::string calledFuncNameStdStr = calledFuncName.str();

          if (sinkFunctions.count(calledFuncNameStdStr)) {
            Value *commandArgument = nullptr;
            unsigned argIndex = 0; // Default for system, popen

            if (calledFuncNameStdStr == "system" || calledFuncNameStdStr == "popen") {
              argIndex = 0;
            } else if (
                calledFuncNameStdStr == "execl" ||
                calledFuncNameStdStr == "execv" ||
                calledFuncNameStdStr == "execle" ||
                calledFuncNameStdStr == "execve" ||
                calledFuncNameStdStr == "execlp" ||
                calledFuncNameStdStr == "execvp" ||
                calledFuncNameStdStr == "execvpe" ||
                calledFuncNameStdStr == "CreateProcessA" || // lpApplicationName or lpCommandLine
                calledFuncNameStdStr == "CreateProcessW"    // lpApplicationName or lpCommandLine
            ) {
              // For exec*, CreateProcess*, use the first argument (path/file/lpApplicationName)
              // or second (lpCommandLine for CreateProcess)
              // For simplicity, let's stick to a common index if possible or make specific choices.
              // CreateProcess lpCommandLine is arg 1. For exec* path is arg 0.
              // The subtask said for other functions (non system/popen) use argOperand(1)
              // For CreateProcessA/W, lpCommandLine is getArgOperand(1)
              // For execl, the 'file' or 'path' is typically the first argument (index 0),
              // but the prompt mentioned 'arg' as second for execl.
              // Let's use 0 for exec* functions as 'path' or 'file' is the first argument.
              // The prompt said: "For other functions, try to get call->getArgOperand(1) as a starting point"
              // This was then followed by: "For exec functions, the first argument is typically path or file. Let's target the file or path argument for now."
              // This implies arg 0 for exec*.
              // Let's stick to the more specific information for exec* (arg 0) and CreateProcess* (arg 1).
              // ShellExecute: lpFile (arg 1) or lpParameters (arg 3). Prompt: "Let's try to get lpParameters (the 4th argument, index 3). If it's null, then try lpFile (the 2nd argument, index 1)."

              if (calledFuncNameStdStr.rfind("exec", 0) == 0) { // starts with exec
                 if (call->getNumArgOperands() > 0) commandArgument = call->getArgOperand(0);
              } else if (calledFuncNameStdStr.find("CreateProcess") != std::string::npos) {
                 if (call->getNumArgOperands() > 1) commandArgument = call->getArgOperand(1); // lpCommandLine
              } else if (calledFuncNameStdStr.find("ShellExecute") != std::string::npos) {
                // Try lpParameters (arg 3) first
                if (call->getNumArgOperands() > 3 && call->getArgOperand(3) && !isa<ConstantPointerNull>(call->getArgOperand(3))) {
                    commandArgument = call->getArgOperand(3);
                } else if (call->getNumArgOperands() > 1) { // Fallback to lpFile (arg 1)
                    commandArgument = call->getArgOperand(1);
                }
              }
              // Fallback if not specifically handled above, though most should be.
              // The original simplified logic was "call->getArgOperand(1)".
              // execl's "arg" (the actual command passed to the new program) is the second argument (index 1)
              // if we consider `path` as the first.
              // Let's refine for execl based on "For execl: It's the second argument (arg)."
              else if (calledFuncNameStdStr == "execl") {
                if (call->getNumArgOperands() > 1) commandArgument = call->getArgOperand(1);
              }
              // Defaulting to arg 0 if commandArgument is still null, for functions like execv where 'file' is appropriate.
              else {
                if (call->getNumArgOperands() > 0) commandArgument = call->getArgOperand(0);
              }


            } else { // Default for system, popen
               if (call->getNumArgOperands() > 0) commandArgument = call->getArgOperand(0);
            }


            if (commandArgument) {
              IRBuilder<> IRB(&I); // Build before the call instruction
              Value *cmdArgForCheck = commandArgument;

              // Ensure the argument is char*
              if (cmdArgForCheck->getType() != CharStarTy) {
                cmdArgForCheck = IRB.CreateBitCast(cmdArgForCheck, CharStarTy);
              }

              IRB.CreateCall(check_cmd_injection, {cmdArgForCheck})
                  ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
            }
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
    // Check if the called value is a function pointer casted from another type
    Value* calledValue = call->getCalledOperand();
    if (ConstantExpr* ce = dyn_cast<ConstantExpr>(calledValue)) {
        if (ce->isCast()) {
            if (Function* f = dyn_cast<Function>(ce->getOperand(0))) {
                return f->getName();
            }
        }
    }
    // Fallback for other indirect calls, though this might still not cover all cases or might be risky
    // The original code: dyn_cast<Function>(call->getCalledOperand()->stripPointerCasts())->getName();
    // This can crash if stripPointerCasts() doesn't resolve to a Function.
    // A safer approach is to return an empty StringRef or a placeholder if the name can't be resolved.
    Value *strippedValue = call->getCalledOperand()->stripPointerCasts();
    if (Function *f = dyn_cast<Function>(strippedValue)) {
        return f->getName();
    }
    return ""; // Return empty if function name cannot be resolved

  }

}

