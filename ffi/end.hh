#pragma once
#include "microvium-ffi.hh"

namespace {
    /**
	 * Callback from microvium that resolves imports.
	 *
	 * This resolves each function to the template instantiation of
	 * `exported_function` with `funcID` as the template parameter.
	 */
	mvm_TeError
	resolve_import(mvm_HostFunctionID funcID, void *, mvm_TfHostFunction *out)
	{
		return magic_enum::enum_switch(
		  [&](auto val) {
			  constexpr Exports Export = val;
			  *out                     = exported_function<Export>;
			  return MVM_E_SUCCESS;
		  },
		  Exports(funcID),
		  MVM_E_UNRESOLVED_IMPORT);
	}

	/**
	 * Helper that deletes a Microvium VM when used with a C++ unique pointer.
	 */
	struct MVMDeleter
	{
		void operator()(mvm_VM *mvm) const
		{
			mvm_free(mvm);
		}
	};
}