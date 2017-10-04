# Enforth

Enforth is an embeddable Forth runtime designed for resource-constrained
environments like the [Arduino Uno][].  The code is written in pure C,
passes most of the [ANS Forth test suite][], and even includes a
multitasker!

Please note that Enforth is not done!  And in fact it needs some major
refactoring related to the foreign-function interface.  But it does work
and is fun to play with.

See [this blog post][] for more information on the genesis of Enforth.

[ANS Forth test suite]: http://www.forth200x.org/documents/html/testsuite.html
[Arduino Uno]: https://store.arduino.cc/usa/arduino-uno-rev3
[this blog post]: http://michaelalynmiller.com/blog/2017/10/04/enforth/


# Running on the Arduino

Enforth was designed from the beginning to run on the Arduino Uno and
includes a "blink" sample that can be used to play with Enforth and do
basic control of the Arduino hardware.  Follow these steps to install
Enforth on your Arduino (tested with Arduino IDE 1.8.5):

1. Clone this repository into your Arduino libraries folder (on Windows:
   `Documents\Arduino\libraries`).
2. Open the Enforth Blink example by selecting File -> Examples ->
   Enforth (under "Examples from Custom Libraries") -> Blink
3. Select your board, serial port, etc. as usual.
4. Upload the sketch.

Now connect to Enforth using a serial console (ideally not the Arduino
IDE Serial Monitor) and you should see the following:

```
Enforth (C) Michael Alyn Miller
```

Enforth's foreign-function interface lets you access code written in C.
You can use the `ffis` command to see all of the FFI functions that are
accessible (press the enter key when you see `<CR>`; the rest of the
text is the response from Enforth):

```
Enforth (C) Michael Alyn Miller
ffis<CR> dubnum twoseven pinMode digitalWrite delay
ok
```

Before you can use a function you need to bring it into your local
dictionary using the `use:` word (do not type `ok `):

```
Enforth (C) Michael Alyn Miller
ok use: pinMode
ok use: digitalWrite
ok
```

Those two `use:` invocations bring in the [pinMode][] and
[digitalWrite][] functions from the Arduino library.  We can then use
them to turn the LED on (`ok` has been elided so that you can copy and
paste these examples):

```forth
13 1 pinmode
13 1 digitalwrite
```

Let's turn those into Forth words:

```forth
: init-led 13 1 pinmode ;
: led-on 13 1 digitalwrite ;
: led-off 13 0 digitalwrite ;
```

Now you can type `led-on` to turn the LED on and `led-off` to turn the
LED off.

Let's combine all of those words into a word that blinks the LED on and
off:

```forth
: blink  init-led  begin  led-on  500 ms  led-off  500 ms  again ;
save
blink
```

The only way to stop the blink example is to restart the Arduino, which
is why you do not see an `ok` prompt after you type `blink`.  However,
because of that `save` command we still have the program available on
the device in EEPROM.  Restart the device, type `load`, and then type
`blink` to run the program again:

```
Enforth (C) Michael Alyn Miller
load
ok blink
```

[digitalWrite]: https://www.arduino.cc/en/Reference/DigitalWrite
[pinMode]: https://www.arduino.cc/en/Reference/PinMode


# Multitasking

Enforth includes a full multitasker!  In fact you were already using
this when you followed the example above -- Enforth runs the serial
console in a task, which means that you can run other operations in the
background.  For example, here is how to run the blink example as a task
(this assumes you have run the examples above):

```forth
' blink task
ok
```

Note that you now get the `ok` prompt and that the LED continues to
blink even while you write other code.  There is currently no way to
stop tasks, short of restarting the device.


# Foreign-Function Interop

Enforth uses the native platform's libraries to interface with other
hardware and software on the machine.  Those `pinMode` and
`digitalWrite` calls used in the examples are actually calls to the
Arduino [pinMode][] and [digitalWrite][] functions.  Enforth has access
to those functions because the Enforth `Blink.ino` sketch defined them
as FFI functions:

```c
// FFI definitions
ENFORTH_EXTERN_VOID(delay, delay, 2)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(delay)

ENFORTH_EXTERN_VOID(pinMode, pinMode, 2)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(pinMode)
```

