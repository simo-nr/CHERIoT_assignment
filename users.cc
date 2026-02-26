/**
 * Code related to storing a secret value.  The attacker should try to leak
 * the value stored in this file.
 */

#include "users.h"
#include <cheri.hh>
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

static User *resolve_user_handle(User *handle)
{
	if (handle == nullptr) {
		return nullptr;
	}

	CHERI::Capability<User> incoming{handle};

	if (incoming.bounds() < static_cast<ptrdiff_t>(sizeof(User))) {
		return nullptr;
	}

	auto basePtr = users.data();
	if (basePtr == nullptr) {
		return nullptr;
	}

	CHERI::Capability<User> base{basePtr};
	ptraddr_t baseAddr = base.address();
	ptraddr_t addr     = incoming.address();

	size_t n = users.size();
	size_t total = n * sizeof(User);

	if (addr < baseAddr || addr >= (baseAddr + total)) {
		return nullptr;
	}

	size_t offset = static_cast<size_t>(addr - baseAddr);
	if ((offset % sizeof(User)) != 0) {
		return nullptr;
	}

	size_t idx = offset / sizeof(User);
	if (idx >= n) {
		return nullptr;
	}

	return &users[idx];
}

void init_users(){
	users.reserve(4);
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
	auto token_ptr = find_active_token(provided_token);
	if (token_ptr == active_tokens.end()) return nullptr;
	const auto [token,user] = *token_ptr;
	// return user;
	CHERI::Capability<User> cap{user};
	cap.bounds() = sizeof(User);
	cap.without_permissions(CHERI::Permission::Store);
	return cap.get();
}

bool is_username_available(const std::string username) {
	return find_user(username) == nullptr;
}

bool set_username(User *user, const std::string new_username)
{
	User *u = resolve_user_handle(user);
	if (u == nullptr) {
		return false;
	}
	if (new_username == u->username) {
		return true;
	}
	if (!is_username_available(new_username)) {
		return false;
	}
	u->username = new_username;
	return true;
}

void set_fullname(User *user, const std::string firstname, const std::string lastname)
{
	User *u = resolve_user_handle(user);
	if (u == nullptr) {
		return;
	}
	u->firstname = firstname;
	u->lastname  = lastname;
}

bool set_password(User *user, const std::string old_password, const std::string new_password)
{
	User *u = resolve_user_handle(user);
	if (u == nullptr) {
		return false;
	}
	// if (u->password != old_password) {
	// 	return false;
	// }
	// u->password = new_password;
	// return true;
	return set_password(user, old_password, new_password);
}