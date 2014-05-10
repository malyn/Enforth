* Add optional stack-checking macros (probably just for non-microcontroller builds).
* Fix up all of the types; sometimes we use uint8_t, sometimes we use arfInteger, etc.
* Turn `dictionary` constructor parameter into a generic `vm` parameter.  Put data stack and return stack at the end of the `vm` buffer.  Put TIB at the start of the buffer (before the dictionary, just like MFORTH).  Allow stack sizes and TIB size to be configured in the constructor.  TIB can be zero if you do not need the text interpreter.
* "Fix" stack usage so that we don't have to waste the last (31st, currently) cell on a TOS value that will never be stored there.  Should probably put the stacks at the beginning of the buffer so that we don't have the issue with sometimes reading TOS from beyond the data stack when the stack is in fact empty.  So organization is: data stack, return stack, TIB, hidden defs, dictionary.
* Move hidden defs into Flash and add a local variable to the inner interpreter that that switches between RAM and Flash threading (we'll need a dedicated CFA for this).
* Implement compilation (`:`, `COMPILE,`, `;`, etc.).
* Reorganize/Clean up source files.  Maybe auto-generate opcodes so that we can avoid some of the duplication, for example.
