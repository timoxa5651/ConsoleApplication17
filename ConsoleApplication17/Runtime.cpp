#include "Runtime.h"
#include "Precompile.h"
#include "Parser.h"
#include <cassert>
#include <queue>

void RuntimeMethod::FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz) {
	std::vector<RuntimeInstr> cmd;

	std::stack<PolizEntry> stack;

	int retCnt = 1;
	int ctorCnt = 1;

	auto CreateScriptingInst = [&cmd, &ctorCnt](const RuntimeInstr& inInstr, const PolizEntry& entry) -> std::string {
		if (entry.cmd == PolizCmd::Var) { // already scripted
			return entry.operand;
		}

		RuntimeInstr instr(RuntimeInstrType::Ctor);
		std::string retName = "$ctor" + std::to_string(ctorCnt++);
		instr.AddParam(retName);

		if (entry.cmd == PolizCmd::Str) {
			instr.AddParam<TID>(Hash{}("String"));
			instr.AddParam(entry.operand);
		}
		else if (entry.cmd == PolizCmd::ConstInt) {
			instr.AddParam<TID>(Hash{}("Int64"));
			instr.AddParam(std::stoll(entry.operand.c_str()));
		}
        else if (entry.cmd == PolizCmd::ConstDbl) {
            instr.AddParam<TID>(Hash{}("Double"));
            instr.AddParam<double>(std::stod(entry.operand.c_str()));
        }
		else if (entry.cmd == PolizCmd::Null) {
			instr.AddParam<TID>(Hash{}("Null"));
		}
		else {
			assert(false);
		}
		cmd.push_back(instr);
		return retName;
	};

	std::vector<int64_t> addrMap(poliz.size());
	for (int64_t i = 0; i < poliz.size(); ++i) {
		addrMap[i] = cmd.size();
		auto& entry = poliz[i];

		switch (entry.cmd) {
		case PolizCmd::Operation: {
			PolizEntry pr2 = stack.top();
			stack.pop();
			PolizEntry pr1 = stack.top();
			stack.pop();

			ERuntimeCallType operType = ERuntimeCallType_FromString(entry.operand);
			RuntimeInstr oper(RuntimeInstrType::Operation);

			std::string retName;
			if (operType == ERuntimeCallType::Assign) {
				assert(pr1.cmd == PolizCmd::Var);
				retName = pr1.operand;
			}
			else {
				retName = "$ret" + std::to_string(retCnt++);
			}
			oper.AddParam(retName);
			oper.AddParam(operType);

			oper.AddParam(CreateScriptingInst(oper, pr1));
			oper.AddParam(CreateScriptingInst(oper, pr2));

			stack.push(PolizEntry{ -1, PolizCmd::Var, retName, pr1.polizEntryIdx });
			cmd.push_back(oper);
			break;
		}
		case PolizCmd::UnOperation: {
			PolizEntry pr1 = stack.top();
			stack.pop();

			ERuntimeCallType operType = ERuntimeCallType_FromString(entry.operand, true);
			RuntimeInstr oper(RuntimeInstrType::UnOperation);

			std::string retName = "$ret" + std::to_string(retCnt++);;
			oper.AddParam(retName);
			oper.AddParam(operType);

			oper.AddParam(CreateScriptingInst(oper, pr1));

			stack.push(PolizEntry{ -1, PolizCmd::Var, retName, pr1.polizEntryIdx });
			cmd.push_back(oper);
			break;
		}
		case PolizCmd::Call: {
			RuntimeInstr call(RuntimeInstrType::Call);

			std::string retName = "$ret" + std::to_string(retCnt++);
			call.AddParam(retName);
			call.AddParam(entry.operand);

			RuntimeMethod* def = ctx->GetMethod(Hash{}(entry.operand));
			assert(def); // "No registered method found"
			int paramCount = def->GetParamCount();
			if (paramCount < 0) {
				PolizEntry paramCountDyn = stack.top();
				stack.pop();
				assert(paramCountDyn.cmd == PolizCmd::ConstInt);
				paramCount = std::stoi(paramCountDyn.operand);
			}
			for (int i = 0; i < paramCount; ++i) {
				PolizEntry pr = stack.top();
				stack.pop();
				call.AddParam(CreateScriptingInst(call, pr));
			}
			stack.push(PolizEntry{ -1, PolizCmd::Var, retName, entry.polizEntryIdx });

			cmd.push_back(call);
			break;
		}
		case PolizCmd::Array: {
			size_t arraySize = std::stoll(entry.operand.c_str());
			RuntimeInstr array(RuntimeInstrType::Array);

			std::string retName = "$ret" + std::to_string(retCnt++);
			array.AddParam(retName);

			for (size_t i = 0; i < arraySize; ++i) {
				PolizEntry pr = stack.top();
				stack.pop();
				array.AddParam(CreateScriptingInst(array, pr));
			}

			stack.push(PolizEntry{ -1, PolizCmd::Var, retName, entry.polizEntryIdx });
			cmd.push_back(array);
			break;
		}
        case PolizCmd::ArraySize: {
                RuntimeInstr array(RuntimeInstrType::ArraySize);

                std::string retName = "$ctor" + std::to_string(ctorCnt++);
                array.AddParam(retName);
                PolizEntry pr = stack.top();
                stack.pop();
                assert(pr.cmd == PolizCmd::Var);
                array.AddParam(CreateScriptingInst(array, pr));

                stack.push(PolizEntry{ -1, PolizCmd::Var, retName, entry.polizEntryIdx });
                cmd.push_back(array);
                break;
        }
        case PolizCmd::ArrayAccess:{
            RuntimeInstr array(RuntimeInstrType::ArrayAccess);

            std::string retName = "$ctor" + std::to_string(ctorCnt++);
            array.AddParam(retName);
            PolizEntry pr2 = stack.top();
            stack.pop();
            PolizEntry pr1 = stack.top();
            stack.pop();
            assert(pr2.cmd == PolizCmd::Var); // array
            array.AddParam(CreateScriptingInst(array, pr1));
            array.AddParam(CreateScriptingInst(array, pr2));

            stack.push(PolizEntry{ -1, PolizCmd::Var, retName, entry.polizEntryIdx });
            cmd.push_back(array);
            break;
        }
		case PolizCmd::Jz: {
			PolizEntry actionVar = stack.top();
			int64_t delta = std::stoll(entry.operand);
			RuntimeInstr jz(RuntimeInstrType::Jz);
			jz.AddParam<int64_t>(delta + i);
			jz.AddParam(CreateScriptingInst(jz, actionVar));

			cmd.push_back(jz);
			break;
		}
		case PolizCmd::Jump: {
			PolizEntry actionVar = stack.top();
			int64_t delta = std::stoll(entry.operand);
			RuntimeInstr jmp(RuntimeInstrType::Jmp);
			jmp.AddParam<int64_t>(delta + i);

			cmd.push_back(jmp);
			break;
		}
		case PolizCmd::Ret: {
			bool hasValue = std::stoll(entry.operand) != 0;
			RuntimeInstr ret(RuntimeInstrType::Ret);
			if (hasValue) {
				PolizEntry actionVar = stack.top();
				ret.AddParam(CreateScriptingInst(ret, actionVar));
			}
			else {
				ret.AddParam(CreateScriptingInst(ret, PolizEntry(-1, PolizCmd::Null, "", entry.polizEntryIdx)));
			}
			cmd.push_back(ret);
			break;
		}

		default: {
			stack.push(entry);
			break;
		}
		}
	}

	// jmp rebase
	for (int64_t i = 0; i < cmd.size(); ++i) {
		auto& instr = cmd[i];
		if (instr.opcode != RuntimeInstrType::Jmp && instr.opcode != RuntimeInstrType::Jz)
			continue;
		int64_t& base = instr.RefParam<int64_t>(0);
		base = addrMap[base] - i - 1;
	}

	//print
	for (auto& instr : cmd) {
		std::cout << RuntimeInstrType_ToString(instr.opcode) << "  ";
		for (int i = 0; i < instr.GetParamCount(); ++i) {
			std::cout << instr.GetParamString(ctx, i) << " ";
		}
		std::cout << std::endl;
	}

	// insert fn
	RuntimeInstr* alloc = ctx->AllocateFunction(this, cmd.size());
	std::copy(cmd.data(), cmd.data() + cmd.size(), alloc);
}

