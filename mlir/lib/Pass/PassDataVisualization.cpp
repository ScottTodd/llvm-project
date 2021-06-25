//===- PassDataVisualization.cpp
//-----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassInstrumentation.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/StringMap.h"
// #include "llvm/Support/Debug.h"
#include "llvm/Support/JSON.h"

using namespace mlir;

//===----------------------------------------------------------------------===//
// PassDataVisualization
//===----------------------------------------------------------------------===//

namespace {
struct PassDataVisualization : public PassInstrumentation {
  PassDataVisualization() : os(jsonStr), jsonOS(os, /*IntentSize=*/2) {
    // TODO(scotttodd): write to a file (path name or output stream arg here)
    jsonOS.arrayBegin();
  }
  ~PassDataVisualization() {
    jsonOS.arrayEnd();

    os.flush();
    llvm::dbgs() << jsonStr;
  }

  void runAfterPass(Pass *pass, Operation *op) override {
    jsonOS.objectBegin();
    jsonOS.attribute("passName", pass->getName());

    llvm::StringMap<int> opDialectCounts;

    auto *topLevelOp = op;
    while (auto *parentOp = topLevelOp->getParentOp())
      topLevelOp = parentOp;

    topLevelOp->walk([&](Operation *opWithinModule) {
      auto opDialectNamespace = opWithinModule->getDialect()->getNamespace();

      // Skip built-in ops (ModuleOp, FuncOp, etc.)
      if (opDialectNamespace.empty())
        return;

      opDialectCounts[opDialectNamespace]++;
    });

    jsonOS.attributeBegin("dialectOpCounts");
    jsonOS.arrayBegin();
    for (const auto &opDialectCount : opDialectCounts) {
      jsonOS.objectBegin();
      jsonOS.attribute("dialectName", opDialectCount.getKey());
      jsonOS.attribute("opCount", opDialectCount.getValue());
      jsonOS.objectEnd();
    }
    jsonOS.arrayEnd();
    jsonOS.attributeEnd();

    jsonOS.objectEnd();
  }

  std::string jsonStr;
  llvm::raw_string_ostream os;
  llvm::json::OStream jsonOS;
};
} // namespace

//===----------------------------------------------------------------------===//
// PassManager
//===----------------------------------------------------------------------===//

/// Add an instrumentation that aggregates statistics for data visualization.
void PassManager::enableDataVisualizationInstrumentation() {
  addInstrumentation(std::make_unique<PassDataVisualization>());
}
