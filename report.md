Capita Selecta Secure Software, CHERI Module: Project Report
============================================================

Write your report in this file.
Reminder: please try to keep the report short and concise. 
We do not want you to spend hours writing it, because that means we will have to spend hours reading the reports.

Task 1: Confidentiality
-----------------------

#### Why does compartmentalization solve the problem?
Moving users into a separate user compartment removes it from the main compartment’s global capability (CGP). Code in main (and therefore the attacker-controlled JS VM running there) can no longer derive a capability that can read or write the users global directly. The only way to interact with user data is via the explicit cross-compartment API, which enforces checks (e.g., login, password checks).

#### Could this problem be solved in a conventional system (without capabilities or compartments)? If yes, briefly outline a possible approach. If no, explain what extra feature CHERIoT has to enable this kind of compartmentalization.
User management would have to be moved in a separate domain, for example a seperate process or service, or use an MPU to isolate memory regions and only expose an interface. Only refactoring in OOP is not secure since any code in the same address space can still read/write to the same memory. CHERIoT enables isolation in the same address space with capabilities and compartments, these can prevent the main compartment from holding a capability to the user database memory. 

Task 2: Fault isolation
-----------------------

#### Why does compartmentalization solve the problem?
By running the JavaScript VM in its own JSVM compartment, faults caused by attacker-controlled bytecode are contained. The main run loop lives in another compartment, so the VM cannot directly crash it. A compartment-specific error handler can intercept exceptions from inside the VM compartment and unwind back to the caller.

#### What happens when the JavaScript VM crashes, now that it has been compartmentalized?
When the JSVM crashes, an error handler is called which will print the error message with which the VM crashed and returns control to the caller. This means that an exception in the compartment will not stop execution, since control is returned and the main loop runs again. A crash of the VM does not affect other compartments. 

Task 3: Integrity
-----------------

#### What CHERI(oT) feature(s) did you use to solve this task?
Capabilities are used. Specifically, the system is updated so that the attacker can not write to the user struct because they only receive read permissions. And also compartmentalization since all mutations are performed by functions implemented inthe user compartment. The VM can hold references to the user struct but cannot use them to mutate the state directly. 

#### Could this problem be solved purely by refactoring the code, so the JS FFI never gets a `User *` capability? If yes, which approach do you prefer? Are they redundant, or complementary? If no, briefly describe how an attacker could still break integrity.
Yes, it could be solved by design by never giving JS direct pointers and instead exposing only an ID plus getter/setter APIs. But here the idea was that the VM was still allowed to hold references. With this constraint, capability based enformcement is the right method. In general, these methods are complementary. Refactoring can reduce the risk by reducinc the attack surface, while capability enforcement provides a safety net if some type of reference must be passed. 

Task 4: Confidentiality 2
-------------------------

#### What CHERI(oT) feature(s) did you use to solve this task?
Once again, capabilities are used. This time the bounds of the capability are changed to not include the address where the password is stored. 

#### Another (more conventional) approach could be to store the password elsewhere. Would this design be more secure given CHERIoT’s security primitives? What trade-offs would it introduce on an embedded platform, given its constrained resources?
Storing passwords separately can also be secure and conceptually simpler, since no secret fields are ever exposed. However, with CHERIoT the bounds-based approach already enforces confidentiality directly via capabilities. Separating storage adds extra code, cross-compartment calls, memory overhead, and complexity. On embedded systems, the bounds-based solution is typically simpler and more efficient.

Task 5 (Bonus): Unforgeability
------------------------------

#### What CHERI(oT) feature(s) did you use to solve this task?
Session tokens are represented as sealed capability handles. Attackers may guess the integer token id, but they cannot fabricate a valid sealed capability, so they cannot hijack another user’s session without access to the actual capability.

#### Compare this system to one which uses random tokens with sufficient entropy. What are their advantages and drawbacks?
Sealed capability tokens do not require randomness and are unforgeable. However they only work on CHERI capability hardware and are not directly usable as tokens outside the system, for example over a network, without any additional overhead. 

Random tokens, on the other hand, are portable to conventional systems and work accross networks boundaries. They require careful handling to avoid leaks and still depend on secrecy of the token value, they also require a secure RNG. 

Task 6 (Bonus): Resource cleanup
--------------------------------

### NOT IMPLEMENTED (but answered the questions)

#### Compare the impact of memory leaks on CHERIoT (with sufficient compartmentalization) to a conventional system. How is CHERIoT an improvement?
With compartmentalization, leaks are primarily contained to the leaking compartment’s resources, reducing the damage impact (similar to fault isolation and DoS containment). In a conventional monolithic address space, leaks and corruption often affect the entire process / system. CHERIoT can also support tighter control via per-compartment allocation patterns and reduced authority (in form of capabilities), making it easier to localize and reason about damage.

#### Can you see any temporal safety issues arising from using this "big hammer"? If so, do you have an idea of how to solve them?
Yes, cleaning up a compartment this way can skip regular clean up methods. This can leave the system in an inconsistent state. Possible solutions for this would be to design APIs to be idempotent, keeping the compartment state minimal and able to restart, performing explicit cleanup in error handlers, avoid sharing mutable states across compartments and using disciplined ownership patterns so dangling references are harder to create. 