std::vector<uint8_t> RuntimeInstr::GetRawParam(size_t idx) const {
	auto& value = this->params[idx];

	ByteStream bf_write;

	if (value.type() == typeid(std::string))
		bf_write.Write(std::any_cast<std::string>(value));
	if (value.type() == typeid(int64_t))
		bf_write.Write(std::any_cast<int64_t>(value));
    if (value.type() == typeid(double))
        bf_write.Write(std::any_cast<double>(value));
	if (value.type() == typeid(HashType))
		bf_write.Write(std::any_cast<HashType>(value));
	if (value.type() == typeid(ERuntimeCallType))
		bf_write.Write(std::any_cast<ERuntimeCallType>(value));

	return bf_write.GetBuffer();
}
std::string RuntimeInstr::GetParamString(RuntimeCtx* ctx, size_t idx) const {
	auto& value = this->params[idx];
	if (value.type() == typeid(std::string))
		return std::any_cast<std::string>(value);
	if (value.type() == typeid(int64_t))
		return std::to_string(std::any_cast<int64_t>(value));
    if (value.type() == typeid(double))
        return std::to_string(std::any_cast<double>(value));
	if (value.type() == typeid(TID)) {
		if (ctx) {
			return ctx->GetType(std::any_cast<TID>(value))->GetName();
		}
		return std::to_string(std::any_cast<TID>(value));
	}
	if (value.type() == typeid(ERuntimeCallType))
		return ERuntimeCallType_ToString(std::any_cast<ERuntimeCallType>(value));
	std::cout << "Unknown ret type " << std::string(value.type().name()) << std::endl;
	assert(false);
}

