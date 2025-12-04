Capita Selecta Secure Software, CHERI Module: Project Report
============================================================

Write your report in this file.
Reminder: please try to keep the report short and concise. 
We do not want you to spend hours writing it, because that means we will have to spend hours reading the reports.

Task 1: Confidentiality
-----------------------

#### Why does compartmentalization solve the problem?
Answer...

#### Could this problem be solved in a conventional system (without capabilities or compartments)? If yes, briefly outline a possible approach. If no, explain what extra feature CHERIoT has to enable this kind of compartmentalization.
Answer...

Task 2: Fault isolation
-----------------------

#### Why does compartmentalization solve the problem?
Answer...

#### What happens when the JavaScript VM crashes, now that it has been compartmentalized?
Answer...

Task 3: Integrity
-----------------

#### What CHERI(oT) feature(s) did you use to solve this task?
Answer...

#### Could this problem be solved purely by refactoring the code, so the JS FFI never gets a `User *` capability? If yes, which approach do you prefer? Are they redundant, or complementary? If no, briefly describe how an attacker could still break integrity.
Answer...

Task 4: Confidentiality 2
-------------------------

#### What CHERI(oT) feature(s) did you use to solve this task?
Answer...

#### Another (more conventional) approach could be to store the password elsewhere. Would this design be more secure given CHERIoT’s security primitives? What trade-offs would it introduce on an embedded platform, given its constrained resources?
Answer...

Task 5 (Bonus): Unforgeability
------------------------------

#### What CHERI(oT) feature(s) did you use to solve this task?
Answer...

#### Compare this system to one which uses random tokens with sufficient entropy. What are their advantages and drawbacks?
Answer...

Task 6 (Bonus): Resource cleanup
--------------------------------

#### Compare the impact of memory leaks on CHERIoT (with sufficient compartmentalization) to a conventional system. How is CHERIoT an improvement?
Answer...

#### Can you see any temporal safety issues arising from using this "big hammer"? If so, do you have an idea of how to solve them?
Answer...
