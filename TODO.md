* We need to switch to MFORTH-style reverse names so that we can put the real XT (the pointer to the LFA) in definitions and stuff and then just do math to go from there to the PFA.  Right now we're doing all of this awful stuff in order to convert from XTs to LFAs to PFAs to ... and it's getting bad.  Also, sometimes an "XT" points at the LFA and sometimes it points at the PFA.  Just bad overall.
  * Change DefGen to output ROM Definitions this way.  LFAs are now XTs (including their MSB-first byte order) and not offsets (although of course XTs are just flagged offsets).  Fix `FIND-ROMDEF` et al.  The inner interpreter should expect to have all XTs point at the NFA (flags) and then do basic math to get to the CFA and PFA.  User Definitions will no longer work.
  * User definitions are still being CREATEd with backwards LFA byte ordering, the name in the wrong place, etc.  This means that they can be called, and invoked via EXECUTE, but will not work when invoked by the inner interpreter (since the name is in the way).  Fix all of that.  `CREATE` and `>BODY` are the biggest culprits here, I think?
* Should be able to eliminate kDefinitionNames and the associated lookup functions now that we have CFAs in definitions -- just put the Code Primitives in the ROM Definition block with only a CFA.  That should allow the token to get called, but W *et al* to be left alone (since we're not calling a `DO*` word).
  * We'll do this after we go to reverse names since that will also make XTs the same everywhere.
  * Another benefit to this unified approach -- the fact that XTs are the same everywhere, including in the LFA -- is that `FIND-WORD` itself can traverse LFAs and use the XT flags (User vs. ROM) to load the string into RAM and then do a `STRING~` on that.  No more `FIND-PRIM`, `FIND-DEF`, etc.  Instead, we only need individual words to read the names of those things.
* XTs are MSB-first, but LFAs (and literals, I think?) are LSB-first; should we make all of that MSB first?
  * I think that this goes away if we just unify XTs everywhere per the above item.
* Modify DefGen to read code primitive EDN data from `/****`-prefixed comments in the `enforth.c` file.  Then rename the `primitives` directory to `definitions` and have it only include ROM definitions.
* Consider additional de-duplication of the Code Prims and ROM Definitions.
  * `I` could compile `R@` instead of providing its own token.  Same thing with `(DO)` and `2>R`
  * `(LOOP)` could maybe always be `(+LOOP)` with a `:charlit 1` in front?
  * Rewriting `DUMP` to use `BEGIN/REPEAT` instead of `DO/LOOP` eliminates `PIQDO`, `PILOOP`, and `PIPLUSLOOP`.
  * `TICKNAMES` could just put a reference to names in the `vm` structure so that it could be accessed with `VM`.
* Consider further adjustments to the XT flags to make FFIs faster and perhaps simplify the actual demux logic in enforth.c
  * We could actually use an $800-prefixed XT for ROM definitions and then uses bits 12-14 for specifying the FFI arity of FFI trampolines.  FFI trampolines would then just be a raw function pointer compiled in to the dictionary.
* We could use relative LFAs with automatic one-byte encoding in cases where the value is less than 256.  This would shave about 240 bytes of the size of the ROM Definition.
* Refactor the DefGen code to make it easier to load in the definitions and then traverse them for analysis purposes.  First analysis: output a GraphViz file that shows the calling patterns between all of the words.
* Start creating the `enforth_*_extern.h` files for various Arduino libs in order to validate the FFI code, workflow, etc.
  * Consider creating a namespace enum for externs so that we can rewrite the FFIDef addresses after a load.  The trampoline would then contain the 16-bit id of the extern (10 bits for namespace, 6 bits for function).
* PARSE-WORD needs to treat all control characters as space if given a space as the delimiter.
* Improve the stack checking code.
  * First, the code is probably too aggressive and may not let us use the last stack item.
  * Second, we have the macro scattered everywhere, but it would be better if the stack sizes were declared in a separate table, organized by token, and then checked in a single place right before DISPATCH\_TOKEN.  Similar to the rest of these tables, the source auto-generator will make it easier to build this table.
  * We should also check the return stack.
* Consider using the pgmspace typedefs (prog\_int8\_t, etc.) if that would make it easier to catch situations where we forgot to use the pgm\_\* accessors.
* Consider creating EnforthDuino.cpp/.h wrappers to make it easier to interact with Enforth in the Arduino environment.
* Add comments to all of the `.edn` files.
* Modify DefGen so that it puts the 0xFF into the names table as soon as the last named primitive is seen (instead of adding in all of those wasted 0x00 bytes).
* Do something about absolute RAM addresses on the stack, in variables, etc.  These prevent the VM from being saved to/from storage (such as EEPROM).
  * We can't relativize everything on save, because we don't always know what we are looking at -- how do we know that a dictionary variable contains a RAM address?  We could probably relativize all addresses in the VM though and then `@`, `!`, etc. would do the adjustment as necessary (and could offer bounds-checking).  All of these addresses are VM-relative and that VM base address will probably end up being stored in a constant register pair.  Access to memory-mapped CPU resources gets messy (this is mostly an ARM problem), although we could offer special fetch and store operations for those.  Similarly, FFI interop involving addresses is now a problem because we need to convert those back and forth.
  * Note that the VM itself has quite a few absolute addresses (DP, HERE, SOURCE, etc.) and we'll need to deal with those on load/save.  Most of these have to do with the text interpreter though and we could easily just say that persistence resets the state of the text interpreter and can only be performed when *not* in compilation mode.  That would leave a very small number of pointers in the VM and those could just be serialized as part of persisting the dictionary.
* Move `dp` and `latest` into the dictionary so that they load/save with the dictionary.
* Add a single default task and move the stacks and BASE into that memory area.  No `PAUSE` yet.
  * Tasks are 32 return stack cells (64/128 bytes), 16 data stack cells (32/64 bytes), 16 user cells (32/64 bytes) for a total of 128/256 bytes per task.
    * Note that tasks go into the dictionary and not at the end!  This allows dictionaries to be resized or only partially copied to storage.
    * Need one task cell to act as a link to the previously-created task as well as a global that points to the newest task (similar to the dictionary itself).
* Forth200x updates (mostly just `TIB` and `#TIB`?, although numeric prefixes look very useful).
* Add `PAUSE`, which spills the registers to global variables in `vm` and then returns from `go()` similar to what we did in Ficl.
* Add dumb exceptions that just restart the VM?
* Consider optimizing the size of `DOCOLON` references by creating "bank-switched" versions of this token.  This would reduce the number of times that calling a definition needs three bytes instead of just two bytes (the primary downside to dictionary-relative instead of IP-relative compilation offsets).
  * You could call `DOCOLON0` for offsets 0-511 in the dictionary, `DOCOLON1` for 512-1023, etc.  Eight of these would allow us to span 4KB of dictionary, at which point we would just fall back to absolute references.  Most (?) dictionaries probably won't be greater than 4KB anyway, and at that point you probably have plenty of RAM to blow on three-byte references.
  * `ALIGN` would actually be needed now in order to make each bank span as many bytes as possible.
  * Remember that compiled XTs can never be smaller than 16-bits anyway, so the goal here is to try and keep those XTs at 16 bits most of the time if possible.
* Consider adding `PAD`, perhaps with a configurable size.  Do not use `PAD` in the kernel though so that we can avoid making it a requirement.
* We may not need to blow 16 tokens on the `PDO*` primitives; instead we can just create a dedicated jump table in `EXECUTE` for those tokens.  `EXECUTE` is almost only ever used when we are doing text interpretation, so spilling and filling registers here should be fine.
  * The only thing that this does is save us token space, it still uses the same amount of ROM.  This is only valuable/necessary if we are running low on tokens.
* Since we have more free tokens now we can probably code in some of the most frequently used FFI functions (`pinWrite` and stuff) as tokens, perhaps through compiler directives.
  * I wonder if we can find a way to predefine a set of trampolines in Flash instead of in RAM?  *i.e.,* we reserve the last 32 tokens for precompiled trampolines and then provide a simplified way to build up that flash array.  This table-based method would actually work since it would just be a list of other addresses (which conveniently we already have thanks to the `FFIDEF_*` vars that are being used for the linked list).  This would give users a way to modify their enforth compile to predefine externals in a way that consumes no RAM.  You still need to define the FFIs, but you don't need to reference them at runtime.
  * This feels like a good balance between ROM and RAM: you can access any FFI at runtime if you are willing to consume memory on that (which is probably fine during development) and then you switch to a ROM-based FFI primitive once you know you'll be using an FFI a lot.  This breaks your flash, of course, but your source is unchanged (and we could make the `EXTERNAL:` word just do nothing in the case where you are trying to reference a ROM-based FFI primitive).
  * This makes the ATtiny85 possible again, because we'll just define the primitives that we care about as ROM primitives.
* Consider different multipliers for the ROM definitions.
  * Maybe we should multiply the token by a prime (3?) instead of 4 so that we can pack these in more/perfectly tightly?  Something along the lines of a Golomb Ruler?  Multiplication on the AVR takes two cycles, just like two left shifts (for x4), so we might as well multiply if it gets us better packing.
