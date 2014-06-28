/* NUMSIGN */
BASE, FETCH, UDSLASHMOD, ROT, DUP, CHARLIT, 9, GREATERTHAN, CHARLIT, 7, AND, PLUS, CHARLIT, 48, PLUS, HOLD, EXIT, 0,

/* NUMSIGNGRTR */
DROP, DROP, HLD, FETCH, HERE, HLDEND, PLUS, OVER, MINUS, EXIT, 0, 0,

/* NUMSIGNS */
NUMSIGN, TWODUP, OR, ZBRANCH, 3, BRANCH, -6, EXIT, 0, 0, 0, 0,

/* DOT */
BASE, FETCH, CHARLIT, 10, NOTEQUALS, ZBRANCH, 3, UDOT, EXIT, DUP, ABS, ZERO, LESSNUMSIGN, NUMSIGNS, ROT, SIGN, NUMSIGNGRTR, TYPE, SPACE, EXIT, 0, 0, 0, 0,

/* COLON */
CREATE, CHARLIT, kDefTypeCOLONHIDDEN, CHANGELATESTDEFTYPE, RTBRACKET, EXIT,

/* SEMICOLON */
CHARLIT, EXIT, COMPILECOMMA, CHARLIT, kDefTypeCOLON, CHANGELATESTDEFTYPE, LTBRACKET, EXIT, 0, 0, 0, 0,

/* LESSNUMSIGN */
HERE, HLDEND, PLUS, HLD, STORE, EXIT,

/* TOBODY */
DUP, TOKENQ, ZBRANCH, 4, DROP, ZERO, EXIT, DUP, NFALENGTH, SWAP, TOLFA, ONEPLUS, ONEPLUS, ONEPLUS, PLUS, EXIT, 0, 0,

/* TOIN */
VMADDRLIT, offsetof(EnforthVM, to_in), EXIT, 0, 0, 0,

/* TONUMBER */
DUP, ZBRANCH, 29, OVER, CFETCH, DIGITQ, ZEROEQUALS, ZBRANCH, 2, EXIT, TOR, TWOSWAP, BASE, FETCH, DUP, TOR, UMSTAR, DROP, SWAP, RFROM, UMSTAR, ROT, PLUS, RFROM, MPLUS, TWOSWAP, CHARLIT, 1, SLASHSTRING, BRANCH, -30, EXIT, 0, 0, 0, 0,

/* TOUPPER */
DUP, CHARLIT, 'a', ONEMINUS, GREATERTHAN, OVER, CHARLIT, 'z', ONEPLUS, LESSTHAN, AND, ZBRANCH, 4, CHARLIT, 32, MINUS, EXIT, 0,

/* ACCEPT */
OVER, PLUS, OVER, KEY, DUP, CHARLIT, 10, NOTEQUALS, ZBRANCH, 43, DUP, CHARLIT, 8, EQUALS, OVER, CHARLIT, 127, EQUALS, OR, ZBRANCH, 18, TWOOVER, DROP, NIP, OVER, NOTEQUALS, ZBRANCH, 9, ONEMINUS, CHARLIT, 8, EMIT, SPACE, CHARLIT, 8, EMIT, BRANCH, 13, DUP, TWOOVER, NOTEQUALS, ZBRANCH, 7, EMIT, OVER, CSTORE, ONEPLUS, BRANCH, 2, TWODROP, BRANCH, -48, DROP, NIP, SWAP, MINUS, EXIT, 0, 0, 0,

/* BASE */
VMADDRLIT, offsetof(EnforthVM, base), EXIT, 0, 0, 0,

/* BL */
CHARLIT, ' ', EXIT, 0, 0, 0,

/* COMPILECOMMA */
DUP, TOCOMPILETOKEN, CCOMMA, TOBODY, QDUP, ZBRANCH, 6, VMADDRLIT, offsetof(EnforthVM, dictionary), FETCH, MINUS, WCOMMA, EXIT, 0, 0, 0, 0, 0,

/* CR */
CHARLIT, 10, EMIT, EXIT, 0, 0,

