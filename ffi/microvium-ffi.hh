#pragma once

#include "../users.h"
#include <debug.hh>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <microvium/microvium.h>
#include <tuple>
#include <string.h>

using FFIDebug = ConditionalDebug<false, "FFI debug">;

/**
 * Code related to the JavaScript interpreter.
 */
namespace
{
	using CHERI::Capability;

	/**
	 * Constants for functions exposed to JavaScript->C++ FFI
	 *
	 * The values here must match the ones used in cheri.js.
	 */
	enum Exports : mvm_HostFunctionID
	{
		// "lawful" user api
		Login = 1,
		Logout,
		GetUsername,
		GetFullname,
		SetUsername,
		SetFullname,
		SetPassword,
		Print,
		// "illict" api
		// register manipulation
		Move = 11,
		LoadCapability,
		LoadInt,
		LoadString,
		StoreCapability,
		StoreInt,
		StoreString,
		// capability manipulation
		GetAddress = 21,
		SetAddress,
		GetBase,
		GetLength,
		GetPermissions,
		// user manipulation
		GetUserDetails = 31,
		GetToken,
		SetToken
	};

	/// Constant for the run function exposed to C++->JavaScript FFI
	static constexpr mvm_VMExportID ExportRun = 1234;

	/**
	 * Type used for the set of capabilities that JavaScript has complete
	 * control over.
	 */
	typedef struct VMState {
		std::array<void *, 8> registers;
		AccessToken current_token;
	} VMState;

	/**
	 * Address of the `AttackerRegisterState` on the stack.  This structure is
	 * on the stack so that it can hold local capabilities.
	 */
	ptraddr_t vmStateAddress;

	/**
	 * Re-derive the pointer to the on-stack register state.
	 */
	VMState &state()
	{
		register VMState *cspRegister asm("csp");
		asm("" : "=C"(cspRegister));
		Capability<VMState> rs{cspRegister};
		rs.address() = vmStateAddress;
		rs.bounds()  = sizeof(VMState);
		return *rs;
	}

	/**
	 * Helper for retrieving user details using `current_token` from the 
	 * VM state.
	 */
	inline User *get_current_user_details()
	{
        return get_user_details(state().current_token);
    }

	/**
	 * Write a capability into one of the VM's register set.
	 */
	void register_write(int regno, void *value)
	{
		if ((regno >= 0) && (regno < 8))
		{
			state().registers[regno] = value;
		}
	}

	/**
	 * Read a register from the VM's register set.  This reads one of the 8
	 * that can be written or CSP (8), CGP (9), or PCC (10).
	 */
	void *register_read(int regno)
	{
		switch (regno)
		{
			default:
				return nullptr;
			case 0 ... 7:
				return state().registers[regno];
			case 8:
			{
				register void *cspRegister asm("csp");
				asm("" : "=C"(cspRegister));
				FFIDebug::log("CSP: {}", cspRegister);
				return cspRegister;
			}
			case 9:
			{
				register void *cgpRegister asm("cgp");
				asm("" : "=C"(cgpRegister));
				FFIDebug::log("CGP: {}", cgpRegister);
				return cgpRegister;
			}
			case 10:
			{
				void *pcc;
				asm("auipcc %0, 0\n" : "=C"(pcc));
				FFIDebug::log("PCC: {}", pcc);
				return pcc;
			}
		}
	}

	/**
	 * Helper that maps from Exports
	 */
	template<Exports>
	constexpr static std::nullptr_t ExportedFn = nullptr;

	/**
	 * Template that returns JavaScript argument specified in `arg` as a C++
	 * type T.
	 */
	template<typename T>
	T extract_argument(mvm_VM *vm, mvm_Value arg);

	/**
	 * Specialisation to return integers.
	 */
	template<>
	__always_inline int32_t extract_argument<int32_t>(mvm_VM *vm, mvm_Value arg)
	{
		return mvm_toInt32(vm, arg);
	}

	/**
	 * Specialisation to return booleans.
	 */
	template<>
	__always_inline bool extract_argument<bool>(mvm_VM *vm, mvm_Value arg)
	{
		return mvm_toBool(vm, arg);
	}

	template<>
	__always_inline std::string extract_argument(mvm_VM *vm, mvm_Value arg)
	{	
		return mvm_toStringUtf8(vm, arg, nullptr);
	}

