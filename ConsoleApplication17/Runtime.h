#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <array>
#include <unordered_map>
#include <stack>
#include <set>
#include <any>
#include <cassert>
#include <list>

#include "Poliz.h"

class Poliz;
class Parser;

using TID = uint64_t;
using REG = uint64_t;
using Hash = std::hash<std::string>;
using HashType = decltype(Hash{}(""));
#define INVALID_REG_VALUE ((uint64_t)-1)

enum class ERuntimeCallType {
	Invalid,

	Assign, // not called

	UnMinus,
	UnNot, // not called

	Add,
	Sub,
	Mult,
	Div,
	IntDiv,
	Remainder,

	CompareEq,
	CompareLess,

	CompareNotEq, // not called
	CompareLessEq, // not called
	CompareGreater, // not called
	CompareGreaterEq, // not called

	Or,
	And,

    ArrayAppend,
    ArrayAccess,
    ArraySize,

	MAX
};
inline std::string ERuntimeCallType_ToString(ERuntimeCallType c) {
	switch (c) {
	case ERuntimeCallType::Assign: return "Assign";
	case ERuntimeCallType::Add: return "Add";
	case ERuntimeCallType::Sub: return "Sub";
	case ERuntimeCallType::Mult: return "Mult";
	case ERuntimeCallType::Div: return "Div";
	case ERuntimeCallType::IntDiv: return "IntDiv";
	case ERuntimeCallType::CompareEq: return "CompareEq";
	case ERuntimeCallType::CompareNotEq: return "CompareNotEq";
	case ERuntimeCallType::CompareLess: return "CompareLess";
	case ERuntimeCallType::CompareLessEq: return "CompareLessEq";
	case ERuntimeCallType::CompareGreater: return "CompareGreater";
	case ERuntimeCallType::CompareGreaterEq: return "CompareGreaterEq";
	case ERuntimeCallType::Or: return "Or";
	case ERuntimeCallType::And: return "And";
	}
	return "";
}
inline ERuntimeCallType ERuntimeCallType_FromString(std::string str, bool isUnary = false) {
	if (str == "=") {
		return ERuntimeCallType::Assign;
	}
	else if (str == "+") {
		return ERuntimeCallType::Add;
	}
	else if (str == "-") {
		return isUnary ? ERuntimeCallType::UnMinus : ERuntimeCallType::Sub;
	}
	else if (str == "!") {
		return isUnary ? ERuntimeCallType::UnNot : ERuntimeCallType::Invalid;
	}
	else if (str == "/") {
		return ERuntimeCallType::Div;
	}
	else if (str == "%") {
		return ERuntimeCallType::Remainder;
	}
	else if (str == "//") {
		return ERuntimeCallType::IntDiv;
	}
	else if (str == "*") {
		return ERuntimeCallType::Mult;
	}
	else if (str == "==") {
		return ERuntimeCallType::CompareEq;
	}
	else if (str == "<=") {
		return ERuntimeCallType::CompareLessEq;
	}
	else if (str == "!=") {
		return ERuntimeCallType::CompareNotEq;
	}
	else if (str == "<") {
		return ERuntimeCallType::CompareLess;
	}
	else if (str == ">") {
		return ERuntimeCallType::CompareGreater;
	}
	else if (str == ">=") {
		return ERuntimeCallType::CompareGreaterEq;
	}
	else if (str == "||") {
		return ERuntimeCallType::Or;
	}
	else if (str == "&&") {
		return ERuntimeCallType::And;
	}
	assert(false);
	return ERuntimeCallType::Invalid;
}

class RuntimeMethod;
class RuntimeExecutor;
class RuntimeVar;
class RuntimeCtx;

enum class ERuntimeType {
	Null,
	//Int32, ))
	Int64,
	Double,
	String,
	Array,

	Custom,
	DEFAULT_MAX = Custom,
};

struct ByteStream {
private:
	std::vector<uint8_t> bf_write;
	uint32_t offset;

	inline void Write(const void* data, size_t dt) {
		bf_write.insert(bf_write.cend(), static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + dt);
	}

	inline void Read(void* data, size_t dt) {
		std::copy(bf_write.data() + this->offset, bf_write.data() + this->offset + dt, (uint8_t*)data);
		this->offset += dt;
	}

public:
	ByteStream() : offset(0) {}
	ByteStream(const std::vector<uint8_t>& st) : offset(0), bf_write(st) {}

	const std::vector<uint8_t>& GetBuffer() { return this->bf_write; }

	inline void Write(std::string val) {
		size_t sz = val.size();
		this->Write(&sz, sizeof(sz));
		this->Write(val.data(), sz);
	}
	inline void Write(const std::vector<uint8_t>& val) {
		bf_write.insert(bf_write.begin(), val.begin(), val.end());
	}