RuntimeVar* RuntimeExecutor::CreateVar(RuntimeCtx* ctx) {
	for (auto& [begin, pool] : this->varPool) {
		RuntimeVar* var = pool.Pop();
		if (var) return var;
	}
	// create pool
	printf("[dbg] Allocating VarPool: %d\n", 1000);
	auto pool = VarPool<RuntimeVar>(ctx, 1000);
	this->varPool[pool.block] = pool;
	return this->CreateVar(ctx);
}
RuntimeVar* RuntimeExecutor::CreateTypedVar(RuntimeCtx* ctx, RuntimeType* type) {
	RuntimeVar* var = this->CreateVar(ctx);
	if (!var->NativeTypeConvert(type))
		assert(false);
	return var;
}

void RuntimeExecutor::ReturnVar(RuntimeCtx* ctx, RuntimeVar* var) {
	auto ownerPool = this->varPool.lower_bound(var);
	if (ownerPool == this->varPool.end() || ownerPool->first != var)
		--ownerPool;
	if (!ownerPool->second.Return(ctx, var)) {
		printf("ReturnVar: Pool errored\n");
	}
}

void LocalScope::Destroy(RuntimeExecutor* exec, RuntimeCtx* ctx) {
	for (auto& [n, state] : this->locals) {
		exec->ReturnVar(ctx, state.var);
	}
	this->locals.clear();
	this->parentLocals.clear();
}
LocalVarState LocalScope::GetLocal(RuntimeExecutor* exec, RuntimeCtx* ctx, const std::string name) {
	auto it = this->locals.find(name);
	if (it == this->locals.end()) {
		auto it2 = this->parentLocals.find(name);
		if (it2 != this->parentLocals.end()) {
			return it2->second;
		}
		LocalVarState state{};
		state.var = exec->CreateVar(ctx);
		this->locals[name] = state;
		return state;
	}
	return it->second;
}
void LocalScope::SetLocal(RuntimeExecutor* exec, RuntimeCtx* ctx, const std::string& varName, RuntimeVar* next, bool isParent) {
	if (isParent) {
		auto it = this->parentLocals.find(varName);
		if (it == this->parentLocals.end()) {
			LocalVarState state{};
			state.var = next;
			this->parentLocals[varName] = state;
		}
		else {
			assert(false); // ??
		}
		return;
	}

	auto it = this->locals.find(varName);
	if (it == this->locals.end()) {
		assert(false); // ??
		return;
	}
	LocalVarState& state = it->second;
	exec->ReturnVar(ctx, state.var);
	state.var = next;
}

LocalVarState RuntimeExecutor::GetLocal(RuntimeCtx* ctx, const std::string& name) {
	return this->currentScope->GetLocal(this, ctx, name);
}
void RuntimeExecutor::SetLocal(RuntimeCtx* ctx, const std::string& varName, RuntimeVar* next) {
	return this->currentScope->SetLocal(this, ctx, varName, next, false);
}