	/**
	 * Populate a tuple with arguments from an array of JavaScript values.
	 * This uses `extract_argument` to coerce each JavaScript value to the
	 * expected type.
	 */
	template<typename Tuple, int Idx = 0>
	__always_inline void
	args_to_tuple(Tuple &tuple, mvm_VM *vm, mvm_Value *args)
	{
		if constexpr (Idx < std::tuple_size_v<Tuple>)
		{
			std::get<Idx>(tuple) = extract_argument<
			  std::remove_reference_t<decltype(std::get<Idx>(tuple))>>(
			  vm, args[Idx]);
			args_to_tuple<Tuple, Idx + 1>(tuple, vm, args);
		}
	}

	/**
	 * Helper template to extract the arguments from a function type.
	 */
	template<typename T>
	struct FunctionSignature;

	/**
	 * The concrete specialisation that decomposes the function type.
	 */
	template<typename R, typename... Args>
	struct FunctionSignature<R(Args...)>
	{
		/**
		 * A tuple type containing all of the argument types of the function
		 * whose type is being extracted.
		 */
		using ArgumentType = std::tuple<Args...>;
	};

	/**
	 * The concrete specialisation that decomposes the function type for a cross
	 * compartment call.
	 */
	template<typename R, typename... Args>
	struct FunctionSignature<R __attribute__((cheri_ccall)) (Args...)>
	{
		/**
		 * A tuple type containing all of the argument types of the function
		 * whose type is being extracted.
		 */
		using ArgumentType = std::tuple<Args...>;
	};

	/**
	* Specialization for callable objects, including lambdas.
	*/
	template<typename T>
	struct FunctionSignature : FunctionSignature<decltype(&T::operator())> {};

	/**
	* Specialization for member function pointers, such as the call operator of lambdas.
	*/
	template<typename C, typename R, typename... Args>
	struct FunctionSignature<R(C::*)(Args...) const>
	{
		using ArgumentType = std::tuple<Args...>;
	};

	/**
	 * Call `Fn` with arguments from the Microvium arguments array.
	 *
	 * This is a wrapper that allows automatic forwarding from a function
	 * exported to JavaScript
	 */
	template<auto Fn>
	__always_inline mvm_TeError call_export(mvm_VM    *vm,
	                                        mvm_Value *result,
	                                        mvm_Value *args,
	                                        uint8_t    argsCount)
	{
		using TupleType = typename FunctionSignature<
		  std::remove_pointer_t<decltype(Fn)>>::ArgumentType;
		// Return an error if we have the wrong number of arguments.
		if (argsCount < std::tuple_size_v<TupleType>)
		{
			return MVM_E_UNEXPECTED;
		}
		// Get the arguments in a tuple.
		TupleType arguments;
		args_to_tuple(arguments, vm, args);
		// If this returns void, we don't need to do anything with the return.
		if constexpr (std::is_same_v<void, decltype(std::apply(Fn, arguments))>)
		{
			std::apply(Fn, arguments);
		}
		else
		{
			// Coerce the return type to a JavaScript object of the correct
			// type and return it.
			auto primitiveResult = std::apply(Fn, arguments);
			if constexpr (std::is_same_v<decltype(primitiveResult), bool>)
			{
				*result = mvm_newBoolean(primitiveResult);
			}
			if constexpr (std::is_same_v<decltype(primitiveResult), int32_t>)
			{
				*result = mvm_newInt32(vm, primitiveResult);
			}
			if constexpr (std::is_same_v<decltype(primitiveResult),
			                             std::string>)
			{
				*result = mvm_newString(
				  vm, primitiveResult.data(), primitiveResult.size());
			}
			if constexpr (std::is_same_v<decltype(primitiveResult), char*>)
			{
				*result = mvm_newString(
					vm, primitiveResult, strlen(primitiveResult)
				);
			}
		}
		return MVM_E_SUCCESS;
	}

	/**
	 * Base template for exported functions.  Forwards to the function defined
	 * with `ExportedFn<E>`.
	 */
	template<Exports E>
	mvm_TeError exported_function(mvm_VM *vm,
	                              mvm_HostFunctionID,
	                              mvm_Value *result,
	                              mvm_Value *args,
	                              uint8_t    argCount)
	{
		return call_export<ExportedFn<E>>(vm, result, args, argCount);
	}

	inline void uart_write(const char *str)
	{
		auto *uart = MMIO_CAPABILITY(Uart, uart);
		for (; *str != '\0'; str++)
		{
			uart->blocking_write(*str);
		}
	}

} // namespace
