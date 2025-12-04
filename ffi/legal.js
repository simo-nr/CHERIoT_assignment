// FFI Imports
// Each function imported from the host environment needs to be assigned to a
// global like this and identified by a constant that the resolver in the C/C++
// code will understand.
// These constants are defined in the `Exports` enumeration.

/**
 * login(username, password)
 * 
 * Log in with the given username and password.
 */
export const login = vmImport(1);

/**
 * logout()
 * 
 * Log out of the current session (if any).
 */
export const logout = vmImport(2);

/**
 * get_username()
 * 
 * Returns the username of the current user (if any).
 * Returns an empty string if no user is logged in.
 */
export const get_username = vmImport(3);

/**
 * get_fullname()
 * 
 * Returns a pair of the first name and last name of the current user (if any).
 * Returns an empty pair otherwise.
 */
export const get_fullname = vmImport(4);

/**
 * set_username(username)
 * 
 * Sets the username of the current user (if any).
 * Returns true if the operation was successful.
 * Users cannot choose a name that is already taken.
 */
export const set_username = vmImport(5);

/**
 * set_fullname(firstname, lastname)
 * 
 * Sets the first and last name of the current user (if any).
 */
export const set_fullname = vmImport(6);

/**
 * set_password(old_password, new_password)
 * 
 * Sets the password of the current user (if any).
 * User must give their old password for security.
 * Returns true if the operation was successful.
 */
export const set_password = vmImport(7);

/**
 * Log function, writes all arguments to the UART.
 */
export const print = vmImport(8);