RuntimeVar* RuntimeExecutor::ExecuteInstr(RuntimeCtx* ctx, RuntimeInstr* instr) {
	//printf("Executing instruction %s\n", RuntimeInstrType_ToString(instr->opcode).c_str());

	if (instr->opcode == RuntimeInstrType::Ctor) {
		auto& bret = instr->GetParam<std::string>(0);
		LocalVarState local = this->GetLocal(ctx, bret);
		if (local.var->GetType()->GetTypeEnum() != ERuntimeType::Null)
			local.var->NativeTypeConvert(ctx->GetType(ERuntimeType::Null)); // reset var so we don't convert
		TID targetType = instr->GetParam<TID>(1);
		local.var->NativeTypeConvert(ctx->GetType(targetType));

		ByteStream writer;
		for (size_t i = 2; i < instr->GetParamCount(); ++i) {
			writer.Write(instr->GetRawParam(i));
		}
 		local.var->NativeCtor(writer.GetBuffer());
	}
	else if (instr->opcode == RuntimeInstrType::UnOperation) {
		auto& bret = instr->GetParam<std::string>(0);
		LocalVarState ret = this->GetLocal(ctx, bret);
		ERuntimeCallType callType = instr->GetParam<ERuntimeCallType>(1);

		const string& bp1 = instr->GetParam<std::string>(2);
		LocalVarState p1 = this->GetLocal(ctx, bp1);

		if (callType == ERuntimeCallType::UnNot) {
			RuntimeVar* newRet = this->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			newRet->data.i64 = !p1.var->IsFalse();
			this->SetLocal(ctx, bret, newRet);
		}
		else {
			RuntimeVar* newRet = p1.var->CallOperator(callType, ctx, this, nullptr);
			if (newRet) this->SetLocal(ctx, bret, newRet);
		}
	}
	else if (instr->opcode == RuntimeInstrType::Operation) {
		auto& bret = instr->GetParam<std::string>(0);
		LocalVarState ret = this->GetLocal(ctx, bret);
		ERuntimeCallType callType = instr->GetParam<ERuntimeCallType>(1);

		const string& btrg = instr->GetParam<std::string>(3);
		LocalVarState target = this->GetLocal(ctx, btrg); // 2nd operand
		if (callType == ERuntimeCallType::Assign) {
			if (ret.var->GetType()->GetTypeEnum() != ERuntimeType::Null)
				ret.var->NativeTypeConvert(ctx->GetType(ERuntimeType::Null));
			ret.var->CopyFrom(target.var);
		}
		else {
			const string& bp1 = instr->GetParam<std::string>(2);
			LocalVarState p1 = this->GetLocal(ctx, bp1);
			const string& bp2 = instr->GetParam<std::string>(3);
			LocalVarState p2 = this->GetLocal(ctx, bp2);

			if (callType == ERuntimeCallType::CompareNotEq) {
				RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareEq, ctx, this, p2.var);
				if (newRet) {
					newRet->data.i64 = !newRet->data.i64;
					this->SetLocal(ctx, bret, newRet);
				}
				else {
					this->SetError("Illegal operation: " + p1.var->GetType()->GetName() + " != " + p2.var->GetType()->GetName());
				}
			}
			else if (callType == ERuntimeCallType::CompareLessEq) {
				bool fail = true;
				RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareEq, ctx, this, p2.var);
				if (newRet) {
					if (newRet->data.i64) {
						this->SetLocal(ctx, bret, newRet);
						fail = false;
					}
					else {
						this->ReturnVar(ctx, newRet);
						RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareLess, ctx, this, p2.var);
						if (newRet) {
							this->SetLocal(ctx, bret, newRet);
							fail = false;
						}
					}
				}
				if (fail) {
					this->SetError("Illegal operation: " + p1.var->GetType()->GetName() + " <= " + p2.var->GetType()->GetName());
				}
			}
			else if (callType == ERuntimeCallType::CompareGreater) {
				bool fail = true;
				RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareEq, ctx, this, p2.var);
				if (newRet) {
					if (newRet->data.i64) {
						newRet->data.i64 = 0;
						this->SetLocal(ctx, bret, newRet);
						fail = false;
					}
					else {
						this->ReturnVar(ctx, newRet);
						RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareLess, ctx, this, p2.var);
						if (newRet) {
							newRet->data.i64 = !newRet->data.i64;
							this->SetLocal(ctx, bret, newRet);
							fail = false;
						}
					}
				}
				if (fail) {
					this->SetError("Illegal operation: " + p1.var->GetType()->GetName() + " > " + p2.var->GetType()->GetName());
				}
			}
			else if (callType == ERuntimeCallType::CompareGreaterEq) {
				bool fail = true;
				RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareEq, ctx, this, p2.var);
				if (newRet) {
					if (newRet->data.i64) {
						this->SetLocal(ctx, bret, newRet);
						fail = false;
					}
					else {
						this->ReturnVar(ctx, newRet);
						RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareLess, ctx, this, p2.var);
						if (newRet) {
							newRet->data.i64 = !newRet->data.i64;
							this->SetLocal(ctx, bret, newRet);
							fail = false;
						}
					}
				}
				if (fail) {
					this->SetError("Illegal operation: " + p1.var->GetType()->GetName() + " >= " + p2.var->GetType()->GetName());
				}
			}
            else if(callType == ERuntimeCallType::Or){
                RuntimeVar* ret = this->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
                ret->data.i64 = !p1.var->IsFalse() || !p2.var->IsFalse();
                this->SetLocal(ctx, bret, ret);
            }
            else if(callType == ERuntimeCallType::And){
                RuntimeVar* ret = this->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
                ret->data.i64 = !p1.var->IsFalse() && !p2.var->IsFalse();
                this->SetLocal(ctx, bret, ret);
            }
			else {
				RuntimeVar* newRet = p1.var->CallOperator(callType, ctx, this, p2.var);
				if (newRet) this->SetLocal(ctx, bret, newRet);
			}
		}
	}
	else if (instr->opcode == RuntimeInstrType::Call) {
		auto& bret = instr->GetParam<std::string>(0);
		LocalVarState local = this->GetLocal(ctx, bret);

		auto& methodName = instr->GetParam<std::string>(1);
		RuntimeParamPack params;
		for (size_t i = 2; i < instr->GetParamCount(); ++i) {
			params.vars.push_back(this->GetLocal(ctx, instr->GetParam<std::string>(i)).var);
		}

		REG nextIp = this->ip;

		RuntimeVar* ret = this->CallMethod(ctx, ctx->GetMethod(methodName), params);

		//if (local.var->heldType->GetTypeEnum() != ERuntimeType::Null) local.var->NativeTypeConvert(ctx->GetType(ERuntimeType::Null)); // destroy existing
		this->SetLocal(ctx, bret, ret);

		this->ip = nextIp;
	}
	else if (instr->opcode == RuntimeInstrType::Array) {

	}
	else if (instr->opcode == RuntimeInstrType::Jz) {
		auto& bcond = instr->GetParam<std::string>(1);
		LocalVarState state = this->GetLocal(ctx, bcond);
		
		if (state.var->IsFalse()) {
			this->ip += instr->GetParam<int64_t>(0);
		}
	}
	else if (instr->opcode == RuntimeInstrType::Jge) {
		/*auto& bcond = instr->GetParam<std::string>(1);
		LocalVarState state = this->GetLocal(ctx, bcond);
		bool fail = true;
		RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareEq, ctx, this, p2.var);
		if (newRet) {
			if (newRet->data.i64) {
				this->SetLocal(ctx, bret, newRet);
				fail = false;
			}
			else {
				this->ReturnVar(ctx, newRet);
				RuntimeVar* newRet = p1.var->CallOperator(ERuntimeCallType::CompareLess, ctx, this, p2.var);
				if (newRet) {
					newRet->data.i64 = !newRet->data.i64;
					this->SetLocal(ctx, bret, newRet);
					fail = false;
				}
			}
		}
		if (fail) {
			this->SetError("Illegal operation: " + p1.var->GetType()->GetName() + " >= " + p2.var->GetType()->GetName());
		}*/
	}
	else if (instr->opcode == RuntimeInstrType::Jmp) {
		this->ip += instr->GetParam<int64_t>(0);
	}
	else if (instr->opcode == RuntimeInstrType::Ret) {
		auto& bret = instr->GetParam<std::string>(0);
		LocalVarState state = this->GetLocal(ctx, bret);
		return state.var;
	}

	return nullptr;
}

