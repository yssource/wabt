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

#include "src/common.h"

using namespace wasm;

// Included so we can use EXPECT_EQ with wasm types.
namespace wasm {

template <typename T>
struct vec_element_eq {
  bool operator()(const T& lhs, const T& rhs) const {
    return lhs == rhs;
  }
};

template <typename T>
struct vec_element_eq<T*> {
  bool operator()(const T* lhs, const T* rhs) const {
    return *lhs == *rhs;
  }
};

template <typename T>
bool operator==(const vec<T>& lhs, const vec<T>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  vec_element_eq<T> is_equal;
  for (size_t i = 0; i < lhs.size(); ++i) {
    if (!is_equal(lhs[i], rhs[i])) {
      return false;
    }
  }

  return true;
}

bool operator==(const Limits& lhs, const Limits& rhs) {
  return lhs.min == rhs.min && lhs.max == rhs.max;
}

bool operator==(const ValType& lhs, const ValType& rhs) {
  return lhs.kind() == rhs.kind();
}

bool operator==(const FuncType& lhs, const FuncType& rhs) {
  return lhs.params() == rhs.params() && lhs.results() == rhs.results();
}

bool operator==(const GlobalType& lhs, const GlobalType& rhs) {
  return *lhs.content() == *rhs.content() &&
         lhs.mutability() == rhs.mutability();
}

bool operator==(const TableType& lhs, const TableType& rhs) {
  return *lhs.element() == *rhs.element() &&
         lhs.limits() == rhs.limits();
}

bool operator==(const MemoryType& lhs, const MemoryType& rhs) {
  return lhs.limits() == rhs.limits();
}


bool operator==(const ExternType& lhs, const ExternType& rhs) {
  if (lhs.kind() != rhs.kind()) {
    return false;
  }

  switch (lhs.kind()) {
    case EXTERN_FUNC:
      return *lhs.func() == *rhs.func();
    case EXTERN_GLOBAL:
      return *lhs.global() == *rhs.global();
    case EXTERN_TABLE:
      return *lhs.table() == *rhs.table();
    case EXTERN_MEMORY:
      return *lhs.memory() == *rhs.memory();
  }
}

bool operator==(const ImportType& lhs, const ImportType& rhs) {
  return lhs.module() == rhs.module() && lhs.name() == rhs.name() &&
         *lhs.type() == *rhs.type();
}

bool operator==(const ExportType& lhs, const ExportType& rhs) {
  return lhs.name() == rhs.name() && *lhs.type() == *rhs.type();
}

} // namespace wasm

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

    EXPECT_EQ(*ft, *ft->copy());

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

    EXPECT_EQ(*gt, *gt->copy());

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

    EXPECT_EQ(*tt, *tt->copy());

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

    EXPECT_EQ(*mt, *mt->copy());

    // Test MemoryType::*.
    EXPECT_EQ(limits.min, mt->limits().min);
    EXPECT_EQ(limits.max, mt->limits().max);
  };

  for (auto limits: {Limits(0), Limits(1), Limits(2, 3)}) {
    test(limits);
  }
}

Name MakeName(const char* cstr) {
  size_t len = strlen(cstr);
  auto name = Name::make_uninitialized(len);
  memcpy(name.get(), cstr, len);
  return name;
}

bool NameEquals(const Name& name, const char* cstr) {
  size_t len = strlen(cstr);
  if (len != name.size()) {
    return false;
  }
  return strncmp(name.get(), cstr, len) == 0;
}

TEST(WasmApi, ImportType) {
  auto test = [](const char* module, const char* name, const ExternType& type) {
    auto it = ImportType::make(MakeName(module), MakeName(name), type.copy());

    EXPECT_TRUE(NameEquals(it->module(), module));
    EXPECT_TRUE(NameEquals(it->name(), name));
    EXPECT_EQ(type, *it->type());

    EXPECT_EQ(*it, *it->copy());
  };

  auto ft = FuncType::make(MakeVecValType({I32}), MakeVecValType({F32}));
  auto gt = GlobalType::make(ValType::make(I32), CONST);
  auto tt = TableType::make(ValType::make(ANYREF), Limits(3, 5));
  auto mt = MemoryType::make(Limits(0, 4));

  test("mod", "func", *ft);
  test("mod", "global", *gt);
  test("mod", "table", *tt);
  test("mod", "memory", *mt);
}

TEST(WasmApi, ExportType) {
  auto test = [](const char* name, const ExternType& type) {
    auto et = ExportType::make(MakeName(name), type.copy());

    EXPECT_TRUE(NameEquals(et->name(), name));
    EXPECT_EQ(type.kind(), et->type()->kind());
    EXPECT_EQ(type, *et->type());

    EXPECT_EQ(*et, *et->copy());
  };

  auto ft = FuncType::make(MakeVecValType({F32}), MakeVecValType({I32}));
  auto gt = GlobalType::make(ValType::make(I32), CONST);
  auto tt = TableType::make(ValType::make(ANYREF), Limits(3, 5));
  auto mt = MemoryType::make(Limits(0, 4));

  test("func", *ft);
  test("global", *gt);
  test("table", *tt);
  test("memory", *mt);
}

