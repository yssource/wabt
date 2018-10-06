/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include "gtest/gtest.h"

#include "include/wasm.h"
#include "include/wasm.hh"

using namespace wasm;

TEST(WasmApi, Store) {
  auto config = Config::make();
  auto engine = Engine::make(std::move(config));
  auto store = Store::make(engine.get());
}

TEST(WasmApi, ValType) {
  auto test_num = [](ValKind kind) {
    auto vt = ValType::make(kind);
    EXPECT_EQ(kind, vt->kind());
    EXPECT_TRUE(vt->is_num());
    EXPECT_FALSE(vt->is_ref());

    auto vt2 = vt->copy();
    EXPECT_EQ(vt2->kind(), vt->kind());
  };

  test_num(I32);
  test_num(I64);
  test_num(F32);
  test_num(F64);

  auto test_ref = [](ValKind kind) {
    auto vt = ValType::make(kind);
    EXPECT_EQ(kind, vt->kind());
    EXPECT_FALSE(vt->is_num());
    EXPECT_TRUE(vt->is_ref());

    auto vt2 = vt->copy();
    EXPECT_EQ(vt2->kind(), vt->kind());
  };

  test_ref(ANYREF);
  test_ref(FUNCREF);
}

using ValKindInitializerList = std::initializer_list<ValKind>;

vec<ValType*> MakeVecValType(ValKindInitializerList kinds) {
  auto v = vec<ValType*>::make_uninitialized(kinds.size());
  size_t i = 0;
  for (auto iter = kinds.begin(); iter != kinds.end(); ++iter) {
    v[i++] = ValType::make(*iter);
  }
  return v;
}

TEST(WasmApi, FuncType) {
  auto test = [](ValKindInitializerList expected_params,
                 ValKindInitializerList expected_results) {
    auto ft = FuncType::make(MakeVecValType(expected_params),
                             MakeVecValType(expected_results));
    const auto& cft = ft;

    // Test ExternType::*.
    EXPECT_EQ(EXTERN_FUNC, ft->kind());
    EXPECT_EQ(ft.get(), ft->func());
    EXPECT_EQ(nullptr, ft->global());
    EXPECT_EQ(nullptr, ft->table());
    EXPECT_EQ(nullptr, ft->memory());
    EXPECT_EQ(cft.get(), cft->func());
    EXPECT_EQ(nullptr, cft->global());
    EXPECT_EQ(nullptr, cft->table());
    EXPECT_EQ(nullptr, cft->memory());

    // TODO ExternType::copy.

    // Test FuncType::params().
    {
      const auto& actual_params = ft->params();
      EXPECT_EQ(expected_params.size(), actual_params.size());
      size_t i = 0;
      for (auto iter = expected_params.begin(); iter != expected_params.end();
           ++iter) {
        EXPECT_EQ(*iter, actual_params[i++]->kind());
      }
    }

    // Test FuncType::results().
    {
      const auto& actual_results = ft->results();
      EXPECT_EQ(expected_results.size(), actual_results.size());
      size_t i = 0;
      for (auto iter = expected_results.begin(); iter != expected_results.end();
           ++iter) {
        EXPECT_EQ(*iter, actual_results[i++]->kind());
      }
    }
  };

  test({}, {});

  for (auto kind : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
    test({kind}, {});
    test({}, {kind});
  }

  for (auto kind1 : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
    for (auto kind2 : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
      test({kind1, kind2}, {});
      test({kind1}, {kind2});
      test({}, {kind1, kind2});
    }
  }

  for (auto kind1 : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
    for (auto kind2 : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
      for (auto kind3 : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
        test({kind1, kind2, kind3}, {});
        test({kind1, kind2}, {kind3});
        test({kind1}, {kind2, kind3});
        test({}, {kind1, kind2, kind3});
      }
    }
  }
}