/* CREATE */
BL, PARSEWORD, DUP, ZEROEQUALS, ZBRANCH, 2, ABORT, HERE, TOXT, LATEST, FETCH, WCOMMA, LATEST, STORE, DUP, CHARLIT, 3, LSHIFT, CHARLIT, kDefTypeCREATE, OR, CCOMMA, TUCK, HERE, SWAP, MOVE, ALLOT, ALIGN, EXIT, 0,

/* CNOTSIMILAR */
CSIMILAR, INVERT, EXIT, 0, 0, 0,

/* CSIMILAR */
TOUPPER, SWAP, TOUPPER, EQUALS, EXIT, 0,

/* EVALUATE */
INTERPRET, EXIT, 0, 0, 0, 0,

/* EXECUTE */
DUP, TOTOKEN, SWAP, TOBODY, PEXECUTE, EXIT,

/* FFIS */
VMADDRLIT, offsetof(EnforthVM, last_ffi), FETCH, QDUP, ZBRANCH, 10, DUP, FFIDEFNAME, OVER, FFIDEFNAMELEN, ITYPE, SPACE, IFETCH, BRANCH, -11, EXIT, 0, 0,

/* FIND */
COUNT, FINDWORD, EXIT, 0, 0, 0,

/* HERE */
VMADDRLIT, offsetof(EnforthVM, dp), FETCH, EXIT, 0, 0,

/* HEX */
CHARLIT, 16, BASE, STORE, EXIT, 0,

/* HOLD */
HLD, FETCH, ONEMINUS, DUP, HLD, STORE, CSTORE, EXIT, 0, 0, 0, 0,

/* LATEST */
VMADDRLIT, offsetof(EnforthVM, latest), EXIT, 0, 0, 0,

/* LITERAL */
DUP, CHARLIT, 255, INVERT, AND, ZEROEQUALS, ZBRANCH, 7, CHARLIT, CHARLIT, CCOMMA, CCOMMA, BRANCH, 5, CHARLIT, LIT, CCOMMA, COMMA, EXIT, 0, 0, 0, 0, 0,

/* QUIT */
INITRP, ZERO, STATE, STORE, TIB, DUP, TIBSIZE, ACCEPT, SPACE, INTERPRET, CR, STATE, FETCH, ZEROEQUALS, ZBRANCH, 6, PDOTQUOTE, 3, 'o', 'k', ' ', BRANCH, -18, 0,

/* SIGN */
ZEROLESS, ZBRANCH, 4, CHARLIT, '-', HOLD, EXIT, 0, 0, 0, 0, 0,

/* SOURCE */
VMADDRLIT, offsetof(EnforthVM, source_len), TWOFETCH, EXIT, 0, 0,

/* SPACE */
BL, EMIT, EXIT, 0, 0, 0,

/* STATE */
VMADDRLIT, offsetof(EnforthVM, state), EXIT, 0, 0, 0,

/* TYPE */
OVER, PLUS, SWAP, TWODUP, NOTEQUALS, ZBRANCH, 7, DUP, CFETCH, EMIT, ONEPLUS, BRANCH, -9, TWODROP, EXIT, 0, 0, 0,

/* UDOT */
ZERO, LESSNUMSIGN, NUMSIGNS, NUMSIGNGRTR, TYPE, SPACE, EXIT, 0, 0, 0, 0, 0,

/* UNUSED */
VMADDRLIT, offsetof(EnforthVM, dictionary), FETCH, VMADDRLIT, offsetof(EnforthVM, dictionary_size), FETCH, PLUS, HERE, MINUS, EXIT, 0, 0,

/* USE */
BL, PARSEWORD, DUP, ZEROEQUALS, ZBRANCH, 2, ABORT, TWODUP, FINDFFIDEF, ZEROEQUALS, ZBRANCH, 8, TYPE, SPACE, CHARLIT, '?', EMIT, CR, ABORT, HERE, TOXT, LATEST, FETCH, WCOMMA, LATEST, STORE, SWAP, CHARLIT, 3, LSHIFT, CHARLIT, kDefTypeFFI, OR, CCOMMA, ALIGN, COMMA, DROP, EXIT, 0, 0, 0, 0,

