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

#include "include/wasm.h"
#include "include/wasm.hh"

#include <cassert>

namespace wasm {

////////////////////////////////////////////////////////////////////////////////
// NOTE(wabt): Much of the code here is copied from wasm-v8.cc in
// rossberg/wasm-c-api, but modified to work with the wabt API.

template<class T>
void ignore(T) {}

template<class C> struct implement;

template<class C>
auto impl(C* x) -> typename implement <C>::type* {
  return reinterpret_cast<typename implement<C>::type*>(x);
}

template<class C>
auto impl(const C* x) -> const typename implement<C>::type* {
  return reinterpret_cast<const typename implement<C>::type*>(x);
}

template<class C>
auto seal(typename implement <C>::type* x) -> C* {
  return reinterpret_cast<C*>(x);
}

template<class C>
auto seal(const typename implement <C>::type* x) -> const C* {
  return reinterpret_cast<const C*>(x);
}


////////////////////////////////////////////////////////////////////////////////

struct ConfigImpl {};

template <> struct implement<Config> { using type = ConfigImpl; };

Config::~Config() {
  impl(this)->~ConfigImpl();
}

void Config::operator delete(void* p) {
  ::operator delete(p);
}

// static
auto Config::make() -> own<Config*> {
  return own<Config*>(seal<Config>(new (std::nothrow) ConfigImpl()));
}

////////////////////////////////////////////////////////////////////////////////

struct EngineImpl {
  EngineImpl(own<Config*>&& config) : config(std::move(config)) {}

  own<Config*> config;
};

template <> struct implement<Engine> { using type = EngineImpl; };

Engine::~Engine() {
  impl(this)->~EngineImpl();
}

void Engine::operator delete(void* p) {
  ::operator delete(p);
}

// static
auto Engine::make(own<Config*>&& config) -> own<Engine*> {
  return own<Engine*>(
      seal<Engine>(new (std::nothrow) EngineImpl(std::move(config))));
}

////////////////////////////////////////////////////////////////////////////////

struct StoreImpl {
  StoreImpl(Engine* engine) : engine(engine) {}

  Engine* engine;
};

template <> struct implement<Store> { using type = StoreImpl; };

Store::~Store() {
  impl(this)->~StoreImpl();
}

void Store::operator delete(void* p) {
  ::operator delete(p);
}

// static
auto Store::make(Engine* engine) -> own<Store*> {
  return own<Store*>(seal<Store>(new (std::nothrow) StoreImpl(engine)));
}

////////////////////////////////////////////////////////////////////////////////

struct ValTypeImpl {
  ValTypeImpl(ValKind kind) : kind(kind) {}

  ValKind kind;
};

template <> struct implement<ValType> { using type = ValTypeImpl; };

ValTypeImpl* valtypes[] = {
  new ValTypeImpl(I32),
  new ValTypeImpl(I64),
  new ValTypeImpl(F32),
  new ValTypeImpl(F64),
  new ValTypeImpl(ANYREF),
  new ValTypeImpl(FUNCREF),
};

ValType::~ValType() {}

void ValType::operator delete(void*) {}

auto ValType::make(ValKind k) -> own<ValType*> {
  return own<ValType*>(seal<ValType>(valtypes[k]));
}

auto ValType::copy() const -> own<ValType*> {
  return make(kind());
}

auto ValType::kind() const -> ValKind {
  return impl(this)->kind;
}

////////////////////////////////////////////////////////////////////////////////

struct ExternTypeImpl {
  explicit ExternTypeImpl(ExternKind kind) : kind(kind) {}
  virtual ~ExternTypeImpl() {}

