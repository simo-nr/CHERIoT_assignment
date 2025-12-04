// Import everything from the environment
import * as legal from "./ffi/legal.js"
import * as exploit from "./ffi/exploit.js"

function run()
{
	legal.login("alice", "badpassword");
	legal.print('Hello world');
	legal.logout();
}


vmExport(1234, run);
