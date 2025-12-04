// Import everything from the environment
import * as legal from "./ffi/legal.js"
import * as exploit from "./ffi/exploit.js"

function run()
{
	legal.login("bob", "bobisasmarterpersonwhousesapassphrase");
	legal.print("Logged in!");
	legal.print("Username: ", legal.get_username());
}


vmExport(1234, run);
