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

#include <timeout.hh>
#include <token.h>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "User manager">;

static TokenKey g_token_key = nullptr;
static int      g_last_allocated_token = 50;


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

	if (g_token_key == nullptr)
	{
		g_token_key = token_key_new();
	}

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

static std::vector<std::pair<int,User*>> active_tokens;

static std::vector<std::pair<int,User*>>::iterator find_active_token(int provided_token)
{
	return std::find_if(
		active_tokens.begin(), active_tokens.end(),
		[provided_token](const auto& token_user_pair) { return token_user_pair.first == provided_token; }
	);
}

static int token_id_internal(AccessToken tok)
{
	if (tok == nullptr || g_token_key == nullptr)
	{
		return -1;
	}
	auto *unsealed = static_cast<AccessTokenObj *>(
	  token_obj_unseal(g_token_key, static_cast<CHERI_SEALED(void *)>(tok)));
	if (unsealed == nullptr)
	{
		return -1;
	}
	return unsealed->token;
}

AccessToken login(const std::string username, const std::string password)
{
	auto *user = find_user(username);
	if (user != nullptr && user->password == password) {
		void *unsealed_raw = nullptr;
		auto sealed_void = token_sealed_unsealed_alloc(
		  nullptr,
		  MALLOC_CAPABILITY,
		  g_token_key,
		  sizeof(AccessTokenObj),
		  &unsealed_raw);

		if (sealed_void == nullptr || unsealed_raw == nullptr)
		{
			return nullptr;
		}

		auto *unsealed = static_cast<AccessTokenObj *>(unsealed_raw);
		unsealed->token = ++g_last_allocated_token;

		AccessToken tok = static_cast<CHERI_SEALED(AccessTokenObj *)>(sealed_void);
		active_tokens.push_back(std::make_pair(unsealed->token, user));

		Debug::log("User logged in with username {} and gave token {}",
		           user->username,
		           unsealed->token);
		return tok;
	}
	if (user != nullptr)
	{
		Debug::log("Login failed for {}", user->username);
	}
	else
	{
		Debug::log("Login failed for unknown user {}", username);
	}
	return nullptr;
}

void logout(AccessToken provided_token)
{
	if (provided_token == nullptr)
	{
		return;
	}
	int id = token_id_internal(provided_token);
	if (id >= 0)
	{
		auto it = find_active_token(id);
		if (it != active_tokens.end())
		{
			active_tokens.erase(it);
		}
	}

	token_obj_destroy(MALLOC_CAPABILITY, g_token_key, static_cast<CHERI_SEALED(void *)>(provided_token));
}

User *get_user_details(AccessToken provided_token)
{
	int id = token_id_internal(provided_token);
	if (id < 0) return nullptr;
	auto it = find_active_token(id);
	if (it == active_tokens.end()) return nullptr;
	User *user = it->second;
	CHERI::Capability<User> cap{user};

	static_assert(offsetof(User, password) > 0);
    cap.bounds() = offsetof(User, password);

	cap.without_permissions(CHERI::Permission::Store);
	return cap.get();
}

int token_id(AccessToken tok)
{
	return token_id_internal(tok);
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
	if (u->password != old_password) {
		return false;
	}
	u->password = new_password;
	return true;
}