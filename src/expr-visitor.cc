/*
 * Copyright 2017 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/expr-visitor.h"

#include "src/cast.h"
#include "src/ir.h"

namespace wabt {

ExprVisitor::ExprVisitor(Delegate* delegate) : delegate_(delegate) {}

Result ExprVisitor::VisitExpr(Expr* expr) {
  switch (expr->type()) {
    case ExprType::AtomicLoad:
      CHECK_RESULT(delegate_->OnAtomicLoadExpr(cast<AtomicLoadExpr>(expr)));
      break;

    case ExprType::AtomicStore:
      CHECK_RESULT(delegate_->OnAtomicStoreExpr(cast<AtomicStoreExpr>(expr)));
      break;

    case ExprType::AtomicRmw:
      CHECK_RESULT(delegate_->OnAtomicRmwExpr(cast<AtomicRmwExpr>(expr)));
      break;

    case ExprType::AtomicRmwCmpxchg:
      CHECK_RESULT(
          delegate_->OnAtomicRmwCmpxchgExpr(cast<AtomicRmwCmpxchgExpr>(expr)));
      break;

    case ExprType::AtomicWait:
      CHECK_RESULT(delegate_->OnAtomicWaitExpr(cast<AtomicWaitExpr>(expr)));
      break;

    case ExprType::AtomicWake:
      CHECK_RESULT(delegate_->OnAtomicWakeExpr(cast<AtomicWakeExpr>(expr)));
      break;

    case ExprType::Binary:
      CHECK_RESULT(delegate_->OnBinaryExpr(cast<BinaryExpr>(expr)));
      break;

    case ExprType::Block:
      CHECK_RESULT(delegate_->OnBlockExpr(cast<BlockExpr>(expr)));
      break;

    case ExprType::Br:
      CHECK_RESULT(delegate_->OnBrExpr(cast<BrExpr>(expr)));
      break;

    case ExprType::BrIf:
      CHECK_RESULT(delegate_->OnBrIfExpr(cast<BrIfExpr>(expr)));
      break;

    case ExprType::BrTable:
      CHECK_RESULT(delegate_->OnBrTableExpr(cast<BrTableExpr>(expr)));
      break;

    case ExprType::Call:
      CHECK_RESULT(delegate_->OnCallExpr(cast<CallExpr>(expr)));
      break;

    case ExprType::CallIndirect:
      CHECK_RESULT(delegate_->OnCallIndirectExpr(cast<CallIndirectExpr>(expr)));
      break;

    case ExprType::Catch:
      CHECK_RESULT(delegate_->OnCatchExpr(cast<CatchExpr>(expr)));
      break;

    case ExprType::Compare:
      CHECK_RESULT(delegate_->OnCompareExpr(cast<CompareExpr>(expr)));
      break;

    case ExprType::Const:
      CHECK_RESULT(delegate_->OnConstExpr(cast<ConstExpr>(expr)));
      break;

    case ExprType::Convert:
      CHECK_RESULT(delegate_->OnConvertExpr(cast<ConvertExpr>(expr)));
      break;

    case ExprType::Drop:
      CHECK_RESULT(delegate_->OnDropExpr(cast<DropExpr>(expr)));
      break;

    case ExprType::Else:
      CHECK_RESULT(delegate_->OnElseExpr(cast<ElseExpr>(expr)));
      break;

    case ExprType::End:
      CHECK_RESULT(delegate_->OnEndExpr(cast<EndExpr>(expr)));
      break;

    case ExprType::GetGlobal:
      CHECK_RESULT(delegate_->OnGetGlobalExpr(cast<GetGlobalExpr>(expr)));
      break;

    case ExprType::GetLocal:
      CHECK_RESULT(delegate_->OnGetLocalExpr(cast<GetLocalExpr>(expr)));
      break;

    case ExprType::If:
      CHECK_RESULT(delegate_->OnIfExpr(cast<IfExpr>(expr)));
      break;

    case ExprType::IfExcept:
      CHECK_RESULT(delegate_->OnIfExceptExpr(cast<IfExceptExpr>(expr)));
      break;

    case ExprType::Load:
      CHECK_RESULT(delegate_->OnLoadExpr(cast<LoadExpr>(expr)));
      break;

    case ExprType::Loop:
      CHECK_RESULT(delegate_->OnLoopExpr(cast<LoopExpr>(expr)));
      break;

    case ExprType::MemoryCopy:
      CHECK_RESULT(delegate_->OnMemoryCopyExpr(cast<MemoryCopyExpr>(expr)));
      break;

    case ExprType::MemoryDrop:
      CHECK_RESULT(delegate_->OnMemoryDropExpr(cast<MemoryDropExpr>(expr)));
      break;

    case ExprType::MemoryFill:
      CHECK_RESULT(delegate_->OnMemoryFillExpr(cast<MemoryFillExpr>(expr)));
      break;

    case ExprType::MemoryGrow:
      CHECK_RESULT(delegate_->OnMemoryGrowExpr(cast<MemoryGrowExpr>(expr)));
      break;

    case ExprType::MemoryInit:
      CHECK_RESULT(delegate_->OnMemoryInitExpr(cast<MemoryInitExpr>(expr)));
      break;

    case ExprType::MemorySize:
      CHECK_RESULT(delegate_->OnMemorySizeExpr(cast<MemorySizeExpr>(expr)));
      break;

    case ExprType::TableCopy:
      CHECK_RESULT(delegate_->OnTableCopyExpr(cast<TableCopyExpr>(expr)));
      break;

    case ExprType::TableDrop:
      CHECK_RESULT(delegate_->OnTableDropExpr(cast<TableDropExpr>(expr)));
      break;

    case ExprType::TableInit:
      CHECK_RESULT(delegate_->OnTableInitExpr(cast<TableInitExpr>(expr)));
      break;

    case ExprType::Nop:
      CHECK_RESULT(delegate_->OnNopExpr(cast<NopExpr>(expr)));
      break;

    case ExprType::Rethrow:
      CHECK_RESULT(delegate_->OnRethrowExpr(cast<RethrowExpr>(expr)));
      break;

    case ExprType::Return:
      CHECK_RESULT(delegate_->OnReturnExpr(cast<ReturnExpr>(expr)));
      break;

    case ExprType::ReturnCall:
      CHECK_RESULT(delegate_->OnReturnCallExpr(cast<ReturnCallExpr>(expr)));
      break;

    case ExprType::ReturnCallIndirect:
      CHECK_RESULT(delegate_->OnReturnCallIndirectExpr(
          cast<ReturnCallIndirectExpr>(expr)));
      break;

    case ExprType::Select:
      CHECK_RESULT(delegate_->OnSelectExpr(cast<SelectExpr>(expr)));
      break;

    case ExprType::SetGlobal:
      CHECK_RESULT(delegate_->OnSetGlobalExpr(cast<SetGlobalExpr>(expr)));
      break;

    case ExprType::SetLocal:
      CHECK_RESULT(delegate_->OnSetLocalExpr(cast<SetLocalExpr>(expr)));
      break;

    case ExprType::Store:
      CHECK_RESULT(delegate_->OnStoreExpr(cast<StoreExpr>(expr)));
      break;

    case ExprType::TeeLocal:
      CHECK_RESULT(delegate_->OnTeeLocalExpr(cast<TeeLocalExpr>(expr)));
      break;

    case ExprType::Throw:
      CHECK_RESULT(delegate_->OnThrowExpr(cast<ThrowExpr>(expr)));
      break;

    case ExprType::Try:
      CHECK_RESULT(delegate_->OnTryExpr(cast<TryExpr>(expr)));
      break;

    case ExprType::Unary:
      CHECK_RESULT(delegate_->OnUnaryExpr(cast<UnaryExpr>(expr)));
      break;

    case ExprType::Ternary:
      CHECK_RESULT(delegate_->OnTernaryExpr(cast<TernaryExpr>(expr)));
      break;

    case ExprType::SimdLaneOp: {
      CHECK_RESULT(delegate_->OnSimdLaneOpExpr(cast<SimdLaneOpExpr>(expr)));
      break;
    }

    case ExprType::SimdShuffleOp: {
      CHECK_RESULT(
          delegate_->OnSimdShuffleOpExpr(cast<SimdShuffleOpExpr>(expr)));
      break;
    }

    case ExprType::Unreachable:
      CHECK_RESULT(delegate_->OnUnreachableExpr(cast<UnreachableExpr>(expr)));
      break;
  }

  return Result::Ok;
}

Result ExprVisitor::VisitExprList(ExprList& exprs) {
  for (Expr& expr : exprs) {
    CHECK_RESULT(VisitExpr(&expr));
  }
  return Result::Ok;
}

Result ExprVisitor::VisitFunc(Func* func) {
  return VisitExprList(func->exprs);
}

}  // namespace wabt
