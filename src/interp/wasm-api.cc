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
  return own<Config*>(seal<Config>(new ConfigImpl()));
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
  return own<Engine*>(seal<Engine>(new EngineImpl(std::move(config))));
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
  return own<Store*>(seal<Store>(new StoreImpl(engine)));
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

// static
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

// static
auto FuncType::make(vec<ValType*>&& params, vec<ValType*>&& results)
  -> own<FuncType*> {
  return params && results
             ? own<FuncType*>(seal<FuncType>(new FuncTypeImpl(params, results)))
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

// static
auto GlobalType::make(own<ValType*>&& content, Mutability mutability)
    -> own<GlobalType*> {
  return content ? own<GlobalType*>(seal<GlobalType>(
                       new GlobalTypeImpl(content, mutability)))
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

// static
auto TableType::make(own<ValType*>&& element, Limits limits)
    -> own<TableType*> {
  return element ? own<TableType*>(
                       seal<TableType>(new TableTypeImpl(element, limits)))
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

// static
auto MemoryType::make(Limits limits) -> own<MemoryType*> {
  return own<MemoryType*>(seal<MemoryType>(new MemoryTypeImpl(limits)));
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

// static
auto ImportType::make(Name&& module, Name&& name, own<ExternType*>&& type)
    -> own<ImportType*> {
  return module && name && type ? own<ImportType*>(seal<ImportType>(
                                      new ImportTypeImpl(module, name, type)))
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

// static
auto ExportType::make(Name&& name, own<ExternType*>&& type)
    -> own<ExportType*> {
  return name && type ? own<ExportType*>(
                            seal<ExportType>(new ExportTypeImpl(name, type)))
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

////////////////////////////////////////////////////////////////////////////////

struct HostData {
  HostData(void* info, void (*finalizer)(void*))
      : info(info), finalizer(finalizer) {}

  void* info;
  void (*finalizer)(void*);
  HostData* other = nullptr;
};

struct Object {
  Object() = default;

  ~Object() {
    HostData* data = host_data;
    while (data) {
      if (data->finalizer) {
        data->finalizer(data->info);
      }
      HostData* next = data->other;
      delete data;
      data = next;
    }
  }

  void addref() { ref_count++; }

  void release() {
    assert(ref_count > 0);
    if (--ref_count == 0) {
      delete this;
    }
  }

  int ref_count = 0;
  HostData* host_data = nullptr;
};

template <typename Ref, typename Obj>
struct RefImpl {
  RefImpl(Obj* obj) : obj(obj) { obj->addref(); }
  ~RefImpl() { obj->release(); }
  RefImpl(const RefImpl&) = delete;
  RefImpl& operator=(const RefImpl&) = delete;

  auto copy() const -> own<Ref*> {
    return make_own(seal<Ref>(new RefImpl<Ref, Obj>(obj)));
  }

  auto get_host_info() const -> void* {
    HostData* host_data = obj->host_data;
    return host_data ? host_data->info : nullptr;
  }

  void set_host_info(void* info, void (*finalizer)(void*)) {
    auto* new_host_data = new HostData(info, finalizer);
    new_host_data->other = obj->host_data;
    obj->host_data = new_host_data;
  }

  Obj* obj;
};

template<> struct implement<Ref> { using type = RefImpl<Ref, Object>; };

Ref::~Ref() {
  impl(this)->~RefImpl();
}

void Ref::operator delete(void* p) {
  ::operator delete(p);
}

auto Ref::copy() const -> own<Ref*> {
  return impl(this)->copy();
}

auto Ref::get_host_info() const -> void* {
  return impl(this)->get_host_info();
}

void Ref::set_host_info(void* info, void (*finalizer)(void*)) {
  return impl(this)->set_host_info(info, finalizer);
}

////////////////////////////////////////////////////////////////////////////////

struct TrapObject : Object {
  TrapObject(Message&& message) : message(std::move(message)) {}

  Message message;
};

using TrapRefImpl = RefImpl<Trap, TrapObject>;

template<> struct implement<Trap> { using type = TrapRefImpl; };

Trap::~Trap() {}

// static
auto Trap::make(Store* store, const Message& msg) -> own<Trap*> {
  return make_own(seal<Trap>(new TrapRefImpl(new TrapObject(msg.copy()))));
}

auto Trap::copy() const -> own<Trap*> {
  return impl(this)->copy();
}

auto Trap::message() const -> Message {
  return impl(this)->obj->message.copy();
}

////////////////////////////////////////////////////////////////////////////////

struct ForeignObject : Object {};

using ForeignRefImpl = RefImpl<Foreign, ForeignObject>;

template <> struct implement<Foreign> { using type = ForeignRefImpl; };

Foreign::~Foreign() {}

// static
auto Foreign::make(Store* store) -> own<Foreign*> {
  return make_own(seal<Foreign>(new ForeignRefImpl(new ForeignObject())));
}

auto Foreign::copy() const -> own<Foreign*> {
  return impl(this)->copy();
}

////////////////////////////////////////////////////////////////////////////////

struct ModuleObject : Object {
  ModuleObject(vec<byte_t>&& binary) : binary(std::move(binary)) {}

  vec<byte_t> binary;
};

using ModuleRefImpl = RefImpl<Module, ModuleObject>;

template<> struct implement<Module> { using type = ModuleRefImpl; };

Module::~Module() {}

// static
auto Module::validate(Store* store, const vec<byte_t>& binary) -> bool {
  // TODO
  return false;
}

// static
auto Module::make(Store* store, const vec<byte_t>& binary) -> own<Module*> {
  return make_own(
      seal<Module>(new ModuleRefImpl(new ModuleObject(binary.copy()))));
}

auto Module::copy() const -> own<Module*> {
  return impl(this)->copy();
}

auto Module::imports() const -> vec<ImportType*> {
  // TODO
  return vec<ImportType*>::make_uninitialized();
}

auto Module::exports() const -> vec<ExportType*> {
  // TODO
  return vec<ExportType*>::make_uninitialized();
}

auto Module::serialize() const -> vec<byte_t> {
  // TODO
  return vec<byte_t>::make_uninitialized();
}

// static
auto Module::deserialize(Store* store, const vec<byte_t>& serialized)
    -> own<Module*> {
  // TODO
  return make_own(
      seal<Module>(new ModuleRefImpl(new ModuleObject(serialized.copy()))));
}

template<> struct implement<Shared<Module>> { using type = vec<byte_t>; };

template<>
Shared<Module>::~Shared() {
  impl(this)->~vec();
}

template<>
void Shared<Module>::operator delete(void* p) {
  ::operator delete(p);
}

auto Module::share() const -> own<Shared<Module>*> {
  auto shared = seal<Shared<Module>>(new vec<byte_t>(serialize()));
  return make_own(shared);
}

// static
auto Module::obtain(Store* store, const Shared<Module>* shared)
    -> own<Module*> {
  return Module::deserialize(store, *impl(shared));
}

}  // namespace wasm
