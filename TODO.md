# Before Release

* Implement the remaining `CORE` words (the ones whose tests have been commented out in `test_core.cpp`).
* Do we need `I*` tokens or can normal tokens just use inProgramSpace?  (and if so, is that actually smaller from a compile perspective?)
  * Looks like we can make this change and that it will be (slightly) smaller, if not (slightly) slower.  We shouldn't make the change until we can run the tests on the Arduino though (just in case we break something).
* Implement `.S`.
* Drop enforth\_test back to 1KB so that it works with `LOAD` and `SAVE`, presumably by splitting up that one test so that it doesn't consume all of the RAM.
* Most of `FOUND-FFIDEF?` is just `FOUND?`; we should find a way to merge that code.
  * FFI definition names are stored normally (forward order) which means that `FOUND?` cannot use `STRING~XT` for comparing FFIs.  We should put definitions in forward order and then just do subtraction to jump to the start of the definition.  Then we can use `FOUND?` for everything.
    * We should drop the terminal bit thing as well; we're not using that anywhere and it's just annoying to constantly mask off.
  * As an FYI, we can't easily remove `FIND-FFIDEF` (even though it looks like `FIND-WORD`), because it is chaining through defs using absolute addresses and not XTs.
* Dictionary search (probably `FIND-WORD` traversal) seems pretty slow.  We should profile it and see about moving that (back) to C.
* Externs should be separate libraries and Git repos.
* Create more `externs/enforth_*.h` files for various Arduino libs in order to validate the FFI code, workflow, etc.
  * Especially interesting to determine is the maximum number of FFI args that are actually need.  We currently support 8, but something like 4 would probably be better.
  * Need to test with things take take/return long (such as Arduino's [randomSeed](http://arduino.cc/en/Reference/RandomSeed) and [random](http://arduino.cc/en/Reference/Random)) and see how those work.  We might need a variant of `ENFORTH_EXTERN` that pushes two cells, for example.
    * For example, `delay` takes a long, but it's not in the right (`100 S>D`) format.  This seems bad...
    * Or do we wrap all of these in `ENFORTH_EXTERN_METHOD` (which then needs a better name...) and manually shift the values?
    * Probably need a variant of the macro (or a `DOFFI*`?) that returns a long and does a double-push as well.
    * Double-push may require an entire duplicate set of `DOFFI*` tokens, which is kind of lame.  Maybe we should have `ENFORTH_EXTERN_METHOD` (or its better-named replacement) just do the push and pop itself using enforth.h-provided `enforth_push` and `enforth_pop`?  That starts to mess with our register variables then...  I suppose duplicating `DOFFI*` is better?
* PARSE-WORD needs to treat all control characters as space if given a space as the delimiter.
* Improve the stack checking code.
  * First, the code is probably too aggressive and may not let us use the last stack item.
  * Second, we have the macro scattered everywhere, but it would be better if the stack sizes were in an extra byte in the definition header and then checked in a single place right before DISPATCH\_TOKEN.  This may make the logic small enough to include on AVRs (although it will add ~240 bytes to the size of the ROM Definition block).
* Consider additional de-duplication of the Code Prims and ROM Definitions.
  * `I` could compile `R@` instead of providing its own token.  Same thing with `(DO)` and `2>R`
  * `TOKEN,` could be `C@`.
  * `(LOOP)` could maybe always be `(+LOOP)` with a `:charlit 1` in front?
  * DefGen could implement the above optimizations, that way the code always reads nicely, even though it is being rewritten during compilation.  Note that this doesn't work for `I`.
  * Is there any benefit to defining `DOCONSTANTROM` for storing constants in ROM PFAs?  Currently we define tokens or words that calculate and return constants.  This would free up tokens, but then we would have to modify DefGen to know about int sizes...  Seems hard.  I suppose DOCONSTANTROM could always take 16-bit constants (as long as we didn't need negative numbers)?
  * Can we eliminate some of the `DOUBLE` words that are used for things like parsing and output?
* Fix tracing now that kDefinitionNames has gone away.
* Create a `:profile` property on each word that lets us build smaller versions of the ROM.  For example, maybe you eliminate `DOUBLE` support or the `TOOLS` so that you can get down to something that fits on ATtiny85.  Code Primitives should be included in this as well and then jump table entries for elided primitives should not be generated (so that the compiler will remove that code).
* Consider creating EnforthDuino.cpp/.h wrappers to make it easier to interact with Enforth in the Arduino environment.  Mostly just to wrap the serial code, allocate the dictionary block, implement EEPROM-backed load/save, etc.
  * Note that this should be a separate library/repo as well.
* Add comments to all of the EDN blocks (probably in a new property so that we can extract it during analysis or to generate docs).
  * Should also add a `:usage` property that we can optionally compile into ROM.  Then a `HELP` word could be written to output that usage line.  Just the stack effects and a short description of the word.
* Move to [Arduino 1.5 library format](https://github.com/arduino/Arduino/wiki/Arduino-IDE-1.5:-Library-specification) now that 1.0.6 supports that format?
* Support 64-bit platforms (requires more `#ifdef`s and some trickiness in `*/MOD`).
* Consider a switch-based version of the inner loop for compilers like Visual Studio that do not support jump-threading.  All of the labels will need to be come BEGIN\_CODEPRIM(DUP)/END\_CODEPRIM(DUP) or something like that.
* Make a tEnforthTask structure for easily accessing structures in the C code.
* Improve task code (see OmniFocus notes).
* Should the `OPERATOR` task be outside of the dictionary so that the dictionary can load/save without packing along the 128-byte task?  This could be in the VM structure and linked just like normal.  enforth\_interpret (which doesn't exist yet) could then start the interpreter on the `OPERATOR` task.  This would make it explicit that the operator state does not load/save.  Maybe we could even pack along some of the other variables this way... (`>IN` and `SOURCE` and stuff could be USER variables?).
  * Users could still evaluate things (such as their turnkey word) on the operator task (although they wouldn't know that); it just wouldn't load/save with their dictionary.  That seems fine, since loading/saving your task state isn't going to work anyway since you re-run the turnkey word at startup.
* Address all TODOs and FIXMEs in the code and definitions.
* Create a 16-bit `r-addr` (relative address) concept and then use that everywhere instead of XTs?  Create a rel\_to\_absolute C function (and inverse) that can be used for mapping to/from relative addresses.  Also need a macro that can tell you if a relative address is in ROM (for AVR).
  * Test this behavior/function by modifying the test code to create a new dictionary after every enforth\_evaluate and copy the old dictionary into that new one.  Forces every address to be relative or the tests will crash.

# After Release

* Consider further adjustments to the XT flags in order to reduce the size of XTs in certain situations.  For example, the XTs could support relative-RAM and relative-ROM variants for spanning definitions as large as 32-bytes (5-bits) if we could get away with dedicating an extra bit to that vs. an absolute (offset-based) address.
  * ROM XTs only need to be 11 bits (assuming that we align to two bytes), for example.
  * This is where a constraint solver could help: perhaps a specific ordering of the words in the dictionary produces an optimal (from a minimum size perspective) dictionary, assuming that words that call each other can be clustered together.
  * Similarly, if LFAs were always relative, with automatic one-byte encoding, then we could shave about 240 bytes off the size of the ROM Definition.
  * We should also check the return stack.
* Evaluate ways to reduce the size of the DOFFI\* tokens, especially now that we have the VOID variants that auto-drop (that code appears to add hundreds of bytes to the build).
* Consider using the pgmspace typedefs (prog\_int8\_t, etc.) if that would make it easier to catch situations where we forgot to use the pgm\_\* accessors.
* Consider creating a namespace enum for FFI definitions so that we can rewrite the FFIDef addresses after a load.  The trampoline would then contain the 16-bit id of the extern (10 bits for namespace, 6 bits for function).
* Do something about absolute RAM addresses on the stack, in variables, etc.  These prevent the VM from being saved to/from storage (such as EEPROM).
  * We can't relativize everything on save, because we don't always know what we are looking at -- how do we know that a dictionary variable contains a RAM address?  We could probably relativize all addresses in the VM though and then `@`, `!`, etc. would do the adjustment as necessary (and could offer bounds-checking).  All of these addresses are VM-relative and that VM base address will probably end up being stored in a constant register pair.  Access to memory-mapped CPU resources gets messy (this is mostly an ARM problem), although we could offer special fetch and store operations for those.  Similarly, FFI interop involving addresses is now a problem because we need to convert those back and forth.
  * Note that the VM itself has quite a few absolute addresses (DP, HERE, SOURCE, etc.) and we'll need to deal with those on load/save.  Most of these have to do with the text interpreter though and we could easily just say that persistence resets the state of the text interpreter and can only be performed when *not* in compilation mode.  That would leave a very small number of pointers in the VM and those could just be serialized as part of persisting the dictionary.
  * The `r-addr` concept should make this possible.  Then all of the standard words would operate on relative addresses and we would provide fetch and store words for processors that actually need you to access memory outside of the dictionary.
    * This is not great though, because we already have Arduino libraries (such as Microview) that have a big RAM block that we are probably going to want to poke at.  I think that we just won't be able to relativize addresses.
    * Maybe this is okay though if `CREATE` and `VARIABLE` return absolute addresses based on the current dictionary address (which I think happens anyway).  Then all of the dictionary-relative address will in fact be relative and only things that are outside of that will be absolute.  Probably those things won't be changing anyway because, in general, we should say that `LOAD` and `SAVE` do not work if you recompile Enforth itself.
* Forth200x updates (mostly just `TIB` and `#TIB`?, although numeric prefixes look very useful).
* Add dumb exceptions that just restart the VM?
* Consider adding `PAD`, perhaps with a configurable size.  Do not use `PAD` in the kernel though so that we can avoid making it a requirement.
* Since we have more free tokens now we can probably code in some of the most frequently used FFI functions (`pinWrite` and stuff) as tokens, perhaps through compiler directives.
  * I wonder if we can find a way to predefine a set of trampolines in Flash instead of in RAM?  *i.e.,* we reserve the last 32 tokens for precompiled trampolines and then provide a simplified way to build up that flash array.  This table-based method would actually work since it would just be a list of other addresses (which conveniently we already have thanks to the `FFIDEF_*` vars that are being used for the linked list).  This would give users a way to modify their enforth compile to predefine externals in a way that consumes no RAM.  You still need to define the FFIs, but you don't need to reference them at runtime.
  * This feels like a good balance between ROM and RAM: you can access any FFI at runtime if you are willing to consume memory on that (which is probably fine during development) and then you switch to a ROM-based FFI primitive once you know you'll be using an FFI a lot.  This breaks your flash, of course, but your source is unchanged (and we could make the `EXTERNAL:` word just do nothing in the case where you are trying to reference a ROM-based FFI primitive).
  * This makes the ATtiny85 possible again, because we'll just define the primitives that we care about as ROM primitives.  The theory here is that we'll only get 256 bytes or something for the dictionary on ATtiny85 and so we don't want to waste 6 bytes on every Arduino function that we want to call.  I don't know if I buy that though, because the real concern on the ATtiny85 is ROM and we're way over our limit at the moment.
  * This would allow `LOAD` and `SAVE` to be FFIs without consuming RAM -- they would just be the first two tokens in this extra table.
  * All of this should be much easier now that we have the new XT format.  We just need a way to tack some extra stuff onto the definitions table (or use a new XT flag to target this extra definition block).  We can probably implement this using macros since we don't actually need the names here (it's just a bunch of FFI trampolines one after the other).  Probably similar to how the names table worked way-back-when.
* Refactor the DefGen code to make it easier to load in the definitions and then traverse them for analysis purposes.  First analysis: output a GraphViz file that shows the calling patterns between all of the words.
