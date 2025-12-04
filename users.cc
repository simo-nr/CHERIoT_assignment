/**
 * Code related to storing a secret value.  The attacker should try to leak
 * the value stored in this file.
 */

#include "users.h"
#include <compartment.h>
#include <debug.hh>
#include <vector>
#include <utility>
#include <algorithm>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "User manager">;

AccessToken allocate_access_token()
{
	static AccessToken last_allocated_token = 0;
	return ++last_allocated_token;
}

static std::vector<User> users;

void init_users(){
	users.push_back({"Alice", "Cipher", "alice", "badpassword"});
	users.push_back({"Bob", "Keyworth", "bob", "bobisasmarterpersonwhousesapassphrase"});
	Debug::log("Users stored at {}", &users);
	Debug::log("Password offset: {}", offsetof(User, password));
}

User *find_user(const std::string username) {
	auto found_user = std::find_if(users.begin(), users.end(),
		[username](const auto& current_user) { return current_user.username == username; }
	);
	if (found_user == users.end()) {
		return nullptr;
	} 
	return &(*found_user);
}

static std::vector<std::pair<AccessToken,User*>> active_tokens;

std::vector<std::pair<AccessToken,User*>>::iterator find_active_token(AccessToken provided_token)
{
	return std::find_if(
		active_tokens.begin(), active_tokens.end(),
		[provided_token](const auto& token_user_pair) { return token_user_pair.first == provided_token; }
	);
}

AccessToken login(const std::string username, const std::string password)
{
	auto *user = find_user(username);
	if (user != nullptr && user->password == password) {
		AccessToken token = allocate_access_token();
		active_tokens.push_back(std::make_pair(token, user));
		Debug::log("User logged in with username {} and gave token {}", user->username, token);
		return token;
	}
	Debug::log("Login failed for {}", user->username);
	return -1;
}

void logout(AccessToken provided_token)
{
	auto token_ptr = find_active_token(provided_token);
	if (token_ptr != active_tokens.end()) active_tokens.erase(token_ptr);
}

User *get_user_details(AccessToken provided_token)
{
	// Search for the token
	auto token_ptr = find_active_token(provided_token);
	// If there is no such active token, the search will yield the end() iterator, return nullptr.
	if (token_ptr == active_tokens.end()) return nullptr;
	// Dereference and deconstruct pair to get the user pointer.
	const auto [token,user] = *token_ptr;
	return user;
}

bool is_username_available(const std::string username) {
	return find_user(username) == nullptr;
}