TEST(WasmApi, GlobalType) {
  auto test = [](ValKind kind, Mutability mut) {
    auto gt = GlobalType::make(ValType::make(kind), mut);
    const auto& cgt = gt;

    // Test ExternType::*.
    EXPECT_EQ(EXTERN_GLOBAL, gt->kind());
    EXPECT_EQ(nullptr, gt->func());
    EXPECT_EQ(gt.get(), gt->global());
    EXPECT_EQ(nullptr, gt->table());
    EXPECT_EQ(nullptr, gt->memory());
    EXPECT_EQ(nullptr, cgt->func());
    EXPECT_EQ(cgt.get(), cgt->global());
    EXPECT_EQ(nullptr, cgt->table());
    EXPECT_EQ(nullptr, cgt->memory());

    // Test GlobalType::*.
    EXPECT_EQ(kind, gt->content()->kind());
    EXPECT_EQ(mut, gt->mutability());
  };

  for (auto mut: {CONST, VAR}) {
    for (auto kind : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
      test(kind, mut);
    }
  }
}

TEST(WasmApi, TableType) {
  auto test = [](ValKind element_kind, Limits limits) {
    auto tt = TableType::make(ValType::make(element_kind), limits);
    const auto& ctt = tt;

    // Test ExternType::*.
    EXPECT_EQ(EXTERN_TABLE, tt->kind());
    EXPECT_EQ(nullptr, tt->func());
    EXPECT_EQ(nullptr, tt->global());
    EXPECT_EQ(tt.get(), tt->table());
    EXPECT_EQ(nullptr, tt->memory());
    EXPECT_EQ(nullptr, ctt->func());
    EXPECT_EQ(nullptr, ctt->global());
    EXPECT_EQ(ctt.get(), ctt->table());
    EXPECT_EQ(nullptr, ctt->memory());

    // Test TableType::*.
    EXPECT_EQ(element_kind, tt->element()->kind());
    EXPECT_EQ(limits.min, tt->limits().min);
    EXPECT_EQ(limits.max, tt->limits().max);
  };

  for (auto limits: {Limits(0), Limits(1), Limits(2, 3)}) {
    for (auto element_kind : {I32, I64, F32, F64, ANYREF, FUNCREF}) {
      test(element_kind, limits);
    }
  }
}

TEST(WasmApi, MemoryType) {
  auto test = [](Limits limits) {
    auto mt = MemoryType::make(limits);
    const auto& cmt = mt;

    // Test ExternType::*.
    EXPECT_EQ(EXTERN_MEMORY, mt->kind());
    EXPECT_EQ(nullptr, mt->func());
    EXPECT_EQ(nullptr, mt->global());
    EXPECT_EQ(nullptr, mt->table());
    EXPECT_EQ(mt.get(), mt->memory());
    EXPECT_EQ(nullptr, cmt->func());
    EXPECT_EQ(nullptr, cmt->global());
    EXPECT_EQ(nullptr, cmt->table());
    EXPECT_EQ(cmt.get(), cmt->memory());

    // Test MemoryType::*.
    EXPECT_EQ(limits.min, mt->limits().min);
    EXPECT_EQ(limits.max, mt->limits().max);
  };

  for (auto limits: {Limits(0), Limits(1), Limits(2, 3)}) {
    test(limits);
  }
}

Name MakeName(const char* name) {
  // TODO Name::make(std::string) will append an additional \0
  return Name::make(std::string(name));
}

TEST(WasmApi, ImportType) {
  auto test = [](const char* module, const char* name, const ExternType& type) {
    auto it = ImportType::make(MakeName(module), MakeName(name), type.copy());

    EXPECT_STREQ(module, it->module().get());
    EXPECT_STREQ(name, it->name().get());
  };

  auto ft = FuncType::make(MakeVecValType({}), MakeVecValType({}));
  auto gt = GlobalType::make(ValType::make(I32), CONST);
  auto tt = TableType::make(ValType::make(ANYREF), Limits(3, 5));
  auto mt = MemoryType::make(Limits(0, 4));

  test("mod", "func", *ft);
  test("mod", "global", *gt);
  test("mod", "table", *tt);
  test("mod", "memory", *mt);
}
