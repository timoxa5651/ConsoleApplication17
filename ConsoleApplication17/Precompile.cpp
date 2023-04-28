#include "Precompile.h"

RuntimeType* Precompile::Type_Null() {
	RuntimeType* type = new RuntimeType("Null", ERuntimeType::Null, 0);
	type->SetNativeCtor([](RuntimeVar* var, ByteStream& stream) {

	});
	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {
		
		var->data.i64 = 0;
		// default constructors for types
		if (type->GetTypeEnum() == ERuntimeType::String) {
			var->data.str.ptr = nullptr;
			var->data.str.size = 0;
		}
		else if (type->GetTypeEnum() == ERuntimeType::Array) {
			var->data.arr.size = 0;
			var->data.arr.cap = 0;
			var->data.arr.data = nullptr;
		}

		//printf("NativeTypeConvert: %s -> %s\n", var->GetType()->GetName().c_str(), type->GetName().c_str());
		return true;
	});
	return type;
}

RuntimeType* Precompile::Type_Int64() {
	RuntimeType* type = new RuntimeType("Int64", ERuntimeType::Int64, 8);
	type->SetNativeCtor([](RuntimeVar* var, ByteStream& stream) {
		var->data.i64 = stream.Read<int64_t>();
		//printf("Int64: %p -> %d\n", var, var->data.i64);
	});

	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {
		//printf("NativeTypeConvert: %s -> %s\n", var->GetType()->GetName().c_str(), type->GetName().c_str());
		if (type->GetTypeEnum() == ERuntimeType::Null) { // Int64 -> Null
			var->data.i64 = 0;
			return true;
		}
		else if (type->GetTypeEnum() == ERuntimeType::String) { // Int64 -> String
			ByteStream stream;
			stream.Write(std::to_string(var->data.i64));
			type->NativeCtor(var, stream);
			return true;
		}
		else if (type->GetTypeEnum() == ERuntimeType::Double) {
			var->data.dbl = var->data.i64;
			return true;
		}

		return false;
	});

	type->SetOperator(ERuntimeCallType::Add, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 + p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.i64 + p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " + " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Sub, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 - p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.i64 - p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " - " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Mult, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 * p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.i64 * p2->data.dbl;
			return ret;
		}
		else if (targetType == ERuntimeType::String) {
			if (p1->data.i64 <= 0) {
				exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " * " + p2->GetType()->GetName() + ", int should be positive");
				return nullptr;
			}
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::String));
			std::string s;
			std::string cp = std::string(p2->data.str.ptr, p2->data.str.ptr + p2->data.str.size);
			for (int64_t i = 0; i < p1->data.i64; ++i) {
				s += cp;
			}
			ByteStream stream;
			stream.Write(s);
			ret->GetType()->NativeCtor(ret, stream);
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " * " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Div, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
            if(p2->data.i64 == 0){
                exec->SetError("Division by zero");
                return nullptr;
            }
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = (double)p1->data.i64 / p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
            if(p2->data.dbl == 0){
                exec->SetError("Division by zero");
                return nullptr;
            }
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.i64 / p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " / " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::IntDiv, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 / p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 / p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " // " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Remainder, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 % p2->data.i64;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " % " + p2->GetType()->GetName());
		return nullptr;
	});

	type->SetOperator(ERuntimeCallType::UnMinus, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
		ret->data.i64 = -p1->data.i64;
		return ret;
	});

	type->SetOperator(ERuntimeCallType::CompareEq, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
		ret->data.i64 = 0;
		if (targetType == ERuntimeType::Int64) {
			ret->data.i64 = p1->data.i64 == p2->data.i64;
		}
		else if (targetType == ERuntimeType::Double) {
			ret->data.i64 = (double)p1->data.i64 == p2->data.dbl;
		}
		return ret;
	});
	type->SetOperator(ERuntimeCallType::CompareLess, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 < p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.i64 < p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " < " + p2->GetType()->GetName());
		return nullptr;
	});
	return type;
}