	template<typename T>
	inline void Write(const T& val) requires std::is_trivially_copyable_v<T>  {
		this->Write(&val, sizeof(val));
	}

	template<typename T>
	inline const T& Read() requires std::is_trivially_copyable_v<T>  {
		T val{};
		this->Read(&val, sizeof(val));
		return val;
	}

	template<typename T>
	inline std::string Read() {
		std::string val;
		size_t ln = this->Read<size_t>();
		val.reserve(ln);
		size_t rd = 0;
		while (rd < ln) {
			char chunk[128];
			size_t pr = std::min(ln - rd, sizeof chunk);
			this->Read(chunk, pr);
			val.insert(val.begin(), chunk, chunk + pr);
			rd += pr;
		}
		return val;
	}
};


struct RuntimeParamPack {
	std::vector<RuntimeVar*> vars;
};
class RuntimeType {
	friend class RuntimeVar;
	using OpCallType = RuntimeVar*(*)(RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2);
private:
	std::string name;
	TID id;
	ERuntimeType type;
	uint32_t size;

	std::function<void(RuntimeVar*, ByteStream&)> nativeCtor;
	std::function<bool(RuntimeVar*, RuntimeType*)> nativeTypeConvert;
	std::function<bool(RuntimeVar*)> nativeIsFalse;
	std::array<OpCallType, static_cast<int>(ERuntimeCallType::MAX)> vtable;

	bool NativeTypeConvert(RuntimeVar* var, RuntimeType* desType){
         return this->nativeTypeConvert(var, desType);
	}
	RuntimeVar* CallOperator(ERuntimeCallType type, RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) {
		int idx = static_cast<int>(type);
		if (!this->vtable[idx]) {
			assert(false);
			return nullptr;
		}
		return this->vtable[idx](ctx, exec, p1, p2);
	}
public:
	void NativeCtor(RuntimeVar* var, const std::vector<uint8_t>& rawData) {
		ByteStream stream(rawData);
		return this->nativeCtor(var, stream);
	}
	void NativeCtor(RuntimeVar* var, ByteStream& stream) {
		return this->nativeCtor(var, stream);
	}

	void SetNativeIsFalse(decltype(RuntimeType::nativeIsFalse) func) {
		this->nativeIsFalse = func;
	}
	void SetNativeCtor(decltype(RuntimeType::nativeCtor) func) {
		this->nativeCtor = func;
	}
	void SetNativeTypeConvert(decltype(RuntimeType::nativeTypeConvert) func) {
		this->nativeTypeConvert = func;
	}

	void SetOperator(ERuntimeCallType type, OpCallType func) {
		this->vtable[static_cast<int>(type)] = func;
	}

	bool HasIsFalseHandler() {
		return this->nativeIsFalse != nullptr;
	}
	bool IsFalse(RuntimeVar* var) {
		if(this->nativeIsFalse)
			return this->nativeIsFalse(var);
		assert(false);
		return false;
	}

	bool IsNativeCtorable() {
		return this->nativeCtor != nullptr;
	}
	TID GetTID() {
		return this->id;
	}
	std::string GetName() {
		return this->name;
	}
	ERuntimeType GetTypeEnum() {
		return this->type;
	}

	RuntimeType(const std::string& name_, ERuntimeType type_, uint32_t size_) : size(size_), name(name_), id(std::hash<std::string>{}(name)), type(type_) {
		this->nativeCtor = nullptr;
        this->nativeTypeConvert = nullptr;
		vtable.fill(0);
	}
};

class RuntimeExecutor;
class RuntimeCtx;
class RuntimeVar {
	RuntimeType* heldType;
public:
	union {
		int64_t i64;
		double dbl;
		struct {
			uint32_t size;
			char* ptr;
		} str;
		struct {
			uint32_t size;
			uint32_t cap;
			RuntimeVar** data;
		} arr;

		struct {
			void* dataBlob;
		} custom;
	} data;
	RuntimeVar() : heldType(0) {}

	void SetType(RuntimeType* type) { this->heldType = type; }
	RuntimeType* GetType() { return this->heldType; }

	bool NativeTypeConvert(RuntimeType* desType){
		if (this->heldType == desType) return true;
        if(this->heldType->NativeTypeConvert(this, desType)){
            this->heldType = desType;
            return true;
        }
        return false;
	}
	void NativeCtor(const std::vector<uint8_t>& rawData){
		return this->heldType->NativeCtor(this, rawData);
	}
	void NativeCtor(ByteStream& rawData) {
		return this->heldType->NativeCtor(this, rawData);
	}

	RuntimeVar* CallOperator(ERuntimeCallType type, RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p2) {
		return this->heldType->CallOperator(type, ctx, exec, this, p2);
	}