/* WORDS */
LATEST, FETCH, QDUP, ZBRANCH, 24, DUP, FFIQ, ZBRANCH, 10, DUP, TOFFIDEF, DUP, FFIDEFNAME, SWAP, FFIDEFNAMELEN, ITYPE, BRANCH, 6, DUP, TONFA, OVER, NFALENGTH, TYPE, SPACE, TOLFA, WFETCH, BRANCH, -25, TICKNAMES, DUP, ICFETCH, CHARLIT, 255, NOTEQUALS, ZBRANCH, 23, DUP, ICFETCH, CHARLIT, 3, RSHIFT, TUCK, OVER, PLUS, ONEPLUS, SWAP, ONEPLUS, ROT, QDUP, ZBRANCH, 5, ITYPE, SPACE, BRANCH, 2, DROP, BRANCH, -28, DROP, EXIT,

/* LTBRACKET */
FALSE, STATE, STORE, EXIT, 0, 0,

/* RTBRACKET */
TRUE, STATE, STORE, EXIT, 0, 0,

/* TODEFTYPE */
TOLFA, ONEPLUS, ONEPLUS, CFETCH, CHARLIT, 7, AND, EXIT, 0, 0, 0, 0,

/* TOFFIDEF */
TOBODY, FETCH, EXIT, 0, 0, 0,

/* TOLFA */
WLIT, 255, 127, AND, VMADDRLIT, offsetof(EnforthVM, dictionary), FETCH, PLUS, EXIT, 0, 0, 0,

/* TONFA */
TOLFA, ONEPLUS, ONEPLUS, ONEPLUS, EXIT, 0,

/* TOTOKEN */
DUP, TOKENQ, ZBRANCH, 2, EXIT, DUP, FFIQ, ZBRANCH, 8, TOFFIDEF, FFIDEFARITY, CHARLIT, 232, PLUS, BRANCH, 5, TODEFTYPE, CHARLIT, 224, PLUS, EXIT, 0, 0, 0,

/* TOXT */
VMADDRLIT, offsetof(EnforthVM, dictionary), FETCH, MINUS, WLIT, 0, 128, OR, EXIT, 0, 0, 0,

/* TOCOMPILETOKEN */
TOTOKEN, DUP, CHARLIT, 0xe0 - 1, GREATERTHAN, ZBRANCH, 4, CHARLIT, 16, PLUS, EXIT, 0,

/* CHANGELATESTDEFTYPE */
LATEST, FETCH, TOLFA, ONEPLUS, ONEPLUS, CFETCH, CHARLIT, 248, AND, OR, LATEST, FETCH, TOLFA, ONEPLUS, ONEPLUS, CSTORE, EXIT, 0,

/* COLD */
PDOTQUOTE, 31, 'E', 'n', 'f', 'o', 'r', 't', 'h', ' ', '(', 'C', ')', ' ', 'M', 'i', 'c', 'h', 'a', 'e', 'l', ' ', 'A', 'l', 'y', 'n', ' ', 'M', 'i', 'l', 'l', 'e', 'r', CR, ABORT, EXIT,

/* DIGITQ */
TOUPPER, CHARLIT, '0', MINUS, DUP, ZEROLESS, ZBRANCH, 4, DROP, ZERO, EXIT, DUP, CHARLIT, 9, GREATERTHAN, ZBRANCH, 13, DUP, CHARLIT, 17, LESSTHAN, ZBRANCH, 4, DROP, FALSE, EXIT, CHARLIT, 7, MINUS, DUP, ONEPLUS, BASE, FETCH, UGREATERTHAN, ZBRANCH, 4, DROP, FALSE, EXIT, TRUE, EXIT, 0,

/* FFIQ */
TODEFTYPE, CHARLIT, kDefTypeFFI, EQUALS, EXIT, 0,

/* FFIDEFARITY */
CHARLIT, offsetof(EnforthFFIDef, arity), PLUS, ICFETCH, EXIT, 0,

/* FFIDEFNAME */
CHARLIT, offsetof(EnforthFFIDef, name), PLUS, IFETCH, EXIT, 0,