namespace {

struct Dummy {
  int a, b;
};

class WasmApiRefTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    auto config = Config::make();
    auto engine = Engine::make(std::move(config));
    store_ = Store::make(engine.get());

    finalizer_count_ = 0;
    info1_ = 42;
    info2_ = {314, 159};
  }

  virtual void TearDown() {
    EXPECT_EQ(2, finalizer_count_);
  }

  void TestRef(Ref* ref) {
    // Initially there is no host info.
    EXPECT_EQ(nullptr, ref->get_host_info());

    ref->set_host_info(&info1_, [](void* info) {
      EXPECT_EQ(&info1_, info);
      EXPECT_EQ(42, *static_cast<int*>(info));
      finalizer_count_++;
    });

    auto ref_copy = ref->copy();

    // Test that we can read the same host info on another ref copy.
    EXPECT_EQ(&info1_, ref->get_host_info());
    EXPECT_EQ(&info1_, ref_copy->get_host_info());

    // Test that both finalizers will be called.
    ref_copy->set_host_info(&info2_, [](void* info) {
      auto* dummy = static_cast<Dummy*>(info);
      EXPECT_EQ(&info2_, dummy);
      EXPECT_EQ(314, dummy->a);
      EXPECT_EQ(159, dummy->b);
      finalizer_count_++;
    });

    // Test that get_host_info returns the most recently set info.
    EXPECT_EQ(&info2_, ref->get_host_info());
    EXPECT_EQ(&info2_, ref_copy->get_host_info());
  }

  own<Store*> store_;

  // Use statics so the values can be checked by the captureless lambdas above.
  static int finalizer_count_;
  static int info1_;
  static Dummy info2_;
};

// static
int WasmApiRefTest::finalizer_count_;
// static
int WasmApiRefTest::info1_;
//static
Dummy WasmApiRefTest::info2_;

}  // end of anonymous namespace.

TEST_F(WasmApiRefTest, Trap) {
  const char str[] = "Hello, World!";
  auto trap = Trap::make(store_.get(), Message::make(std::string(str)));
  EXPECT_STREQ(str, trap->message().get());

  TestRef(trap.get());
}

TEST_F(WasmApiRefTest, Foreign) {
  auto foreign = Foreign::make(store_.get());
  TestRef(foreign.get());
}

TEST(WasmApi, ModuleImports) {
  // (import "m0" "f0" (func))
  // (import "m0" "f1" (func (param i32 f32 i64 f64) (result i32)))
  // (import "m0" "g0" (global i32))
  // (import "m1" "g1" (global (mut f32)))
  // (import "m1" "m" (memory 1))
  // (import "m1" "t" (table 2 anyfunc))
  byte_t data[] = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02,
      0x60, 0x00, 0x00, 0x60, 0x04, 0x7f, 0x7d, 0x7e, 0x7c, 0x01, 0x7f,
      0x02, 0x34, 0x06, 0x02, 0x6d, 0x30, 0x02, 0x66, 0x30, 0x00, 0x00,
      0x02, 0x6d, 0x30, 0x02, 0x66, 0x31, 0x00, 0x01, 0x02, 0x6d, 0x30,
      0x02, 0x67, 0x30, 0x03, 0x7f, 0x00, 0x02, 0x6d, 0x31, 0x02, 0x67,
      0x31, 0x03, 0x7d, 0x01, 0x02, 0x6d, 0x31, 0x01, 0x6d, 0x02, 0x00,
      0x01, 0x02, 0x6d, 0x31, 0x01, 0x74, 0x01, 0x70, 0x00, 0x02,
  };
  auto binary = vec<byte_t>::make(sizeof(data), data);
  auto config = Config::make();
  auto engine = Engine::make(std::move(config));
  auto store = Store::make(engine.get());
  auto module = Module::make(store.get(), binary);
  auto imports = module->imports();

  auto ft0 = FuncType::make(MakeVecValType({}), MakeVecValType({}));
  auto ft1 = FuncType::make(MakeVecValType({I32, F32, I64, F64}),
                            MakeVecValType({I32}));
  auto gt0 = GlobalType::make(ValType::make(I32), CONST);
  auto gt1 = GlobalType::make(ValType::make(F32), VAR);
  auto mt = MemoryType::make(Limits(1));
  auto tt = TableType::make(ValType::make(ANYREF), Limits(2));

  ASSERT_EQ(6u, imports.size());
  EXPECT_EQ(*ImportType::make(MakeName("m0"), MakeName("f0"), std::move(ft0)),
            *imports[0]);
  EXPECT_EQ(*ImportType::make(MakeName("m0"), MakeName("f1"), std::move(ft1)),
            *imports[1]);
  EXPECT_EQ(*ImportType::make(MakeName("m0"), MakeName("g0"), std::move(gt0)),
            *imports[2]);
  EXPECT_EQ(*ImportType::make(MakeName("m1"), MakeName("g1"), std::move(gt1)),
            *imports[3]);
  EXPECT_EQ(*ImportType::make(MakeName("m1"), MakeName("m"), std::move(mt)),
            *imports[4]);
  EXPECT_EQ(*ImportType::make(MakeName("m1"), MakeName("t"), std::move(tt)),
            *imports[5]);
}