  ExternKind kind;
};

template <>
struct implement<ExternType> {
  using type = ExternTypeImpl;
};

ExternType::~ExternType() {
  impl(this)->~ExternTypeImpl();
}

void ExternType::operator delete(void *p) {
  ::operator delete(p);
}

auto ExternType::copy() const -> own<ExternType*> {
  switch (kind()) {
    case EXTERN_FUNC: return func()->copy();
    case EXTERN_GLOBAL: return global()->copy();
    case EXTERN_TABLE: return table()->copy();
    case EXTERN_MEMORY: return memory()->copy();
  }
}

auto ExternType::kind() const -> ExternKind {
  return impl(this)->kind;
}

////////////////////////////////////////////////////////////////////////////////

struct FuncTypeImpl : ExternTypeImpl {
  FuncTypeImpl(vec<ValType*>& params, vec<ValType*>& results)
      : ExternTypeImpl(EXTERN_FUNC),
        params(std::move(params)),
        results(std::move(results)) {}

  vec<ValType*> params;
  vec<ValType*> results;
};

template<> struct implement<FuncType> { using type = FuncTypeImpl; };

FuncType::~FuncType() {}

auto FuncType::make(vec<ValType*>&& params, vec<ValType*>&& results)
  -> own<FuncType*> {
  return params && results
    ? own<FuncType*>(
        seal<FuncType>(new(std::nothrow) FuncTypeImpl(params, results)))
    : own<FuncType*>();
}

auto FuncType::copy() const -> own<FuncType*> {
  return make(params().copy(), results().copy());
}

auto FuncType::params() const -> const vec<ValType*>& {
  return impl(this)->params;
}

auto FuncType::results() const -> const vec<ValType*>& {
  return impl(this)->results;
}


auto ExternType::func() -> FuncType* {
  return kind() == EXTERN_FUNC
             ? seal<FuncType>(static_cast<FuncTypeImpl*>(impl(this)))
             : nullptr;
}

auto ExternType::func() const -> const FuncType* {
  return kind() == EXTERN_FUNC
             ? seal<FuncType>(static_cast<const FuncTypeImpl*>(impl(this)))
             : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

struct GlobalTypeImpl : ExternTypeImpl {
  GlobalTypeImpl(own<ValType*>& content, Mutability mutability)
      : ExternTypeImpl(EXTERN_GLOBAL),
        content(std::move(content)),
        mutability(mutability) {}

  own<ValType*> content;
  Mutability mutability;
};

template<> struct implement<GlobalType> { using type = GlobalTypeImpl; };

GlobalType::~GlobalType() {}

auto GlobalType::make(own<ValType*>&& content, Mutability mutability)
    -> own<GlobalType*> {
  return content ? own<GlobalType*>(seal<GlobalType>(
                       new (std::nothrow) GlobalTypeImpl(content, mutability)))
                 : own<GlobalType*>();
}

auto GlobalType::copy() const -> own<GlobalType*> {
  return make(content()->copy(), mutability());
}

auto GlobalType::content() const -> const ValType* {
  return impl(this)->content.get();
}

auto GlobalType::mutability() const -> Mutability {
  return impl(this)->mutability;
}

auto ExternType::global() -> GlobalType* {
  return kind() == EXTERN_GLOBAL
             ? seal<GlobalType>(static_cast<GlobalTypeImpl*>(impl(this)))
             : nullptr;
}

auto ExternType::global() const -> const GlobalType* {
  return kind() == EXTERN_GLOBAL
             ? seal<GlobalType>(static_cast<const GlobalTypeImpl*>(impl(this)))
             : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

struct TableTypeImpl : ExternTypeImpl {
  TableTypeImpl(own<ValType*>& element, Limits limits)
      : ExternTypeImpl(EXTERN_TABLE),
        element(std::move(element)),
        limits(limits) {}

  own<ValType*> element;
  Limits limits;
};

template<> struct implement<TableType> { using type = TableTypeImpl; };

TableType::~TableType() {}

auto TableType::make(own<ValType*>&& element, Limits limits)
    -> own<TableType*> {
  return element ? own<TableType*>(seal<TableType>(
                       new (std::nothrow) TableTypeImpl(element, limits)))
                 : own<TableType*>();
}

auto TableType::copy() const -> own<TableType*> {
  return make(element()->copy(), limits());
}

auto TableType::element() const -> const ValType* {
  return impl(this)->element.get();
}

auto TableType::limits() const -> const Limits& {
  return impl(this)->limits;
}

auto ExternType::table() -> TableType* {
  return kind() == EXTERN_TABLE
             ? seal<TableType>(static_cast<TableTypeImpl*>(impl(this)))
             : nullptr;
}

auto ExternType::table() const -> const TableType* {
  return kind() == EXTERN_TABLE
             ? seal<TableType>(static_cast<const TableTypeImpl*>(impl(this)))
             : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

struct MemoryTypeImpl : ExternTypeImpl {
  MemoryTypeImpl(Limits limits)
      : ExternTypeImpl(EXTERN_MEMORY), limits(limits) {}

  Limits limits;
};

template<> struct implement<MemoryType> { using type = MemoryTypeImpl; };

MemoryType::~MemoryType() {}

auto MemoryType::make(Limits limits) -> own<MemoryType*> {
  return own<MemoryType*>(
      seal<MemoryType>(new (std::nothrow) MemoryTypeImpl(limits)));
}

auto MemoryType::copy() const -> own<MemoryType*> {
  return MemoryType::make(limits());
}

auto MemoryType::limits() const -> const Limits& {
  return impl(this)->limits;
}

auto ExternType::memory() -> MemoryType* {
  return kind() == EXTERN_MEMORY
             ? seal<MemoryType>(static_cast<MemoryTypeImpl*>(impl(this)))
             : nullptr;
}

auto ExternType::memory() const -> const MemoryType* {
  return kind() == EXTERN_MEMORY
             ? seal<MemoryType>(static_cast<const MemoryTypeImpl*>(impl(this)))
             : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

struct ImportTypeImpl {
  ImportTypeImpl(Name& module, Name& name, own<ExternType*>& type)
      : module(std::move(module)),
        name(std::move(name)),
        type(std::move(type)) {}

  Name module;
  Name name;
  own<ExternType*> type;
};

template<> struct implement<ImportType> { using type = ImportTypeImpl; };

ImportType::~ImportType() {
  impl(this)->~ImportTypeImpl();
}

void ImportType::operator delete(void *p) {
  ::operator delete(p);
}

auto ImportType::make(Name&& module, Name&& name, own<ExternType*>&& type)
    -> own<ImportType*> {
  return module && name && type
             ? own<ImportType*>(seal<ImportType>(
                   new (std::nothrow) ImportTypeImpl(module, name, type)))
             : own<ImportType*>();
}

auto ImportType::copy() const -> own<ImportType*> {
  return make(module().copy(), name().copy(), type()->copy());
}

auto ImportType::module() const -> const Name& {
  return impl(this)->module;
}

auto ImportType::name() const -> const Name& {
  return impl(this)->name;
}

auto ImportType::type() const -> const ExternType* {
  return impl(this)->type.get();
}

////////////////////////////////////////////////////////////////////////////////

struct ExportTypeImpl {
  ExportTypeImpl(Name& name, own<ExternType*>& type)
      : name(std::move(name)), type(std::move(type)) {}

  Name name;
  own<ExternType*> type;
};

template<> struct implement<ExportType> { using type = ExportTypeImpl; };

ExportType::~ExportType() {
  impl(this)->~ExportTypeImpl();
}

void ExportType::operator delete(void *p) {
  ::operator delete(p);
}

auto ExportType::make(Name&& name, own<ExternType*>&& type)
    -> own<ExportType*> {
  return name && type ? own<ExportType*>(seal<ExportType>(
                            new (std::nothrow) ExportTypeImpl(name, type)))
                      : own<ExportType*>();
}

auto ExportType::copy() const -> own<ExportType*> {
  return make(name().copy(), type()->copy());
}

auto ExportType::name() const -> const Name& {
  return impl(this)->name;
}

auto ExportType::type() const -> const ExternType* {
  return impl(this)->type.get();
}

}  // namespace wasm