/* FFIDEFNAMELEN */
FFIDEFNAME, DUP, DUP, ICFETCH, ZBRANCH, 4, ONEPLUS, BRANCH, -6, SWAP, MINUS, EXIT,

/* FINDDEF */
TWOTOR, LATEST, FETCH, QDUP, ZBRANCH, 23, DUP, TWORFETCH, ROT, FOUNDDEFQ, ZBRANCH, 13, DUP, TODEFTYPE, CHARLIT, kDefTypeCOLONIMMEDIATE, EQUALS, CHARLIT, 2, AND, ONEMINUS, TWORFROM, TWODROP, EXIT, TOLFA, WFETCH, BRANCH, -24, TWORFROM, TWODROP, FALSE, EXIT, 0, 0, 0, 0,

/* FINDFFIDEF */
TWOTOR, VMADDRLIT, offsetof(EnforthVM, last_ffi), FETCH, QDUP, ZBRANCH, 20, RFETCH, OVER, FFIDEFNAMELEN, EQUALS, ZBRANCH, 11, DUP, TWORFETCH, ROT, FOUNDFFIDEFQ, ZBRANCH, 5, TWORFROM, TWODROP, TRUE, EXIT, IFETCH, BRANCH, -21, TWORFROM, TWODROP, FALSE, EXIT,

/* FINDPRIM */
TWOTOR, ZERO, TICKNAMES, DUP, ICFETCH, CHARLIT, 255, NOTEQUALS, ZBRANCH, 29, DUP, TWORFETCH, ROT, FOUNDPRIMQ, ZBRANCH, 16, DROP, ICFETCH, CHARLIT, 7, AND, CHARLIT, kDefTypeCOLONIMMEDIATE, EQUALS, CHARLIT, 2, AND, ONEMINUS, TWORFROM, TWODROP, EXIT, ROT, ONEPLUS, ROT, DROP, SWAP, BRANCH, -34, TWODROP, TWORFROM, TWODROP, FALSE, EXIT, 0, 0, 0, 0, 0,

/* FINDWORD */
TWODUP, FINDDEF, QDUP, ZBRANCH, 5, TWOSWAP, TWODROP, BRANCH, 8, TWODUP, FINDPRIM, DUP, ZBRANCH, 3, TWOSWAP, TWODROP, EXIT, 0,

/* FOUNDDEFQ */
DUP, TODEFTYPE, CHARLIT, kDefTypeCOLONHIDDEN, EQUALS, ZBRANCH, 5, DROP, TWODROP, FALSE, EXIT, TWODUP, NAMELENGTH, NOTEQUALS, ZBRANCH, 5, DROP, TWODROP, FALSE, EXIT, DUP, FFIQ, ZBRANCH, 3, FOUNDFFIQ, EXIT, TONFA, SWAP, QDUP, ZBRANCH, 22, ONEMINUS, TOR, OVER, CFETCH, OVER, CFETCH, CNOTSIMILAR, ZBRANCH, 6, TWODROP, RFROM, DROP, FALSE, EXIT, ONEPLUS, SWAP, ONEPLUS, SWAP, RFROM, BRANCH, -23, TWODROP, TRUE, EXIT, 0, 0, 0, 0, 0,

/* FOUNDFFIQ */
TOFFIDEF, FOUNDFFIDEFQ, EXIT, 0, 0, 0,

/* FOUNDFFIDEFQ */
FFIDEFNAME, SWAP, QDUP, ZBRANCH, 22, ONEMINUS, TOR, OVER, CFETCH, OVER, ICFETCH, CNOTSIMILAR, ZBRANCH, 6, TWODROP, RFROM, DROP, FALSE, EXIT, ONEPLUS, SWAP, ONEPLUS, SWAP, RFROM, BRANCH, -23, TWODROP, TRUE, EXIT, 0,