TEST(WasmApi, ModuleImportsError) {
  // If the module can't be parsed, don't return any imports.
  byte_t data[] = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,  // magic + version

      0x02, 0x08,  // Import section, 8 bytes
      0x01,        // 1 import
      0x01, 0x6d,  // Module: "m"
      0x01, 0x67,  // Name: "g"
      0x03,        // Import kind: global
      0x7f, 0x00,  // Global type: i32, not mutable

      0x7f  // Invalid section id
  };
  auto binary = vec<byte_t>::make(sizeof(data), data);
  auto config = Config::make();
  auto engine = Engine::make(std::move(config));
  auto store = Store::make(engine.get());
  auto module = Module::make(store.get(), binary);
  auto imports = module->imports();

  EXPECT_FALSE(imports);
  EXPECT_EQ(nullptr, imports.get());
}

TEST(WasmApi, ModuleExports) {
  // (func (export "f0"))
  // (func (export "f1") (param i32 f32 i64 f64) (result i32) i32.const 0)
  // (global (export "g0") i32 (i32.const 0))
  // (global (export "g1") (mut f32) (f32.const 0))
  // (memory (export "m") 1)
  // (table (export "t") 2 anyfunc)
  byte_t data[] = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02, 0x60,
      0x00, 0x00, 0x60, 0x04, 0x7f, 0x7d, 0x7e, 0x7c, 0x01, 0x7f, 0x03, 0x03,
      0x02, 0x00, 0x01, 0x04, 0x04, 0x01, 0x70, 0x00, 0x02, 0x05, 0x03, 0x01,
      0x00, 0x01, 0x06, 0x0e, 0x02, 0x7f, 0x00, 0x41, 0x00, 0x0b, 0x7d, 0x01,
      0x43, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x07, 0x1d, 0x06, 0x02, 0x66, 0x30,
      0x00, 0x00, 0x02, 0x66, 0x31, 0x00, 0x01, 0x02, 0x67, 0x30, 0x03, 0x00,
      0x02, 0x67, 0x31, 0x03, 0x01, 0x01, 0x6d, 0x02, 0x00, 0x01, 0x74, 0x01,
      0x00, 0x0a, 0x09, 0x02, 0x02, 0x00, 0x0b, 0x04, 0x00, 0x41, 0x00, 0x0b,
  };
  auto binary = vec<byte_t>::make(sizeof(data), data);
  auto config = Config::make();
  auto engine = Engine::make(std::move(config));
  auto store = Store::make(engine.get());
  auto module = Module::make(store.get(), binary);
  auto exports = module->exports();

  auto ft0 = FuncType::make(MakeVecValType({}), MakeVecValType({}));
  auto ft1 = FuncType::make(MakeVecValType({I32, F32, I64, F64}),
                            MakeVecValType({I32}));
  auto gt0 = GlobalType::make(ValType::make(I32), CONST);
  auto gt1 = GlobalType::make(ValType::make(F32), VAR);
  auto mt = MemoryType::make(Limits(1));
  auto tt = TableType::make(ValType::make(ANYREF), Limits(2));

  ASSERT_EQ(6u, exports.size());
  EXPECT_EQ(*ExportType::make(MakeName("f0"), std::move(ft0)), *exports[0]);
  EXPECT_EQ(*ExportType::make(MakeName("f1"), std::move(ft1)), *exports[1]);
  EXPECT_EQ(*ExportType::make(MakeName("g0"), std::move(gt0)), *exports[2]);
  EXPECT_EQ(*ExportType::make(MakeName("g1"), std::move(gt1)), *exports[3]);
  EXPECT_EQ(*ExportType::make(MakeName("m"), std::move(mt)), *exports[4]);
  EXPECT_EQ(*ExportType::make(MakeName("t"), std::move(tt)), *exports[5]);
}

TEST(WasmApi, ModuleExportsError) {
  // If the module can't be parsed, don't return any exports.
  byte_t data[] = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,  // magic + version

      0x05, 0x03,  // Memory section, 3 bytes
      0x01,        // 1 memory
      0x00, 0x01,  // Initial size 0, no maximum
      0x07, 0x05,  // Export section, 5 bytes
      0x01,        // 1 export
      0x01, 0x6d,  // Name: "m"
      0x02, 0x00,  // kind: global, index: 0

      0x7f  // Invalid section id
  };
  auto binary = vec<byte_t>::make(sizeof(data), data);
  auto config = Config::make();
  auto engine = Engine::make(std::move(config));
  auto store = Store::make(engine.get());
  auto module = Module::make(store.get(), binary);
  auto exports = module->exports();

  EXPECT_FALSE(exports);
  EXPECT_EQ(nullptr, exports.get());
}
