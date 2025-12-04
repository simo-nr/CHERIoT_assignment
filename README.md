Capita Selecta Secure Software, CHERI Module: Project
=====================================================

In this project you will be using CHERI(oT) features to defend a small software system, against attackers who try to break into the system using various exploits.
The small software system takes the form of an embedded device running a JavaScript Virtual Machine (VM), where the VM is allowed to interact with its environment in a limited and controlled way.

Practicalities
--------------

Your defense against the attackers has been divided into 4 main tasks and 2 bonus tasks.
Attacks will come in the form of JavaScript exploit files (`exploit_x_abc.js`).
Each task addresses the exploit with the corresponding number.  
Along with patching the exploit, you should write a *small* report explaining what you did and why.
Each task has some guiding questions to indicate what you should write about.
Please try to keep the report short and concise. 
We do not want you to spend hours writing it, because that means we will have to spend hours reading the reports.

If you have any questions regarding the project, you can post them on the [Toledo forum](https://ultra.edu.kuleuven.cloud/ultra/courses/_126764_1/engagement/discussion/_8844878_1?view=discussions&courseId=_126764_1). 
**Please do not post code there.** 
Instead, you should send the code by mail to [Elias Storme](mailto:elias.storme@kuleuven.be) and mention that you have done so in your question on the forum.  

With regards to grading: completing the main tasks correctly and answering questions correctly in your report, without any bonus tasks, will give you a score of at least 16/20.
For full marks you should complete all tasks.
You are allowed to talk to your fellow students about this project, **but you cannot share code** (putting your code in a public repository also counts as sharing code).  
Using ChatGPT or other LLMs is also allowed, but you should adhere to [the university guidelines](https://www.kuleuven.be/english/education/student/educational-tools/guidelines-for-students-on-the-authorised-use-of-genai).
Also keep in mind that they tend to hallucinate on niche topics, and "LLM hallucinated my homework" is not a valid excuse.

The deadline for submitting the project is the 3rd of March 2026, 18:00.  
You can submit your project through the Toledo assignment, as a zip of this folder.
To keep down the size of the zip file, do not include any files listed in the `.gitignore` file (especially not `node_modules`).
The easiest way to do this is to commit all your changes and then execute `git archive -o project.zip HEAD`.  
Note: you do **not** need to give the zip file a special name, or put your project in a named subfolder in the zip.
Toledo will do this automatically, and it is disruptive to the grading workflow if every submission is a little different.

Project codebase
----------------

### General setup

This project starts from a firmware image which implements a JavaScript VM. 
That VM is allowed to interact with the outside environment in a limited way.
It reads JavaScript programs (as compiled bytecode) from the UART and executes them.  
The JavaScript code has access to a set of FFI (Foreign Function Interface) functions, split into two parts.

The first part we call the *legal* API (`ffi/legal.js`), which implements a rudimentary access control system.
From JavaScript, users can start an authenticated session with `login`, and end that session with `logout`.
When authenticated, users can view and modify their own private data.
The legal API represents what is intended to be allowed for the user.

The second part we call the *exploit* API (`ffi/exploit.js`), it contains a set of functions which allow arbitrary capability manipulation, including pointer chasing.
This simulates an attacker building a code-reuse attack, with a lot more power than is normally possible on a CHERI system.
The attacker can:

 - Read the stack capability, global capability, and program counter capability into any of eight (virtual) register slots.
 - Read the permissions, bounds, address, and length of a capability in a register.
 - Read capabilities, integers and strings from memory via any of its eight capabilities registers into a register.
 - Set the address of a capability in any of the registers.
 - Write a capability from a register into memory via a capability in any of the registers.
 - Write an integer or a string into memory via a capability in any of the registers.
 - Run arbitrary JavaScript to perform any of the above actions, intermixed with other computation.

This is the equivalent of one of the most powerful [weird machines](https://en.wikipedia.org/wiki/Weird_machine) that it is possible to create on a CHERI system from code reuse attacks.  
As was mentioned before, the exploit API **simulates** an attacker employing code-reuse attacks to build a weird machine.
We simulate this because building the exploit API from actual code-reuse attacks would be time-consuming and error-prone.

You can find documentation on the full set of the functions exposed through the FFI in their respective JavaScript source files.

The codebase of this project is not an example of stellar software design.
This is for a large part due to the vulnerabilities which were introduced, but that does not necessarily make it unrealistic.
A much larger codebase may suffer from the same vulnerabilities and bad design, but in a way which is less obvious.  
The setup of the project tries to strike a balance between being complex enough to allow some interesting exploits, and simple enough to understand within the timeframe of this project.  
The objective of this project is not to completely redesign the software to eliminate the vulnerabilities.
Rather, you should address the vulnerabilities (preferably with CHERIoT features), while leaving as much code unchanged as possible.)

To make this last point more concrete: **you should never modify the existing JavaScript FFI API!**
That means you should never change the signature of any existing functions in either API.
Changing the implementation of the existing functions or adding new ones is fine, though.

### Code structure

The exploits have been written from the assumption that the attacker has full access to the source code of the system.
Analysis of the code should identify the `users` datastructure as the prime target to try to attack.
This datastructure resides in the `users.cc` file, and it contains a rudimentary user database, consisting of a `vector` of `User` structs (see `users.h` for the definition of `User`).  
`users.h` provides the API for interacting with this database, which should be quite straightforward.
Upon logging in the user receives an `AccessToken`, which they can then use to access their `User` struct or log out.

The thread entrypoint is the `run` function in `main.cc`.
This function will initialize the `users` database through `init_users()`, after which it will continuously execute JavaScript bytecode.
To execute bytecode, it waits until it has read a complete bytecode file (which is enclosed between `{` and `}`), after which it constructs the VM and makes it call the `run()` function in JavaScript.
If at any point during execution there is an exception in the VM, it will exit and return an error status code, which will be reported.

As mentioned in the previous section, JavaScript code has access to C++ functions through an FFI, split into legal and exploit parts.
They are defined in `ffi/legal.js` and `ffi/legal.hh` for the legal API, and `ffi/exploit.js` and `ffi/exploit.hh` for the exploit API, each for JS and C++ respectively.
On the C++ side, functions are exported using an `enum` and some template magic in `ffi/microvium-ffi.hh`.
On the JavaScript side, a function can be imported by defining a constant which is the result of calling `vmImport(x)` where `x` corresponds to the `enum` value in C++.

For example, `register_move` in Javascript corresponds to `Move` in the `Exports` enum.
Further in `exploit.hh`, `export_move` is defined and then exported with a specialization of the `ExportedFn` template for `Move`:
```C++
template<>
constexpr static auto ExportedFn<Move> = export_move;
```

At the top of `ffi/microvium-ffi.hh` you will find one of those `ConditionalDebug` instances with its condition set to false.
This debug instance is there to help you with debugging the FFI, but produces a lot of output!
You may of course enable it whenever you need it.

### CHERIoT Documentation

You may have noticed during the hands-on exercises that it can be tricky to find documentation on CHERIoT (Google asking "did you mean: chariot" certainly does not help).
Here is a list of documentation which we think might be helpful for solving this project:
- [CHERIoT Programmers' Guide](https://cheriot.org/book/index.html)
- [cheriot-rtos docs](https://github.com/CHERIoT-Platform/cheriot-rtos/tree/main/docs)
- The source code of some headers in the CHERIoT SDK: [cheri.hh](https://github.com/CHERIoT-Platform/cheriot-rtos/blob/main/sdk/include/cheri.hh) and [token.h](https://github.com/CHERIoT-Platform/cheriot-rtos/blob/main/sdk/include/token.h)
- [Our own Hands-on Exercises](https://gitlab.kuleuven.be/distrinet/education/capita-selecta-secure-software/cheri/handson-exercises)

Compiling and running the project
---------------------------------

Compiling and running the project is a little bit different than in the exercise session, mainly because we have to feed the simulator bytecode through UART.
There are two shell scripts that help with doing that.
The first, `run_simulator.sh`, will compile and run the firmware in the CHERIoT Ibex simulator, with the UART connected to a named pipe.
This allows the second script, `load_js.sh`, to compile a JavaScript file and provide it directly to the simulator.
It may be useful to run these scripts each in their own terminal window.

To test that everything is working, try the `hello_alice.js` file.  
First, start up the simulator. After compiling the firmware and booting, eventually it should print out something like this:
```
$ ./run_simulator.sh
...
User manager: Users stored at 0x2004dd00 (v:1 0x2004dd00-0x2004dd18 l:0x18 o:0x0 p: G RWcgm- -- ---)
User manager: Password offset: 0x30
```

Then load the JavaScript file into the simulator. The first time this may take a while, because it has to fetch some dependencies through npm.
```
$ ./load_js.sh hello_alice.js 
...
Output generated: /dev/null
816 bytes
```

In your window running `run_simulator.sh` you should then see something like this:
```
Main compartment: Read 0x168 bytes of bytecode
Main compartment: 3384 bytes of heap available
User manager: User logged in with username alice and gave token 1
Alice: Hello world
```

Don't worry if the numbers don't match exactly.

The lines that start with magenta `JavaScript compartment` or `User manager` are debugging lines that are produced by `Debug::log` calls in C++.
Output from JavaScript does not have this prefix.

Task 1: Confidentiality
-----------------------

The first exploit takes advantage of the fact that `users` is a global variable residing in the same compartment as the other code.
It changes Alice's password by reading it from `users` directly, uses it to log in and finally modify the password.
The output of the exploit should look like this:
```
User manager: Users stored at 0x2004dd00 (v:1 0x2004dd00-0x2004dd18 l:0x18 o:0x0 p: G RWcgm- -- ---)
User manager: Password offset: 0x30
Main compartment: Read 0x4a2 bytes of bytecode
Main compartment: 1848 bytes of heap available
User alice had password badpassword
User manager: User logged in with username alice and gave token 1
Alice: Changed the password to worsepassword
```
If it reports `UsersAddress is after the end of CGP` you should modify `UsersAddress` to the value that was logged at the start of execution (in this case `0x2004e778`).

To fix this issue, you should use compartmentalization to make `users` not accessible anymore from the `main` compartment.
Create a `user` compartment, with `users.cc` as its source file.

This first task is meant to be quite straightforward, allowing you to gain confidence in working in the codebase.

#### Answer the following questions in your report:
- Why does compartmentalization solve the problem?
- Could this problem be solved in a conventional system (without capabilities or compartments)?
If yes, briefly outline a possible approach.
If no, explain what extra feature CHERIoT has to enable this kind of compartmentalization.

Task 2: Fault isolation
-----------------------

Exploit 2 takes advantage of the fact that the JavaScript VM runs in the main compartment.
This means that if the attacker can cause some kind of hardware exception, they can crash the whole loop and deny service to other users.

To address this, we want to move the JavaScript execution into a compartment so that, if it crashes, it doesn't take out the main run loop.
This will require slightly larger changes than the first task.
In the first exercise, our compartment boundary aligned with an existing software-engineering boundary.
The code handling the users was already a conceptually separate component, we just made it a security boundary.

You will probably find it easier to do this task as four steps:

 1. Factor the code that handles the JavaScript VM (from the large block comment to the end of the loop iteration) into a separate function that just takes the bytecode buffer as an argument.
 2. Move that function into a separate file.
 3. Move that file into a separate compartment.
 4. Add an exception handler to the new compartment, which reports the exception, but does not shut down the simulator.

At the end of this refactoring, you should be able to run exploit 2 multiple times, without it actually crashing the main compartment.

#### Answer the following questions in your report:
- Why does compartmentalization solve the problem?
- What happens when the JavaScript VM crashes, now that it has been compartmentalized?

Task 3: Integrity
-----------------

This task will consist of protecting the integrity of the `users` datastructure.
Exploit 3 demonstrates how its integrity can be broken, by changing the username of a user to one that is already taken.
If Bob tries to log in after this exploit has happened, they will be rejected because of an incorrect password.

The objective of this task is to protect the `users` datastructure's integrity, **while still allowing the Javascript VM to hold references to `users`.**
Concretely, `get_user_details` should still return `User *` , but only the `user` compartment (which was set up in task 1) should be able to modify `users`.
This means you can change the setters for `User` fields, but you should not have to modify getters.

#### Answer these questions in your report:
- What CHERI(oT) feature(s) did you use to solve this task?
- Could this problem be solved purely by refactoring the code, so the JS FFI never gets a `User *` capability? 
If yes, which approach do you prefer? Are they redundant, or complementary?
If no, briefly describe how an attacker could still break integrity.

Task 4: Confidentiality 2
-------------------------

In task 1 we prevented that a user's password is directly changed due to being in the same compartment, which could be done without logging in.
What if a user was a bit careless, and left without logging out?
Task 3 should have as a side-effect that we cannot directly overwrite the password in memory, but that does little to prevent changing the password, as demonstrated in exploit 4.

Task 4 consists of keeping the password confidential, even if a user session was accidentally kept open.
Similar to task 3, we require that you can still pass `User *` to the VM, it just should not be able to use it to read the password.

#### Answer these questions in your report:
- What CHERI(oT) feature(s) did you use to solve this task?
- Another (more conventional) approach could be to store the password elsewhere.
Would this design be more secure given CHERIoT’s security primitives?
What trade-offs would it introduce on an embedded platform, given its constrained resources?


Task 5 (Bonus): Unforgeability
------------------------------

You may have noticed that the algorithm which is used to generate `AccessToken`s is, to put it mildly, not that secure.
Of course this way of generating tokens would never be used in practice, but it is a good simplification of the situation where the tokens have insufficient entropy (are in some way guessable).

Exploit 5 guesses the token to hijack another user's session.
To execute the the exploit correctly you should first run `bob_forgets_to_logout.js`, and then `exploit_5_unforgeability.js`.

For this task, you have to make the tokens unforgeable **without relying on randomness**.
Concretely, you can change tokens' type and how they are allocated, but do not make them less guessable.

#### Answer these questions in your report:
- What CHERI(oT) feature(s) did you use to solve this task?
- Compare this system to one which uses random tokens with sufficient entropy.  
What are their advantages and drawbacks?

Task 6 (Bonus): Resource cleanup
--------------------------------

Just before the JavaScript VM starts, the simulator will report a line like this:

```
JavaScript compartment: 0xbf8 bytes of heap available
```

**If the Main compartment prints this instead of the JavaScript compartment, move those debug lines to `js.cc`, otherwise the exploit will not make sense.**
This number is not the total amount of available heap memory, it is the amount that the compartment that logs the message is authorised to allocate.
If this is exhausted, other compartments may still allocate memory from their quotas, but the JavaScript compartment may crash if it tries to allocate memory.
Thanks to task 2, our leak has a constrained blast radius, but it's still an availability problem for the JS VM.

If you have correctly moved the JavaScript code to a new compartment, then that compartment will leak some memory every time you load the `exploit_2_availability.js` script.
If you have moved (or copied) this line into the compartment that runs the JavaScript interpreter then you will see the amount of memory available for that compartment go down each time that `exploit_2_availability.js` runs.

Exploit 6 takes advantage of this fact to try to deny service to other users (notice it is a shell script instead of JavaScript).
It runs `exploit_2_availability.js` over and over until the JS compartment runs out of memory.
Running any other JavaScript code after this exploit will cause the VM to not be able to start, for example `hello_alice.js`:
```sh
JavaScript compartment: Read 0x330 bytes of bytecode
JavaScript compartment: 144 bytes of heap available
js.cc:36 Assertion failure in run_javascript
Failed to parse bytecode: MVM_E_MALLOC_FAIL(0x2)
                   0: Illegal instruction (hart 0) at PC 0x2004b616: 0x00000000

JavaScript compartment: None(0x0) caused by JavaScript VM.
Register CZR(0x0) contained invalid value: 0x0 (v:0 0x0-0x0 l:0x0 o:0x0 p: - ------ -- ---)
```

Memory quotas are implemented via a capability model.
Each compartment may hold zero or more capabilities that authorise allocating memory, with different quotas.
By default, each one holds a capability accessed via the `MALLOC_CAPABILITY` macro that authorises it to allocate up to 4096 bytes.
This is configurable, see `/cheriot-rtos/sdk/include/stdlib.h` for more information.

In `stdlib.h`, you may notice a function called `heap_free_all`.
This is a big hammer for resource cleanup: it frees *all* memory that was allocated with a specific capability.
You can use this to avoid memory leaks.
Most commonly, you will use this in concert with an *error handler*.

Modify your error handler from task 2 to ensure no memory can leak if the JavaScript compartment crashes.

#### Answer the following questions in your report:
- Compare the impact of memory leaks on CHERIoT (with sufficient compartmentalization) to a conventional system.  
How is CHERIoT an improvement?
- Can you see any temporal safety issues arising from using this "big hammer"?  
If so, do you have an idea of how to solve them?
