#pragma once
#include <stdint.h>
#include <string>

typedef struct User {
	std::string firstname;
	std::string lastname;
	std::string username;
	std::string password;
} User;

typedef int AccessToken;

/**
 * Initialize the user database in memory. Should be called once at the start
 * of the firmware.
 */
void init_users(void);

/**
 * Log in with the given username and password.
 * Returns an AccessToken if login is successful, and -1 otherwise.
 */
AccessToken login(const std::string username, const std::string password);

/**
 * Log out for the given AccessToken, which invalidates said token.
 */
void logout(AccessToken);

/**
 * Get the details of the user associated with the given token.
 * Returns a pointer to a User struct if the token is valid, nullptr otherwise.
 */
User *get_user_details(AccessToken);

/**
 * Returns true if the given username is not yet taken by another user.
 */
bool is_username_available(const std::string username);