RuntimeVar* RuntimeExecutor::CallMethod(RuntimeCtx* ctx, RuntimeMethod* method, const RuntimeParamPack& params) {
	if (method->IsNative()) {
		return method->NativeCall(ctx, this, params); // can be unnamed, later moved to scope in Ret
	}

	LocalScope* oldScope = this->currentScope;
	this->currentScope = new LocalScope();
	// push params
	auto& names = method->GetParamNames();
	for (size_t i = 0; i < method->GetParamCount(); ++i) {
		auto state = this->GetLocal(ctx, names[i]);
		state.var->CopyFrom(params.vars[i]);
		//this->currentScope->SetLocal(this, ctx, names[i], state.var, true); -- pass as reference
	}

	this->ip = method->GetVA();
	// scripted
	RuntimeVar* returnVar = nullptr;
	while (!this->isErrored) {
		RuntimeInstr* instr = ctx->GetInstr(this->ip);
		this->ip += 1;

		RuntimeVar* var = this->ExecuteInstr(ctx, instr);
		if (instr->opcode == RuntimeInstrType::Ret) {
			// handle Ret
			returnVar = var;
			break;
		}
	}
	if (returnVar) {
		RuntimeVar* retCopy = this->CreateVar(ctx);
		retCopy->CopyFrom(returnVar);
		returnVar = retCopy;
	}

	if (oldScope) {
		this->currentScope->Destroy(this, ctx);
		delete this->currentScope;
		this->currentScope = oldScope;
	} // don't destroy last scope so we can see main return value
	return returnVar;
}
void RuntimeExecutor::SetError(std::string errorMessage) {
	this->isErrored = true;
	this->errorMessage = errorMessage;
}

