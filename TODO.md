* Fix multi-line definitions (currently just aborts with UNDERFLOW).
* Move `dp` and `latest` into the dictionary so that they load/save with the dictionary.
* Make ROM definition IPs on the return stack relative to the start of the ROM definition block.  We can do this now that all of the ROM definitions are finally in this one block.
* Add flow control words (`IF`, `THEN`, `DO`, `WHILE`, etc.).
* Modify `test/enforth` to optionally take a list of files on the command line and then interpret each file in order (by just feeding the data through `KEY` for now).  This will allow us to start running the anstests.
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
* Add a single default task and move the stacks and BASE into that memory area.  No `PAUSE` yet.
  * Tasks are 32 return stack cells (64/128 bytes), 16 data stack cells (32/64 bytes), 16 user cells (32/64 bytes) for a total of 128/256 bytes per task.
    * Note that tasks go into the dictionary and not at the end!  This allows dictionaries to be resized or only partially copied to storage.
    * Need one task cell to act as a link to the previously-created task as well as a global that points to the newest task (similar to the dictionary itself).
* Consider adding [Catch](https://github.com/philsquared/Catch)-based unit tests in order to augment the anstests-based tests.  Maybe even run the test suite using Catch?
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
