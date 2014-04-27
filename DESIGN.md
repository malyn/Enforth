# ARF

Arduino-based Forth: embraces the Arduino ecosystem and libraries, while allow Forth-style interactive programming.

# Overview

Provided as an Arduino Library with a small handful of entry points.  Two key entry points are KEY and EMIT.  Using these, Technomancy could have KEY receive input from the keyboard matrix and EMIT send characters out to the USB host.  This way you could program the device by firing up Notepad and then typing into your keyboard!  No need for a serial console or anything like that.

# Operation

* Flash contains your Arduino sketch, which includes the ARF "kernel" (just another Arduino Library), any linked-in libraries, and all of your predefined words.  The flash dictionary is stored here.
* RAM contains your interactive words and a runtime dictionary.
  * Obviously the RAM used by your libraries and stuff is here as well.
  * The size of the ARF RAM dictionary space is controlled by an ARF #define and can vary by platform.
* EEPROM contains your load-at-boot words and a checksum to verify that we should actually load the EEPROM dictionary into RAM.
* At any time you can copy an existing definition from RAM to EEPROM.  A `GO` definition in RAM after boot (copied from EEPROM or already in the flash) will cause that word to be executed.
  * You can provide a boot-time hook for bypassing the automatic `GO` word, probably by detecting a low/high signal on an input pin, but it's up to you.

# Implementation

* 16-bit Forth
* Token-threaded Forth with 8-bit tokens.
  * 32 words (5 bits) may be defined in RAM and require a 128-byte RAM lookup table.
  * Remaining words (224) are in flash.
  * Ideally we would only use 128/160/192 flash words for the kernel and then make 96/64/32 available for FFI primitives.
  * 128 looks possible, which then gives us 96 for FFI (which should be fine for a huge number of Arduino Libraries and stuff) and 32 for RAM.

# Primitives vs. Definitions

**Primitives** are part of the ARF kernel and are Forth words written in C.  They can block if necessary (for I/O, although that would not be ideal if we ever implement multitasking), but they cannot reenter the ARF kernel.  That is because the kernel needs a thread of words to follow and that thread effectively stops at the point where a C word has been invoked.

**Definitions** are Forth words that are located in RAM and participate in the threading process.  This allows those words to reenter the interpreter, change between interpretation and compilation states, etc.

This distinction between the two types of words works fine in almost all cases except for `EVALUATE` and `QUIT`, which loop and/or reenter the address interpreter.  Those two words need to be present in RAM so that they can participate in the threading flow.

In order to do that, the initialization phase for a blank dictionary will compile those two words into the dictionary so that they are available in RAM.  This does take up dictionary space, but is unavoidable if we want to offer an interactive Forth environment.  Note that we could choose to optionally exclude those words from the dictionary, but then the Forth kernel would only be capable of executing turnkey applications that do not require the text interpreter.

Another option would be to use the C stack for these words by invoking a C function and then looping in there, calling back into the inner interpreter as necessary.  That is what Ficl does, but then it needs setjmp/longjmp.  Without those we would potentially blow up the stack.

The ARF approach sacrifices a very small amount of RAM for performance (no need to determine if the IP is pointing at RAM or flash) and the conservation of the C stack (no recursion in the interpreter).

*I wonder if we'll eventually need to support Flash-based definitions though?  The RAM-based approach puts a permanent limit on the size of a Forth application that you can create since all of your definitions have to fit in RAM...  Maybe people would rather trade speed (IP lookup in RAM vs. Flash) for the convenience of getting to stay in Forth (as opposed to having to constantly rewrite Forth in C as your program gets larger).  Having the IP be smart would let us do the whole DUMP-WORD thing to produce #defines or the equivalent PROGMEM string.*

This is going to be hard to do though... Both Flash and RAM use the same numerical address space because they are accessed with different words.  That means that IP has to go into "Flash mode" and stay there until we return from the word that put us into Flash mode.  We should leave this until a later release.

# Execution Tokens

An Execution Tokens (XT) in ARF can refer either to an ARF primitive or user definition.  We need to be able to tell these two things apart in a couple of different scenarios:

1. Inner Interpreter
   The inner interpreter will pull instructions out of the current word's thread and needs to know if a value represents a one-byte primitive or a two-byte address reference.
