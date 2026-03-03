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
	VMState state;
	state.current_token = nullptr;
	vmStateAddress = Capability{&state}.address();

	mvm_TeError                         err;
	std::unique_ptr<mvm_VM, MVMDeleter> vm;
	{
		mvm_VM *rawVm;
		err = mvm_restore(
			&rawVm,
			(void *)bytecode,
			bytecode_len,
			MALLOC_CAPABILITY,
			::resolve_import);
		if (err != MVM_E_SUCCESS)
		{
			Debug::log("Failed to parse bytecode: {}", err);
			return -1;
		}
		vm.reset(rawVm);
	}

	mvm_Value run;
	err = mvm_resolveExports(vm.get(), &ExportRun, &run, 1);
	if (err != MVM_E_SUCCESS)
	{
		Debug::log("Failed to get run function: {}", err);
		return -1;
	}
	err = mvm_call(vm.get(), run, nullptr, nullptr, 0);
	if (err != MVM_E_SUCCESS)
	{
		Debug::log("Failed to call run function: {}", err);
		return -1;
	}

	return 0;
}

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	(void)frame;

	Debug::log("JS VM fault: mcause=0x{:x}, mtval=0x{:x}", mcause, mtval);

	if (mcause == 0x1c)
	{
		auto [cause, reg] = CHERI::extract_cheri_mtval(static_cast<uint32_t>(mtval));
		Debug::log("CHERI fault details: cause={}, reg={}", cause, reg);
	}
	return ErrorRecoveryBehaviour::ForceUnwind;
}