RuntimeType* Precompile::Type_Double() {
	RuntimeType* type = new RuntimeType("Double", ERuntimeType::Double, 8);
	type->SetNativeCtor([](RuntimeVar* var, ByteStream& stream) {
		var->data.dbl = stream.Read<double>();
		//printf("Double: %p -> %.2f\n", var, (float)var->data.dbl);
	});

	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {
		//printf("NativeTypeConvert: %s -> %s\n", var->GetType()->GetName().c_str(), type->GetName().c_str());

		if (type->GetTypeEnum() == ERuntimeType::Null) { // Double -> Null
			var->data.dbl = 0;
			return true;
		}
		else if (type->GetTypeEnum() == ERuntimeType::String) { // Double -> String
			ByteStream stream;
			stream.Write(std::to_string(var->data.dbl));
			type->NativeCtor(var, stream);
			return true;
		}
		else if (type->GetTypeEnum() == ERuntimeType::Int64) { // Double -> Int64
			var->data.i64 = var->data.dbl;
			return true;
		}

		return false;
	});

	type->SetOperator(ERuntimeCallType::Add, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl + p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl + p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " + " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Sub, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl - p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl - p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " - " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Mult, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl * p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl * p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " * " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Div, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
            if(p2->data.i64 == 0){
                exec->SetError("Division by zero");
                return nullptr;
            }
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl / p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
            if(p2->data.dbl == 0){
                exec->SetError("Division by zero");
                return nullptr;
            }
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
			ret->data.dbl = p1->data.dbl / p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " / " + p2->GetType()->GetName());
		return nullptr;
		});
	type->SetOperator(ERuntimeCallType::IntDiv, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " // " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Remainder, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " % " + p2->GetType()->GetName());
		return nullptr;
	});

	type->SetOperator(ERuntimeCallType::UnMinus, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Double));
		ret->data.dbl = -p1->data.dbl;
		return ret;
	});

	type->SetOperator(ERuntimeCallType::CompareEq, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
		ret->data.i64 = 0;
		if (targetType == ERuntimeType::Int64) {
			ret->data.i64 = p1->data.dbl == (double)p2->data.i64;
		}
		else if (targetType == ERuntimeType::Double) {
			ret->data.i64 = p1->data.dbl == p2->data.dbl;
		}
		return ret;
	});
	type->SetOperator(ERuntimeCallType::CompareLess, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.dbl < p2->data.i64;
			return ret;
		}
		else if (targetType == ERuntimeType::Double) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			ret->data.i64 = p1->data.dbl < p2->data.dbl;
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " < " + p2->GetType()->GetName());
		return nullptr;
	});
	return type;
}

RuntimeType* Precompile::Type_String() {
	RuntimeType* type = new RuntimeType("String", ERuntimeType::String, sizeof(RuntimeVar::data.str));
	type->SetNativeCtor([](RuntimeVar* var, ByteStream& stream) {
		std::string str = stream.Read<std::string>();
		var->data.str.size = str.size();
		var->data.str.ptr = new char[str.size() + 1];
		memcpy(var->data.str.ptr, str.c_str(), var->data.str.size);
		var->data.str.ptr[var->data.str.size] = 0;
	});
	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {

		if (type->GetTypeEnum() == ERuntimeType::Null) { // String -> Null
			if (var->data.str.ptr) {
				delete[] var->data.str.ptr;
				var->data.str.ptr = 0;
			}
			var->data.str.size = 0;
			return true;
		}

		return false;
	});


	type->SetOperator(ERuntimeCallType::Add, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::String) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::String));
			std::string s = std::string(p1->data.str.ptr, p1->data.str.ptr + p1->data.str.size) + std::string(p2->data.str.ptr, p2->data.str.ptr + p2->data.str.size);
			ByteStream stream;
			stream.Write(s);
			ret->GetType()->NativeCtor(ret, stream);
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " + " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Sub, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " - " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Mult, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::Int64) {
			if (p2->data.i64 <= 0) {
				exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " * " + p2->GetType()->GetName() + ", int should be positive");
				return nullptr;
			}
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::String));
			std::string s;
			std::string cp = std::string(p1->data.str.ptr, p1->data.str.ptr + p1->data.str.size);
			for (int64_t i = 0; i < p2->data.i64; ++i) {
				s += cp;
			}
			ByteStream stream;
			stream.Write(s);
			ret->GetType()->NativeCtor(ret, stream);
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " * " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Div, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " / " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::IntDiv, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " // " + p2->GetType()->GetName());
		return nullptr;
	});
	type->SetOperator(ERuntimeCallType::Remainder, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " % " + p2->GetType()->GetName());
		return nullptr;
	});

	type->SetOperator(ERuntimeCallType::UnMinus, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		exec->SetError("Illegal operation: - for " + p1->GetType()->GetName());
		return nullptr;
	});

	type->SetOperator(ERuntimeCallType::CompareEq, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
		ret->data.i64 = 0;
		if (targetType == ERuntimeType::String) {
			ret->data.i64 = p1->data.str.size == p2->data.str.size && (p1->data.str.ptr == p2->data.str.ptr || !memcmp(p1->data.str.ptr, p2->data.str.ptr, p1->data.str.size));
		}
		return ret;
	});
	type->SetOperator(ERuntimeCallType::CompareLess, [](RuntimeCtx* ctx, RuntimeExecutor* exec, RuntimeVar* p1, RuntimeVar* p2) -> RuntimeVar* {
		ERuntimeType targetType = p2->GetType()->GetTypeEnum();
		if (targetType == ERuntimeType::String) {
			RuntimeVar* ret = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
			if (!p1->data.str.ptr || !p2->data.str.ptr) {
				ret->data.i64 = 0;
			}
			else if(p1->data.str.size < p2->data.str.size) {
				ret->data.i64 = 1;
			}
			else {
				ret->data.i64 = memcmp(p1->data.str.ptr, p2->data.str.ptr, p1->data.str.size) > 0;
			}
			return ret;
		}
		exec->SetError("Illegal operation: " + p1->GetType()->GetName() + " < " + p2->GetType()->GetName());
		return nullptr;
	});

	return type;
}
RuntimeType* Precompile::Type_Array() {
	RuntimeType* type = new RuntimeType("Array", ERuntimeType::Array, sizeof(RuntimeVar::data.arr));

	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {
        if(type->GetTypeEnum() == ERuntimeType::Null){
            if(var->data.arr.data){
                delete var->data.arr.data;
                var->data.arr.data = 0;
            }
            var->data.arr.size = var->data.arr.cap = 0;
        }
		return false;
	});
    type->SetNativeCtor([](RuntimeVar* var, ByteStream& stream) {
        int64_t cap = stream.Read<int64_t>();
        var->data.arr.size = 0;
        var->data.arr.data = new RuntimeVar*[cap];
        var->data.arr.cap = cap;
    });

	return type;
}