2. `EXECUTE`, `COMPILE,`, etc.
   These words expect an XT on the stack and will then do something with that XT.  Note that in at least one case (`COMPILE,`) this stack-based XT will be later used by the inner interpreter.

We have already decided that the Inner Interpreter will identify primitives by a clear high bit -- values 0-127 -- and use a set high bit to identify a relative (negative) offset to the user-defined word in the dictionary that is being referenced.

This same thing will not work for stack-based XTs though, because the stack pointer of course has no relation to the IP.  This means that XTs on the stack need to be different from compiled XTs.  I recommend that we continue to use the high bit to decide between primitives and user definitions, but that the actual XT is a positive offset from the start of the dictionary and we just clear the high bit before trying to use the offset.

`COMPILE,` then calculates the offset from `HERE` to that offset (after including the dictionary start), `EXECUTE` just sets IP to the dictionary start plus the offset, etc.

# Relocation

ARF programs are created interactively.  The program manifests itself as a dictionary image.  The dictionary image can be copied verbatim into EEPROM for turnkey applications and is then copied back into RAM to execute the turnkey application.

This works for the following reasons:

* Branch addresses are 8-bit relative offsets and so relocate with the word, even if the word was to change its starting location (which doesn't happen).
* Indirect threaded references are 16-bit relative offsets into prior parts of the dictionary and so the entire dictionary is relocatable.
* The only absolute addresses are CFA addresses, which point at jump locations in program memory for the four `DO*` words.  EEPROM dictionaries are only valid for a specific build of ARF, so this is fine as well.

In theory we could rewrite CFA addresses after copying the dictionary from RAM, although to do that we would need to add a flag to each word's header that specified the type of CFA.

The other option would be to go to a tokenized CFA and offset through a `DO`-specific jump table, but that would slow down all word invocations.  Rewriting-during-load is much more efficient and only costs us two bits in the flag field (since there are four `DO` words).

# Optimizations

RAM is the most precious resource in the system and so a handful of optimizations/decisions have been made in order to conserve that resource:

* We do not implement `WORD` or `FIND` since those require a separate word buffer.  Instead, we use MFORTH's `PARSE-WORD` and `FIND-WORD` functions, which expect `c-addr u` and thus can point directly at the terminal input buffer.
* The hidden definitions (`EVALUATE`, `INTERPRET`, and `QUIT`) are stored in flash and a dedicated CFA is used to switch the interpreter into PROGMEM threading mode on the AVR.  This slows down the inner interpreter since it has to know which mode it is in, but the benefit is that we do not need so spend 100 or so bytes on these hidden definitions.

# Initialization

ARF is initialized with an array.  ARF takes over that array and uses it for the dictionary and stacks (the latter which are configurable in the constructor).  The stacks start at the end of the array and the dictionary at the beginning.  At the very beginning of the array are the RAM definitions for `EVALUATE` and `QUIT` per the discussion earlier.

`go()` takes the name of the word to find and execute and, by default, that word is `COLD`.  `COLD` compiles `EVALUATE` and `QUIT` into the dictionary and then calls `ABORT` (which calls `QUIT`).  The caller could also specify `WARM-EEPROM` which copies the dictionary from EEPROM to RAM and then calls `ABORT` (since the EEPROM would already have to contain `EVALUATE` and `QUIT`).

Later, we will allow people to specify `WARM-EEPROM` which loads and verifies the EEPROM and then calls `ABORT`.  Finally, people will eventually be able to specify their own turnkey word (which of course also requires the EEPROM to be loaded).

## Kernel

* Rules:
  * No alignment words; you know your platform.
  * No pictured numeric output.
  * Immediate kernel words don't need an opcode.
* Possible Rules:
  * Get rid of string input and dictionary searches?  ACCEPT, FIND, WORD, &gt;NUMBER
    * This really focuses ARF on embedded systems (no text input)...
  * Eliminate unlikely helpers: SPACE, SPACES
  * Drop HEX and DECIMAL and put BASE back in there
  * Consider eliminating advanced concepts -- immediate, &gt;BODY, etc.
* Note:
  * Things like BEGIN, AGAIN, REPEAT, etc. are actually immediate words and don't require opcodes.
  * Same thing with HERE (no opcode).

CORE (implemented)

    !                   '                   *

    */                  */MOD               +                   +!
    +LOOP               ,                   -                   .
    ."                  /                   /MOD                0<

    0=                  1+                  1-                  2!
    2*                  2/                  2@                  2DROP
    2DUP                2OVER               2SWAP               :
    ;                   <                   =                   >

    >BODY               >NUMBER             >R                  ?DUP
    @                   ABORT               ABORT"              ABS
    ACCEPT              ALLOT               AND                 BEGIN
    BL                  C!                  C,                  C@

    CHAR                CONSTANT            COUNT               CR
    CREATE              DECIMAL             DO                  DOES>
    DROP                DUP                 ELSE                EMIT
    EXECUTE             EXIT                FILL                FIND

    HERE                I                   IF                  IMMEDIATE
    INVERT              J                   KEY                 LEAVE
    LITERAL             LOOP                LSHIFT              M*
    MAX                 MIN                 MOD                 MOVE

    NEGATE              OR                  OVER                QUIT
    R>                  R@                  RECURSE             REPEAT
    ROT                 RSHIFT              S"                  S>D
    SPACE               SPACES              SWAP

    THEN                TYPE                U.                  U<
    UM*                 UM/MOD              UNLOOP              UNTIL
    VARIABLE            WHILE               WORD                XOR

CORE EXT

    0>                  0<>                 2>R                 2R>
    2R@                 <>                  AGAIN               HEX
    NIP                 TUCK

CORE (removed looping constructs if we go with Machineforth-style recursion)

    +LOOP               DO                  I                   J
    LEAVE               LOOP                RECURSE             REPEAT
    UNLOOP              UNTIL               WHILE

CORE (immediate)

    POSTPONE            [                   [']                 [CHAR]
    ]

CORE (not implemented)

    (
    SIGN

    #                   #>                  #S                  <#
    >IN                 ALIGN               ALIGNED             BASE
    CELL+               CELLS               CHAR+               CHARS
    DEPTH               ENVIRONMENT?        EVALUATE            FM/MOD
    HOLD                SM/REM              SOURCE              STATE

CORE (maybe don't need?)

    UM/MOD              S>D                 U.                  U<
    UM*                 M*

CORE (questions)

* Do we need `,` and its like?  We can't copy data space to EEPROM, so it seems like `,` doesn't make sense.  Instead you should name data (`CREATE hdr 8 ALLOT`) and then provide an initializer in your `GO` word..?
  * That doesn't work either, because we have no way of knowing how much space should get copied to EEPROM.
  * I think that CREATE (and DOES> and , and ..?) need to go away and be replaced with some other thing that creates a "buffer" word.  Such a word just dumps a literal on the stack of its own address and then does an EXIT, with the remainder of the body of the word containing the buffer.  Copying the word to EEPROM also copies the data.
  * We need something here, because if you just want a scratch buffer then we can't screw you by making you materialize that whole thing in EEPROM.
    * Maybe there is no CREATE (or ALLOT or , or ..?) and instead you just call a word at `GO` time that creates a buffer you using `ALLOCATE` which you then store in a `VARIABLE`.  That uses standard malloc, just like how our dictionary used malloc.
  * Now, what about constant data?  That's the other thing that CREATE and , are for.
    * **I guess we do need CREATE and , (and might as well do ALLOT) and will just have to figure out how long the word is either by looking at HERE or the start of the next word.**
    * **Definitely need ALLOCATE as well though.**

## Interesting Pivots

### 9-bit tokens

* Use "9"-bit tokens -- 64 for RAM and 256 for flash -- by storing "extension bits" in the dictionary header for the word.  So when the word is executing you augment each opcode in order to determine if you should lookup from RAM or flash.  If you eliminate loops then you can just shift this token as you go through the word, refilling when you get to empty.

### Token+Indirect Threading

* Use 7-bit tokens to identify (CORE+other) Forth words in the kernel.
* Use a clear high-bit to indicate a 16-bit indirect threaded word located in RAM (which you then just shift-left one and jump to in the 16-bit RAM address space).

Eliminates the need for the RAM-based lookup table at the expense of 2x-sized RAM words.  But you probably won't have very many RAM words, so this probably ends up as a net win.

Also, makes store/load of RAM words to/from EEPROM and Flash easier, because you can just rewrite the jumps as you store/load the words.  Contrast this with opcodes, which completely change depending on which words have been stored.  Probably not actually that different, but feels better for some reason...

Downside is you only have 128 words total in Flash, which is only going to be enough for Forth and not enough for Arduino Libraries.  Possible solution there is to compile trampoline words into RAM as you reference them during interactive development.  In other words, hundreds of Library words are available in the Flash dictionary, but do not have opcodes.  Then if you reference one interactively we create a trampoline word that can be referenced using the 16-bit indirect-threaded address.  The trampoline word just issues a jump directly to the Flash routine.  This lets us offer hundreds of Library words without any runtime cost until/unless individual words are actually used.  Also, this should make it possible to create "\*\_arf.h" headers that you can include after the library which then just do:

    ARF_LIBRARY_WORD("eepromRead", EEPROM.read, 1);
    ARF_LIBRARY_WORD("eepromWrite", EEPROM.write, 2);

etc.  Those macros add something to the compiled ARF dictionary, again, on the theory that we have tons of flash and should blow it on making things available interactively.

Another nice thing about this approach is that the trampoline words can easily be stored in Flash/EEPROM as well, because they just look like normal, runtime-added words.  *i.e.,* they get referenced through indirect-threading, are added as dependencies, etc.

#### Hybrid Dictionary

Dictionary information is normally stored in the definition, but this seems wasteful for things like Library Word Trampolines, which only exist to provide an indirect reference, and yet still need a dictionary entry since they are inserted at runtime during interactive development.  These are going to be words with long names as well.

I feel like we could do something weird where our dictionary header (the NFA and stuff) is a pointer to the name instead of the name itself.  Then this name could exist in Flash for Library Word Trampolines (thus reusing the extant string in Flash) or to a location in RAM if this is a user-defined word.

In fact, this can get us the benefit of the reverse-word thing that we do in MFORTH, but without reverse words!  The NFA can point to HERE-n, where n is the length of the word.  This way we can always jump to the name in constant time while still being able to have everything be fixed relative to the CFA.  Yes, getting to the name is an extra jump, but who cares?  We rarely need the name and only ever during compilation or interactive dumping.

In fact, this can get us the benefit of the reverse-word thing that we do in MFORTH, but without reverse words!  The NFA can be either a high-bit set negative offset into RAM or a high-bit clear absolute address in Program Memory.  This way we can always jump to the name in constant time while still being able to have everything be fixed relative to the CFA.  Yes, getting to the name is an extra jump, but who cares?  We rarely need the name and only ever during compilation or interactive dumping.

Sample dictionary entry for a user-defined word:

    | 'F' | 'O' | 'O' | %10000011 | $1400 | ... |

Here, NFA is a -3 offset (and so the high bit is set, indicating a relative NFA).  Absolute value of offset is also the length.

And for a Library Word Trampoline ($001234 is address of NUL-terminated name in program memory, $00 is the Call Native/FFI opcode, $012486 is address of library function in program memory, #args is the number of args to the native function, $7F is the opcode for EXIT):

    | $001234 | $1408 | $00 | $012486 | #args | $7F |

Here the high bit is clear and NFA is a 24-bit absolute address into Program Memory.  Presumably we could use #defines to limit this to 16-bit on smaller platforms (such as the ATMega328P with only 32KB of Flash).

FFI opcode reads the address of the function, the number of args, sets up the registers (per <http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_reg_usage>, I think...), calls the function, pushes the result (r25:r24) onto the stack, then executes the next word.  More/Better info: <http://gcc.gnu.org/wiki/avr-gcc#Register_Layout>

Interesting note: this could essentially be the only opcode, because you could just access every Forth kernel word this way...  The other opcodes are just for efficiency (no trampoline needed).  I guess we need $7F EXIT to pop the return stack into the Forth IP.

We should think about having a series of opcodes, one for each number of args (0..10) for rapid conversion of the Forth stack into an FFI call.

References for adding assembly code to Arduino libraries:

* <http://nerdralph.blogspot.com/2013/12/writing-avr-assembler-code-with-arduino.html>
* <http://gcc.gnu.org/wiki/avr-gcc#Register_Layout>
* <http://ucexperiment.wordpress.com/2013/06/03/gcc-inline-assembler-cookbook/>
* Not as useful/accurate, but just in case: <http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_reg_usage>
* Important information on conserving RAM (and Flash) on an Arduino: <http://www.fourwalledcubicle.com/AVRArticles.php>

**NOTE** significantly changes the Workflow described below, because you just include the `\*\_arf.h` files for whichever libraries you want to make available.  You don't actually have to write the FFI word yourself, in other words.

##### Jump Table

It's pretty hard (impossible?) to get gcc-avr to automatically generate a jump table from a switch statement (as in <http://stackoverflow.com/questions/3250178/avoid-range-check-on-switch-case-statement-in-gcc>).  This is probably due to the fact that the AVR is a Harvard architecture and so accessing Program Memory requires special instructions (LPM).  Even doing the GCC goto-label trick (<http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html>) doesn't seem to work since avr-gcc is apparently copying that into RAM as initialized data.

Ah!  The combination is to use the goto-label trick combined with the `PGMSPACE` macro and then `pgm_read_word`.  This causes our jump table to be stored in Program Space.

There are still a couple of unnecessary instructions here though, because GCC doesn't know that we can just blow away Z, for example.  Instead, it tries to retain Z across jumps.  We could probably reduce the number of instructions at use if we create an assembly version of arfInnerInterpreter that has careful knowledge of which registers are being used and then just blows away Z as part of the IJMP, issuing a dedicated JMP back to the top of the loop.

Here is what is currently being done (R30:R31/Z contains the opcode):

```asm
;; Get Z to point at the jump table + opcode.
LDI R31,0x00	Load immediate
LSL R30		    Logical Shift Left
ROL R31		    Rotate Left Through Carry
SUBI R30,0x98	Subtract immediate
SBCI R31,0xFF	Subtract immediate with carry

;; Load R24:R25 with the address of the label.
LPM R24,Z+		(3) Load program memory and postincrement
LPM R25,Z		(3) Load program memory

;; Push the label onto the stack and then 'return' to that label.
PUSH R24		(2) Push register on stack
PUSH R25		(2) Push register on stack
RET 		    (4) Subroutine return
```

`PUSH` + `PUSH` + `RET` is 8 instructions, but instead we could do `MOVW` and `ICALL` which is 4 instructions.  That's a 2x improvement, although probably in the grand scheme of things it doesn't matter, given everything else that is happening here.

The best (?) speed up is likely to be an assembly version of arfInnerInterpreter.  Then we could store a bunch of stuff in registers -- R2-R17 are call-saved -- and minimize the overall number of instructions here.  The jump table could stay in R2:R3, for example, and so then jumping to an opcode becomes very efficient:

```asm
;; R2:R3 contains jump table base.
;; R4:R5 contains current opcode.

LSL R4:R5       ;; opcode * 2
MOVW Z,R2:R3    ;; get jump table base in Z
ADD Z,R4:R5     ;; add opcode to Z
LPM R24,Z+      ;; get label in Zlo
LPM R25,Z       ;; and Zhi
ICALL           ;; jump to label
```

This is incomplete, because we can't LSL the opcode as a word.  That's why the original code clears R31 and then LSLs through carry to R31.  The only optimization we can really make here would be to avoid the subtract-immediate thing by storing the jump table in a register.  We might be able to do that anyway though, which we'll try shortly.

Okay, you can load program memory a single time, store that in a register, and then do math on the register.  That looks like this:

```asm
;; ... more or less the same; ADDs vs. SUBs

;; Load Z with the address of the label.
LD R0,Z+		(2) Load indirect and postincrement
LDD R31,Z+0		(2) Load indirect with displacement
MOV R30,R0		(1) Copy register

;; Jump to the label.
IJMP 		    (2) Indirect jump to (Z)
```

That is 14 vs. 7 instructions, which is a pretty big win for this loop.

This doesn't work, because we need to LPM as part of decoding, otherwise we're just back in RAM again.  Maybe we can put the table into a register and save a few instructions there at least...

What we want is this:

```asm
;; ... exact same as original; Z points at jump table offset

;; Load Z with the address of the label.
LPM R0,Z+		(3) Load program memory and postincrement
LPM R31,Z		(3) Load program memory
MOV R30,R0      (1) Copy register

;; Jump to the label.
IJMP 		    (2) Indirect jump to (Z)
```

That only saves 5 instructions though and doesn't feel worth us hand-coding stuff.  I just wish we could get gcc to blow away Z!  Let's think, what does Z contain...  It contains the opcode.  Then it contains the offset.  Then we use it for loading.  We never need the offset again in the loop, so why is gcc not reusing the register?  It must be that gcc's goto-label just isn't smart enough to assemble that code.  I guess we could do some inline assembly here..?  Maybe as a TODO for later.

**Probably not worth** optimizing this right now.  Let's just build out the rest of everything now that we know we are using the jump table instead of the switch.  We should try and get this working enough that we can call it on the Arduino, just to make sure that we aren't building something unusable in that environment...

##### Threading Model and Opcodes

ARF is indirect-threaded because the CFA contains the opcode that runs the word: DOCOLON, DOCREATE, DOVARIABLE, etc.

Words look like this:

    =================HEADER=============== ====CFA==== =======PFA...
    | 'F' | 'O' | 'O' | %10000011 | $1400 | opDOCOLON | opx | opy | $13F0 | ... | opPARENSEMI |

##### Plan

Blink tutorial: <http://www.arduino.cc/en/Tutorial/Blink>

1. Create basic blink Arduino sketch with our ARF library.
2. We'll need to reference that assembly serial sample thing so that we can have .cpp and .h and .S code.
3. Need the basic layout: NEXT (opcode loop through a table, really), EXIT, the stack, maybe a couple of basic opcodes for testing.
3. Add the single zero-op FFI opcode ($00) and EXIT ($7F).
4. Define a "blink" function that does LED high, delay, low, delay.
5. Write some startup code that builds a dictionary in RAM with the right FFI and EXIT calls and stuff.
6. Execute that and see if it blinks.
7. Add the single-op FFI opcode and use it for delay (creating ledOn and ledOff FFIs).
8. Same thing for two-op FFI opcodes so that the entire thing can be in ARF.

Thoughts: No loop constructs, just tail recursion.

Technically we could go for a simpler approach, just to get something working at all:

1. Basic blink sketch with ARF library.
2. ARF.cpp, ARF.h, ARF.S files.
3. NEXT, EXIT, the stack, dictionary traversal.
4. Zere-op FFI opcode ($00) that just calls the "blink" function and doesn't actually do any FFI.
5. Startup code with hardcoded dictionary.
6. Execute that.

## Workflow

* Create sketch, bring in ARF and other Arduino Libraries.
* Define precompiled ARF words.
  * Words for accessing library functions (FFI word; requires writing Arduino functions).
  * App-specific words -- can be written in Wiring (FFI word) or Forth (which then needs some sort of metacompiler).
* Flash the new code.
* Interactively try out your words, define new words, etc.  Dump your words to the metacompiler output when you're ready to build a new bootloader.
* Ideally it should be possible to write your words to EEPROM without having to reflash (for small projects).  This would let people provide a "batteries included" flash that you can then work with entirely from the device itself.

# Arduino Restrictions

* Flash is always bigger than RAM and RAM is always bigger than EEPROM.  Presumably RAM is fastest.
  * 32KB of Flash is the minimum that we will have, then 2KB RAM, then 1KB EEPROM.
* **However** EEPROM is never very big!  2KB at the most, whereas RAM is 8KB starting on the Teensy++ 2.0 and then goes up to 16KB and 64KB.
  * This means that RAM is ultimately the best place to store the interactive dictionary.
  * Also, we can't really execute out of EEPROM given how slow that is likely to be.
* Ideally should run most programs from Flash, which means that we need a meta-compiler or, perhaps even better, a way for ARF to output its own turnkey source.  *i.e.,* you run some command and it spams out a C-style string, #define, whatever that you can then just copy/paste back into your sketch in order to provide your turnkey code.
* Otherwise programs can be typed in and then run from RAM, with
