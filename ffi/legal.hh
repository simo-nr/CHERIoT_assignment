#pragma once
#include "microvium-ffi.hh"

namespace {

	/**
	 * Log in with the given username and password.
	 * `state` is updated with obtained token.
	 */
	bool export_login(const std::string username, const std::string password)
	{
		auto token = login(username, password);
		if (token < 0) return false;
		state().current_token = token;
		return true;
	}
	
	/** 
	 * Log out of the current session (if any).
	 * Reset the token to being invalid.
	 */
	void export_logout()
	{ 
		if (state().current_token < 0) return;
		logout(state().current_token);
		state().current_token = -1;
	};

	/**
	 * Return the username of the current user (if any).
	 */
    std::string export_get_username()
	{
        auto *user = get_current_user_details();
		if (user == nullptr) return "";
        return user->username;
    }

	/**
	 * Return the first and last name of the user who is currently logged in.
	 */
    std::pair<std::string, std::string> export_get_fullname()
	{
        auto *user = get_current_user_details();
		if (user == nullptr) return std::make_pair("","");
        auto fullname = std::make_pair(user->firstname, user->lastname);
        return fullname;
    }

	/**
	 * Set the username of the user who is currently logged in.
	 */
    bool export_set_username(std::string new_username)
	{
        auto *user = get_current_user_details();
        
		if (user == nullptr) return false;
        // if (new_username == user->username) return true;
        // if (!is_username_available(new_username)) return false;

        // user->username = new_username;
        // return true;
		return set_username(user, new_username);
    }

	/**
	 * Set the first and last name of the user who is currently logged in.
	 */
	void export_set_fullname(std::string firstname, std::string lastname)
	{
		auto *user = get_current_user_details();
		if (user == nullptr) return;
		// user->firstname = firstname;
		// user->lastname = lastname;
		set_fullname(user, firstname, lastname);
	}

	/**
	 * Set the password of the user who is currently logged in.
	 * User must give their old password for security.
	 */
	bool export_set_password(std::string old_password, std::string new_password)
	{
		auto *user = get_current_user_details();

		if (user == nullptr){
			FFIDebug::log("Password change failed because user was not found");
			return false;
		}
		if (old_password != user->password){
			FFIDebug::log(
				"Password change failed for user {} because old password was incorrect!",
				user->username);
			return false;
		}
		
		user->password = new_password;
		return true;
	}

	template<>
	constexpr auto ExportedFn<Login> = export_login;

	template<>
	constexpr auto ExportedFn<Logout> = export_logout;

	template<>
	constexpr auto ExportedFn<GetUsername> = export_get_username;

	template<>
	constexpr auto ExportedFn<GetFullname> = export_get_fullname;

	template<>
	constexpr auto ExportedFn<SetUsername> = export_set_username;

	template<>
	constexpr auto ExportedFn<SetFullname> = export_set_fullname;

	template<>
	constexpr auto ExportedFn<SetPassword> = export_set_password;

	/**
	 * Print a string passed from JavaScript.
	 */
	template<>
	mvm_TeError exported_function<Print>(mvm_VM            *vm,
	                                     mvm_HostFunctionID funcID,
	                                     mvm_Value         *result,
	                                     mvm_Value         *args,
	                                     uint8_t            argCount)
	{
		auto *user = get_user_details(state().current_token);
		if (user != nullptr) {
			std::string prefix;
			prefix += user->firstname;
			prefix += ": ";
			uart_write(prefix.data());
		}
		// Iterate over the arguments.
		for (unsigned i = 0; i < argCount; i++)
		{
			// Coerce the argument to a string and get it as a C string
			const char *str = mvm_toStringUtf8(vm, args[i], nullptr);
			// Write each character to the UART
			uart_write(str);
		}
		// Write a trailing newline
		uart_write("\n");
		// Unconditionally return success
		return MVM_E_SUCCESS;
	}
}