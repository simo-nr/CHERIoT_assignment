#include "cdefs.h"
#include "ffi/legal.hh"
#include "ffi/exploit.hh"
#include "ffi/end.hh"
#include "users.h"
#include <allocator.h>
#include <compartment.h>
#include <cstdint>
#include <cstdlib>
#include <debug.hh>
#include <riscvreg.h>
#include <type_traits>
#include <vector>

#include "jsvm.h"

using Debug = ConditionalDebug<true, "JSVM compartment">;


int run_js_bytecode(const uint8_t *bytecode, size_t bytecode_len)
{
	// Allocate the space for the VM capability registers on the stack and
	// record its location.
	// **Note**: This must be on the stack and in same compartment as the
	// JavaScript interpreter, so that the callbacks can re-derive it from
	// csp.
	VMState state;
	state.current_token = -1;
	vmStateAddress = Capability{&state}.address();

	mvm_TeError                         err;
	std::unique_ptr<mvm_VM, MVMDeleter> vm;
	// Create a Microvium VM from the bytecode.
	{
		mvm_VM *rawVm;
		err = mvm_restore(
			&rawVm,            /* Out pointer to the VM */
			(void *)bytecode,          /* Bytecode data */
			bytecode_len,      /* Bytecode length */
			MALLOC_CAPABILITY, /* Capability used to allocate memory */
			::resolve_import); /* Callback used to resolve FFI imports */
		// If this is not valid bytecode, give up.
		if (err != MVM_E_SUCCESS)
		{
			Debug::log("Failed to parse bytecode: {}", err);
			return -1;
		}
		vm.reset(rawVm);
	}

	// Get a handle to the JavaScript `run` function.
	mvm_Value run;
	// If `run` cannot be resolved, report this as an error.
	err = mvm_resolveExports(vm.get(), &ExportRun, &run, 1);
	if (err != MVM_E_SUCCESS)
	{
		Debug::log("Failed to get run function: {}", err);
		return -1;
	}
	// Call the function:
	err = mvm_call(vm.get(), run, nullptr, nullptr, 0);
	// Check the exit status of `run`, report error if not success.
	if (err != MVM_E_SUCCESS)
	{
		Debug::log("Failed to call run function: {}", err);
		return -1;
	}

	return 0;
}