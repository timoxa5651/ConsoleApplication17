#include "Precompile.h"

RuntimeType* Precompile::Type_Null() {
	RuntimeType* type = new RuntimeType("Null", ERuntimeType::Null, 0);
	type->SetNativeCtor([](RuntimeVar* var, const std::vector<uint8_t>& data) {

	});
	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {
		
		printf("NativeTypeConvert: %s -> %s\n", var->heldType->GetName().c_str(), type->GetName().c_str());
		return true;
	});
	return type;
}

RuntimeType* Precompile::Type_Int64() {
	RuntimeType* type = new RuntimeType("Int64", ERuntimeType::Int64, 8);
	type->SetNativeCtor([](RuntimeVar* var, const std::vector<uint8_t>& data) {
		//var->data.i64 = *(int64_t*)data;
		printf("Int64 ctor called!!!!\n");
	});
	type->SetNativeTypeConvert([](RuntimeVar* var, RuntimeType* type) -> bool {
		
		printf("NativeTypeConvert: %s -> %s\n", var->heldType->GetName().c_str(), type->GetName().c_str());
		return true;
	});
	return type;
}

RuntimeType* Precompile::Type_String() {
	RuntimeType* type = new RuntimeType("String", ERuntimeType::Null, 0);
	type->SetNativeCtor([](RuntimeVar* var, const std::vector<uint8_t>& data) {

	});
	return type;
}

void Precompile::AddReservedMethods(RuntimeCtx* ctx) {
	ctx->AddMethod(new RuntimeMethod("print", -1, [](RuntimeCtx* ctx, const std::vector<RuntimeVar*>& params) -> RuntimeVar* {
		printf("Script: print called...\n");
		return nullptr;
	}));

	ctx->AddMethod(new RuntimeMethod("read", -1, [](RuntimeCtx* ctx, const std::vector<RuntimeVar*>& params) -> RuntimeVar* {
		return nullptr;
	}));
}

void Precompile::CreateTypes(RuntimeCtx* ctx) {
	ctx->AddType(Precompile::Type_Null());
	ctx->AddType(Precompile::Type_Int64());
	ctx->AddType(Precompile::Type_String());

	AddReservedMethods(ctx);
}