	RuntimeVar* CopyFrom(RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* other);


	bool IsFalse() {
		if(this->heldType->HasIsFalseHandler())
			return this->heldType->IsFalse(this);

		int64_t nl = 0;
		for (int i = 0; i < this->heldType->size; i += sizeof(nl)) {
			if (memcmp((uint8_t*)&this->data + i, &nl, this->heldType->size - i))
				return false;
		}
		return true;
	}
};

using RuntimeMethodPtr = RuntimeVar*(*)(RuntimeCtx*, RuntimeExecutor*, const std::vector<RuntimeVar*>&);
class RuntimeMethod {
public:
	RuntimeMethod() : anyParams(false), va(INVALID_REG_VALUE), native(nullptr) {

	}
	RuntimeMethod(const std::string& name_, const std::vector<std::string>& params_) : RuntimeMethod() {
		this->name = name_;
		this->params = params_;
	}
	RuntimeMethod(const std::string& name, const std::vector<std::string>& params_, RuntimeMethodPtr native) : RuntimeMethod(name, params_) {
		this->native = native;
	}
	RuntimeMethod(const std::string& name_) : RuntimeMethod() {
		this->name = name_;
		this->anyParams = true;
	}
	RuntimeMethod(const std::string& name, RuntimeMethodPtr native) : RuntimeMethod(name) {
		this->native = native;
	}

	void FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz);

	int GetParamCount() {
		return this->anyParams ? -1 : this->params.size();
	}
	const std::vector<std::string>& GetParamNames() {
		return this->params;
	}

	std::string GetName() {
		return this->name;
	}
	REG GetVA() {
		return this->va;
	}
	void SetVA(REG va) {
		this->va = va;
	}

	bool IsNative() { return this->native != nullptr; }
	RuntimeVar* NativeCall(RuntimeCtx* ctx, RuntimeExecutor* exec, const RuntimeParamPack& params) {
		return this->native(ctx, exec, params.vars);
	}

private:
	std::string name;
	bool anyParams;
	std::vector<std::string> params;

	RuntimeMethodPtr native;
	REG va;
};

enum class RuntimeInstrType {
	Invalid = 0,

	Ctor, // Ctor [ret] [tid] params...
	Operation, // Operation [ret] [operation(ERuntimeCallType)] param1 param2
	UnOperation, // Operation [ret] [operation(ERuntimeCallType)] param1
	Call, // Call [ret] [func] params...
	Array, // Array [ret] params...
	Jz, // Jz [delta] [var]
	Jge, // Jge [delta] [var]
	Jmp, // Jmp [delta]
	Ret, // Ret [value]
    ArraySize, // ArraySize [ret] [array]
    ArrayAccess, // ArrayAccess [ret] [idx] [idx]
};
inline std::string RuntimeInstrType_ToString(RuntimeInstrType c) {
	switch (c) {
	case RuntimeInstrType::Invalid: return "Invalid";
	case RuntimeInstrType::Ctor: return "Ctor";
	case RuntimeInstrType::Operation: return "Operation";
	case RuntimeInstrType::UnOperation: return "UnOperation";
	case RuntimeInstrType::Call: return "Call";
	case RuntimeInstrType::Array: return "Array";
	case RuntimeInstrType::Jz: return "Jz";
	case RuntimeInstrType::Jge: return "Jge";
	case RuntimeInstrType::Jmp: return "Jmp";
	case RuntimeInstrType::Ret: return "Ret";
	}
	return "";
}

struct RuntimeInstr {
	RuntimeInstrType opcode;

	template<typename T>
	void AddParam(T param) {
		static_assert(std::is_same_v<T, double> || std::is_same_v<T, std::string> || std::is_same_v<T, int64_t> || std::is_same_v<T, HashType> || std::is_same_v<T, TID> || std::is_same_v<T, ERuntimeCallType>);
		this->params.push_back(param);
	}

	template<typename T>
	const T& GetParam(size_t idx) const {
		return std::any_cast<const T&>(this->params[idx]);
	}
	template<typename T>
	T& RefParam(size_t idx) {
		return std::any_cast<T&>(this->params[idx]);
	}

	size_t GetParamCount() const {
		return this->params.size();
	}
	std::string GetReturnName() const {
		return this->GetParam<std::string>(0);
	}

	std::string GetParamString(RuntimeCtx* ctx, size_t idx) const;
	std::vector<uint8_t> GetRawParam(size_t idx) const;

	RuntimeInstr(RuntimeInstrType op) : opcode(op) {}
	RuntimeInstr() : RuntimeInstr(RuntimeInstrType::Invalid) {}
private:
	std::vector<std::any> params;
};

