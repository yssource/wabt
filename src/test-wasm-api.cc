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

    {
      const auto& actual_params = ft->params();
      EXPECT_EQ(expected_params.size(), actual_params.size());
      size_t i = 0;
      for (auto iter = expected_params.begin(); iter != expected_params.end();
           ++iter) {
        EXPECT_EQ(*iter, actual_params[i++]->kind());
      }
    }

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