Those definitions all take two arguments (the `2` at the end of the
`ENFORTH_EXTERN_VOID` call) and return zero values (the `_VOID` part).
You can also define functions that return values:

```c
static int doubleNumber(int num)
{
    return num + num;
}

ENFORTH_EXTERN(dubnum, doubleNumber, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(dubnum)
```

The above code defines a function named `doubleNumber` that takes one
number, adds that number to itself, and returns the resulting value.

Note that FFIs are chained together by way of the `#undef/#define` lines
you see in each block above.  This is required in order to allow Enforth
to look up the FFI functions by name at runtime.


# Unit Tests

Enforth can be built on any OS, but running the unit tests currently
requires a 32-bit operating system due to the fact that `*/MOD` fails on
64-bit math.  You can use Cygwin to build and test Enforth, but be sure
you install Cygwin x86, even if you are on a 64-bit Windows PC.

You can run unit tests either on the local machine, or against an
Arduino device connected to the serial port.  Note that the Arduino
device must be loaded with Enforth's "blink" sketch (as described at the
beginning of this document), since that sketch includes a couple of FFI
functions that are used to test the FFI code.

Running the unit tests on your local machine (here, Cygwin x86) can be
done like so:

```sh
$ make -f Makefile.cygwin test
```

That will build and compile Enforth and run the ANS Forth test suite.
The output should look something like the following:

```
TESTING BASIC ASSUMPTIONS
TESTING BOOLEANS: INVERT AND OR XOR
TESTING 2* 2/ LSHIFT RSHIFT
TESTING COMPARISONS: 0= = 0< < > U< MIN MAX
TESTING STACK OPS: 2DROP 2DUP 2OVER 2SWAP ?DUP DEPTH DROP DUP OVER ROT SWAP
TESTING >R R> R@
TESTING ADD/SUBTRACT: + - 1+ 1- ABS NEGATE
TESTING MULTIPLY: S>D * M* UM*
TESTING DIVIDE: FM/MOD SM/REM UM/MOD */ */MOD / /MOD MOD
TESTING HERE , @ ! CELL+ CELLS C, C@ C! CHARS 2@ 2! ALIGN ALIGNED +! ALLOT
TESTING CHAR [CHAR] [ ] BL S"
TESTING ' ['] FIND EXECUTE IMMEDIATE COUNT LITERAL POSTPONE STATE
TESTING IF ELSE THEN BEGIN WHILE REPEAT UNTIL RECURSE
TESTING DO LOOP +LOOP I J UNLOOP LEAVE EXIT
TESTING DEFINING WORDS: : ; CONSTANT VARIABLE CREATE DOES> >BODY
TESTING SOURCE >IN WORD
TESTING <# # #S #> HOLD SIGN BASE
TESTING >NUMBER HEX DECIMAL
TESTING OUTPUT: . ." CR EMIT SPACE SPACES TYPE U.
 !"#$%&'()*+,-./0123456789:;<=>?@
ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`
abcdefghijklmnopqrstuvwxyz{|}~
0 1 2 3 4 5 6 7 8 9
0123456789
A B C D E F G
0  1  2  3  4  5
LINE 1
LINE 2
TESTING DICTIONARY SEARCH RULES
TESTING +LOOP (Enforth)
TESTING FFI (Enforth)
===============================================================================
All tests passed (610 assertions in 22 test cases)
```

The same tests can be run against Enforth on an Arduino Uno.  First,
load the Enforth blink sketch onto your device, then run the following
command:

```sh
$ ENFORTH_PORT=/dev/ttyS0 make -f Makefile.cygwin sertest
```

Replace `/dev/ttyS0` with the path to serial port on which your Arduino
is connected.  The output is much more verbose than the local test run
since it shows you all of the data exchanged on the serial port.  Serial
tests also take longer to run, since they test code restarts the device
after every group of tests (in order to not overflow the Arduino RAM
with test definitions).


# License

Copyright 2008-2017 Michael Alyn Miller

Licensed under the Apache License, Version 2.0 (the "License"); you may
not use this file except in compliance with the License.  You may obtain
a copy of the License at

<http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


<!-- vim: set tw=72 ts=4 sts=4 sw=4 et ai fo=troqn flp=^\\s*\\d\\+\\.\\s\\+\\|^\\s*[-*]\\s\\+: -->