class RuntimeCtx
{
public:
	RuntimeCtx();
	~RuntimeCtx();

	void AddPoliz(Parser* parser, Poliz* root);
	void AddType(RuntimeType* type);
	void AddMethod(RuntimeMethod* method);
	
	RuntimeMethod* GetMethod(HashType methodName);
	RuntimeMethod* GetMethod(std::string methodName) {
		return this->GetMethod(Hash{}(methodName));
	}
	RuntimeType* GetType(TID typeName);
	RuntimeType* GetType(ERuntimeType typeEnum);

	RuntimeInstr* GetInstr(REG idx);
	RuntimeInstr* AllocateFunction(RuntimeMethod* method, size_t size);

	int64_t ExecuteRoot(std::string functionName);
    RuntimeExecutor* GetExecutor(){ return this->executor; }

	std::string GetErrorString();
	size_t GetCodeSize() { return this->instrHolder.size(); }
private:
	std::map<HashType, RuntimeMethod*> regMethods;
	std::map<TID, RuntimeType*> regTypes;
	std::array<RuntimeType*, (int)ERuntimeType::DEFAULT_MAX> defaultTypes;
	std::vector<RuntimeInstr> instrHolder;

	RuntimeExecutor* executor;
};


template<typename T>
struct VarPool {
	T* block;
	int size;
	std::list<T*> lst;
	
	VarPool() : block(nullptr), size(0) {}
	inline VarPool(RuntimeCtx* ctx, int length){
		this->block = new T[length];
		this->size = length;
		for(int i = 0; i < length; ++i){
            RuntimeVar* var = this->block + i;
            var->SetType(ctx->GetType(ERuntimeType::Null));
			this->lst.push_back(var);
		}
	}
	inline RuntimeVar* Pop(){
		if (this->lst.empty())
			return nullptr;
		RuntimeVar* var = this->lst.front();
		this->lst.pop_front();
		return var;
	}
	inline bool Return(RuntimeCtx* ctx, RuntimeVar* var) {
		assert(var >= this->block && var <= this->block + this->size);
		if (!var->NativeTypeConvert(ctx->GetType(ERuntimeType::Null)))
			assert(false); // type can't be constructed to Null??
		this->lst.push_back(var);
		return true;
	}
};

struct LocalVarState {
	RuntimeVar* var;
	//bool dirty;
};

struct LocalScope {
	std::unordered_map<std::string, LocalVarState> locals;
	std::unordered_map<std::string, LocalVarState> parentLocals;

public:
	LocalScope() = default;

	void Destroy(RuntimeExecutor* exec, RuntimeCtx* ctx);
	//LocalScope* GetParent() { return this->parent; }

	LocalVarState GetLocal(RuntimeExecutor* exec, RuntimeCtx* ctx, const std::string name);
	void SetLocal(RuntimeExecutor* exec, RuntimeCtx* ctx, const std::string& varName, RuntimeVar* next, bool isParent);

	/*inline bool PushParentVariable(const std::string& str) {
		if (!this->parent) return false;
		auto it = this->parent->locals.find(str);
		if (it == this->parent->locals.end()) return false;
	}*/
};

class RuntimeExecutor {
private:
	REG ip;
	REG lastErrorIp;
	std::stack<REG> stack;

	bool isErrored;
	std::string errorMessage;
	std::map<RuntimeVar*, VarPool<RuntimeVar>> varPool;
	LocalScope* currentScope;

	LocalVarState GetLocal(RuntimeCtx* ctx, const std::string& name);
	void SetLocal(RuntimeCtx* ctx, const std::string& varName, RuntimeVar* next);

	RuntimeVar* ExecuteInstr(RuntimeCtx* ctx, RuntimeInstr* instr);
public:
	~RuntimeExecutor() {
		if (this->currentScope) {
			delete this->currentScope;
			this->currentScope = nullptr;
		}
	}
	RuntimeExecutor() : ip(INVALID_REG_VALUE), lastErrorIp(INVALID_REG_VALUE), isErrored(false), currentScope(nullptr) {};
	void Reset() {
		this->isErrored = false;
		while (!this->stack.empty()) this->stack.pop();
		this->stack.push(INVALID_REG_VALUE);
	}

	void SetError(std::string errorMessage);
	bool IsErrored() { return this->isErrored; }
	std::string GetError() { return this->errorMessage; }

	RuntimeVar* CallMethod(RuntimeCtx* ctx, RuntimeMethod* method, const RuntimeParamPack& params);
	RuntimeVar* CreateVar(RuntimeCtx* ctx);
	RuntimeVar* CreateTypedVar(RuntimeCtx* ctx, RuntimeType* type);
	void ReturnVar(RuntimeCtx* ctx, RuntimeVar* var);
};