void Precompile::AddReservedMethods(RuntimeCtx* ctx) {
    ctx->AddMethod(new RuntimeMethod("print", [](RuntimeCtx *ctx, RuntimeExecutor *exec,
                                                 const std::vector<RuntimeVar *> &params) -> RuntimeVar * {
        printf("[Script] ");
        int arg = 1;
        for (auto &param: params) {
            RuntimeVar *str = exec->CreateVar(ctx);
            str->CopyFrom(ctx, ctx->GetExecutor(), param);
            if (param->GetType()->GetTypeEnum() == ERuntimeType::Null ||
                !str->NativeTypeConvert(ctx->GetType(ERuntimeType::String))) {
                exec->SetError("print: Invalid argument " + std::to_string(arg) + ", not string");
            } else {
                printf("%s ", str->data.str.ptr);
            }
            exec->ReturnVar(ctx, str);
            arg += 1;
        }
        printf("\n");
        return exec->CreateVar(ctx);
    }));

    ctx->AddMethod(new RuntimeMethod("read", [](RuntimeCtx *ctx, RuntimeExecutor *exec,
                                                const std::vector<RuntimeVar *> &params) -> RuntimeVar * {
        std::string str;
        std::cin >> str;
        RuntimeVar *var = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::String));
        ByteStream stream;
        stream.Write(str);
        var->NativeCtor(stream);
        return var;
    }));

    ctx->AddMethod(new RuntimeMethod("int", {"i"}, [](RuntimeCtx *ctx, RuntimeExecutor *exec,
                                               const std::vector<RuntimeVar *> &params) -> RuntimeVar * {
        if (params[0]->GetType()->GetTypeEnum() != ERuntimeType::String) {
            exec->SetError("Failed to convert to int");
            return 0;
        }
        std::string str = params[0]->data.str.ptr;

        ByteStream stream;
        try {
            stream.Write(std::stoll(str));
        }
        catch (...) {
            exec->SetError("Failed to convert to int");
            return 0;
        }
        RuntimeVar *var = exec->CreateTypedVar(ctx, ctx->GetType(ERuntimeType::Int64));
        var->NativeCtor(stream);
        return var;
    }));
}

void Precompile::CreateTypes(RuntimeCtx* ctx) {
	ctx->AddType(Precompile::Type_Null());
	ctx->AddType(Precompile::Type_Int64());
	ctx->AddType(Precompile::Type_Double());
	ctx->AddType(Precompile::Type_String());
    ctx->AddType(Precompile::Type_Array());

	AddReservedMethods(ctx);
}