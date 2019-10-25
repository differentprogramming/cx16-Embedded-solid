# cx16-Embedded-solid
Embedded Solid will be dev env/compiler hosted on a CommanderX16.  This starts with PC implementation before self-hosting.

I've been interested in language implementation for a very long time.  And since I started out programming on 8 bit computers it was kind of eye opening to be looking at the 6502 on the CommanderX16 again.  I'd forgotten how simple processors were back then.  

For a while, I've been wanting to come up with a new language that people could use like the ubiquitous scripting languages, Python, Ruby and Lua but with somewhat more practical design decisions than these popular ones.  But a COMPILER not an interpreter.  And I want it to be relatively small and have a tiny but nice environment for debugging.

Seeing this tiny machine makes the challenge that much greater.  Can I implement something close to what I want on a 65c02 based machine?  If I can, it will be a hack to be proud of. 

The language is called Solid which despite being somewhat object oriented is not based on the SOLID acronym of OO design principles.  I just picked that name because the language is has an emphasis on the concrete implementation of software.  There are no abstractions that aren't worth the expense.  No "everything is an object" like Smalltalk.  No unlimited precision numbers that no one will ever need like Lisps. No "all collection types have been merged" like Lua.  

Note: this document uses a bunch of programming terms that I'm not defining here.  If you're new to programming then you probably won't understand it. 

For people who don't know what it is, the CommanderX16 a retro computer that's being worked on. It will have a 8 mhz 65c02 or maybe they'll change it to a 65816, 512 to 2 megabytes of ram and graphics and sound processing that's about on par with a super nintendo or Sega Genesis/Megadrive.

Currently the language I've designed for it is pretty plain vanilla, with the following perperties:
1) Most importantly, the ability to place data and code in the bank switched memory, even though using that will slow execution down a lot.
2) A vm that makes code very compact.
3) Support for fast code that fits in the main memory and addresses data in main memory as well as slower VM based code that sits out on the banks. Note, that part of the compiler will be written in the the language minus that stuff once it works. 
4) Support for things a 6502 needs like 8 bit arithmetic. 
5) Eventually a built in assembler.
6) The language itself:

  - Looks like a cross between Smalltalk and lua with static typed variables for primitive types and a wider choice of collection types than lua.
  
  - Object oriented with duck typing, but primitive types are kept in statically typed variables.  
  
    * Messages are typed.  You may not know what kind of object you are sending a message to, but you DO know the proper static type signature of a message with the appropriate name. 
    
    * There is no overloading on type for messages or functions. If you need to send different types you can make the parameter boxed.  Boxed variables can take any type.
    
    * Has a LUA like calling convention so that functions can both accept a variable number of parameters and return a variable number of values.
  
    * There will be a choice of object lifetime models from full mark and sweep garbage collection to reference counted garbage collection to phase based lifetimes (ie useful for games, objects last until the scene or subscene changes). 
  
  - Built in collection and structre types include:
  
    * Hash tables, typed or with boxed elements.
    
    * Double linked lists, typed or with boxed elements.
    
    * counted balanced trees (ie a collection that can be accessed by index in log time, insert and delete take log time as do adding or removing from the ends.)  Basically a perfect type to implement a word processor with.
    
    * Arrays whose size is immutable. Basically a type useful for interacting with assembly code.
    
    * Classes.  Note that access to variables and methods will be through a table of selectors X classes, so that access is a constant time function without the overhead of hashing.
    
    * Objects.  
    
    * Closures.
    
    * Strings.
    
    * Boxed values. (Ie. can hold any type).
    
    * C style records, possibly in main memory.  Another type meant for communicating with assembly language.
    
    * Messages. Messages can be stored. Note if a message goes to doesNotUnderstand: then the values are boxed first.
    
   - Scalar types will be: unsigned bytes, signed 2 byte numbers, signed 3 byte numbers, real numbers, booleans, atoms, selectors, pointers to data on banks, pointers to data in main memory (note, pointers can be to functions as well). Note that explicit pointers won't be followed by any garbage collector.  They're meant for communicating with assembly language. 
   
   - Things that aren't in the language because it's already a bit too fancy for a self hosted embedded language, no exceptions, no co-routines, no advanced flow of control like backtracking or continuations, no module system.
   
   - Things that aren't in the language because it doesn't seem worth the effort: goto.
   
7) Flow of control: Functions, if then elseif else end, while, do until, for, break/continue for loops, switch case.
8) Things that are my indulgence.  I call the boolean type "whether" and instead of "true" and "false" I have "yes" and "no".  You're asked to use a comment so that whenever something is typed "whether" you state what the question being answered is. 
9) As I said when I talked about why I named the language "Solid", this language is designed for practial development not conceptual purity. Classes are not objects. Numbers are not objects.  Whether other types are objects depends on whether that simplifies the compiler, but you won't be given the option of adding methods to built in types.  
10) Once the language is done then I (we?  Any help?) can work on a developement environment.  At least an editor.  A debugger would be nice.  And I'm curious if I can make a nice environment from it all.  Maybe a text mode windowing system >.>  After all, I want the same for a PC version.

What makes me think that a language this complex can be hosted on a 6502 based machine?  Part of it is that the bytecodes the VM runs will be much much more compact than 6502 code and the vm will host the compiler.  Part of it is that these machines will have up to 2 megabytes of ram.  

One reason I'm doing this is that I also want to make a PC version, but I expect that will be a more complex language and of course won't have CommanderX16 specific features.

Also the reason I wrote my own emulator is that I just want an emulator I can test the compiler on which will show me that code generation works and how fast the code runs.  Hacking into the existing CommanderX16 emulator for that sounded like more work than writing my own.  

The grammar is written in Antlr, but I'm just using Antlrworks to verify that the grammar makes sense, I'm not actually implementing it in Antler.  It will be a hand-coded top-down parser, written in the target vm. 

The License is GNU version 3

If anyone wants to work on the project too, let me know. It's probably in a bit too early a stage for that yet.  I have to get the vm working and write a preliminary compiler first. 

The repository isn't starting out open to contibutions by the public but I could change that.
