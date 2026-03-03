#pragma once
#include <stdint.h>
#include <string>
#include <compartment.h>

typedef struct User {
	std::string firstname;
	std::string lastname;
	std::string username;
	std::string password;
} User;

struct AccessTokenObj
{
	int token;
};

/// The actual access token passed around is an unforgeable sealed capability.
using AccessToken = CHERI_SEALED(AccessTokenObj *);


/**
 * Initialize the user database in memory. Should be called once at the start
 * of the firmware.
 */
void __cheri_compartment("user") init_users(void);

/**
 * Log in with the given username and password.
 * Returns an AccessToken if login is successful, and nullptr otherwise.
 */
AccessToken __cheri_compartment("user") login(const std::string username, const std::string password);

/**
 * Log out for the given AccessToken, which invalidates said token.
 */
void __cheri_compartment("user") logout(AccessToken);

/**
 * Get the details of the user associated with the given token.
 * Returns a pointer to a User struct if the token is valid, nullptr otherwise.
 */
User __cheri_compartment("user") *get_user_details(AccessToken);

/**
 * Task 5 helper: return the (guessable) integer token id for debugging / exploit API.
 * Returns -1 if token is invalid.
 */
int __cheri_compartment("user") token_id(AccessToken);

/**
 * Returns true if the given username is not yet taken by another user.
 */
bool __cheri_compartment("user") is_username_available(const std::string username);

bool __cheri_compartment("user") set_username(User *user, const std::string new_username);

void __cheri_compartment("user") set_fullname(User *user, const std::string firstname, const std::string lastname);

bool __cheri_compartment("user") set_password(User *user, const std::string old_password, const std::string new_password);