/* FOUNDPRIMQ */
DUP, DUP, ICFETCH, CHARLIT, 3, RSHIFT, PLUS, ONEPLUS, TOR, DUP, ICFETCH, CHARLIT, 3, RSHIFT, ROT, NOTEQUALS, ZBRANCH, 5, TWODROP, RFROM, FALSE, EXIT, ONEPLUS, OVER, CFETCH, OVER, ICFETCH, CNOTSIMILAR, ZBRANCH, 5, TWODROP, RFROM, FALSE, EXIT, ONEPLUS, SWAP, ONEPLUS, SWAP, DUP, RFETCH, EQUALS, ZBRANCH, -19, TWODROP, RFROM, TRUE, EXIT, 0,

/* HALT */
PHALT, EXIT, 0, 0, 0, 0,

/* HLD */
VMADDRLIT, offsetof(EnforthVM, hld), EXIT, 0, 0, 0,

/* HLDEND */
CHARLIT, kEnforthCellSize * 8 * 3, EXIT, 0, 0, 0,

/* INTERPRET */
VMADDRLIT, offsetof(EnforthVM, source_len), TWOSTORE, ZERO, TOIN, STORE, BL, PARSEWORD, DUP, ZBRANCH, 37, FINDWORD, QDUP, ZBRANCH, 14, ONEPLUS, STATE, FETCH, ZEROEQUALS, OR, ZBRANCH, 4, EXECUTE, BRANCH, 21, COMPILECOMMA, BRANCH, 18, NUMBERQ, ZBRANCH, 8, STATE, FETCH, ZBRANCH, 11, LITERAL, BRANCH, 8, TYPE, SPACE, CHARLIT, '?', EMIT, CR, ABORT, BRANCH, -40, TWODROP, EXIT, 0, 0, 0, 0, 0,

/* ITYPE */
OVER, PLUS, SWAP, TWODUP, NOTEQUALS, ZBRANCH, 7, DUP, ICFETCH, EMIT, ONEPLUS, BRANCH, -9, TWODROP, EXIT, 0, 0, 0,

/* NAMELENGTH */
TOLFA, ONEPLUS, ONEPLUS, CFETCH, CHARLIT, 3, RSHIFT, EXIT, 0, 0, 0, 0,

/* NFALENGTH */
DUP, FFIQ, ZBRANCH, 5, DROP, ZERO, BRANCH, 8, TOLFA, ONEPLUS, ONEPLUS, CFETCH, CHARLIT, 3, RSHIFT, EXIT, 0, 0,

/* NUMBERQ */
OVER, CFETCH, DUP, CHARLIT, '-', EQUALS, OVER, CHARLIT, '+', EQUALS, OR, ZBRANCH, 17, CHARLIT, '-', EQUALS, ZBRANCH, 4, TRUE, BRANCH, 2, FALSE, TOR, CHARLIT, 1, SLASHSTRING, RFROM, BRANCH, 3, DROP, ZERO, TOR, TWODUP, ZERO, ZERO, TWOSWAP, TONUMBER, ZBRANCH, 8, DROP, TWODROP, RFROM, DROP, ZERO, BRANCH, 10, DROP, TWONIP, DROP, RFROM, ZEROLESS, ZBRANCH, 2, NEGATE, TRUE, EXIT, 0, 0, 0, 0,

/* PARSEWORD */
TOR, SOURCE, TOIN, FETCH, SLASHSTRING, RFETCH, TOR, OVER, CFETCH, RFETCH, EQUALS, OVER, AND, ZBRANCH, 6, CHARLIT, 1, SLASHSTRING, BRANCH, -12, RFROM, DROP, OVER, SWAP, RFROM, TOR, OVER, CFETCH, RFETCH, NOTEQUALS, OVER, AND, ZBRANCH, 6, CHARLIT, 1, SLASHSTRING, BRANCH, -12, RFROM, TWODROP, DUP, SOURCE, DROP, MINUS, TOIN, STORE, OVER, MINUS, EXIT, 0, 0, 0, 0,

/* TIB */
VMADDRLIT, offsetof(EnforthVM, tib), EXIT, 0, 0, 0,

/* TIBSIZE */
CHARLIT, 80, EXIT, 0, 0, 0,

/* TOKENQ */
WLIT, 0, 128, AND, ZEROEQUALS, EXIT,