RuntimeCtx::RuntimeCtx() {
	this->executor = new RuntimeExecutor();
	Precompile::CreateTypes(this);

	// set default
	this->defaultTypes[(int)ERuntimeType::Int64] = this->GetType(Hash{}("Int64"));
	this->defaultTypes[(int)ERuntimeType::String] = this->GetType(Hash{}("String"));
	this->defaultTypes[(int)ERuntimeType::Null] = this->GetType(Hash{}("Null"));
	this->defaultTypes[(int)ERuntimeType::Array] = this->GetType(Hash{}("Array"));
	this->defaultTypes[(int)ERuntimeType::Double] = this->GetType(Hash{}("Double"));

}
RuntimeCtx::~RuntimeCtx() {
	if (this->executor) {
		delete this->executor;
		this->executor = nullptr;
	}
}

RuntimeMethod* RuntimeCtx::GetMethod(HashType methodName) {
	auto it = this->regMethods.find(methodName);
	if (it == this->regMethods.end())
		return nullptr;
	return it->second;
}
RuntimeType* RuntimeCtx::GetType(TID typeName) {
	auto it = this->regTypes.find(typeName);
	if (it == this->regTypes.end())
		return nullptr;
	return it->second;
}
RuntimeType* RuntimeCtx::GetType(ERuntimeType typeEnum) {
	return this->defaultTypes[static_cast<int>(typeEnum)];
}

void RuntimeCtx::AddPoliz(Parser* parser, Poliz* root) {
	// add types


	// add methods
	auto plist = parser->GetFunctions();

	for (auto& func : plist) {
		if (this->GetMethod(func.name)) {
			continue; // don't override
		}
		RuntimeMethod* method = new RuntimeMethod(func.name, func.argNames);
		this->regMethods[Hash{}(func.name)] = method;
	}

	for (auto& [h, method] : this->regMethods) {
		if (method->IsNative()) continue;
		const auto& pz = root->functionsRegistry[method->GetName()];
		std::cout << "--------            Generating method " << method->GetName() << std::endl;
		method->FromPoliz(this, pz.poliz);
		std::cout << std::endl;
	}
}

int64_t RuntimeCtx::ExecuteRoot(std::string functionName) {
	RuntimeMethod* method = this->GetMethod(functionName);
	if (!method) {
		return 1;
	}
	this->executor->Reset();
	RuntimeVar* ret = this->executor->CallMethod(this, method, RuntimeParamPack());
	if (!ret) {
		return 1;
	}
	if (ret->GetType()->GetTypeEnum() != ERuntimeType::Int64) {
		if (!ret->NativeTypeConvert(this->GetType(ERuntimeType::Int64))) {
			this->executor->SetError("Main should return Int64");
			return 1;
		}
	}
	return ret->data.i64;
}
std::string RuntimeCtx::GetErrorString() {
	return this->executor->GetError(); 
}

void RuntimeCtx::AddType(RuntimeType* type) {
	this->regTypes[type->GetTID()] = type;
}
void RuntimeCtx::AddMethod(RuntimeMethod* method) {
	this->regMethods[Hash{}(method->GetName())] = method;
}

RuntimeInstr* RuntimeCtx::AllocateFunction(RuntimeMethod* method, size_t size) {
	method->SetVA(this->instrHolder.size());
	this->instrHolder.resize(this->instrHolder.size() + size);
	return &this->instrHolder[this->instrHolder.size() - size];
}
RuntimeInstr* RuntimeCtx::GetInstr(REG idx) {
	return &this->instrHolder[idx];
}