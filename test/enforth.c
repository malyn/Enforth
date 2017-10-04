/* Copyright 2008-2017 Michael Alyn Miller
 * vi:ts=4:sts=4:et:sw=4:sr:et:tw=72:fo=tcrq
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 */

/* -------------------------------------
 * Includes.
 */

/* ANSI C includes. */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/* Curses includes. */
#include <curses.h>

/* Enforth includes. */
#include "enforth.h"



/* -------------------------------------
 * Sample FFI definitions.
 */

/* Externs */
ENFORTH_EXTERN_VOID(clear, clear, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(clear)

ENFORTH_EXTERN(rand, rand, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(rand)

ENFORTH_EXTERN_VOID(srand, srand, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(srand)



/* -------------------------------------
 * Enforth I/O primitives.
 */

static int enforthCursesKeyQuestion(void)
{
    int c = getch();
    if (c == ERR)
    {
        return 0;
    }
    else
    {
        ungetch(c);
        return -1;
    }
}

/* 6.1.1750 KEY
 *
 * ( -- char )
 *
 * Receive one character char, a member of the implementation-defined
 * character set. Keyboard events that do not correspond to such
 * characters are discarded until a valid character is received, and
 * those events are subsequently unavailable.

 * All standard characters can be received. Characters received by KEY
 * are not displayed.

 * Any standard character returned by KEY has the numeric value
 * specified in 3.1.2.1 Graphic characters. Programs that require the
 * ability to receive control characters have an environmental
 * dependency.

 * See: 10.6.2.1305 EKEY , 10.6.1.1755 KEY?
 */
static char enforthCursesKey(void)
{
    int c;
    while ((c = getch()) == ERR)
    {
        /* Repeat. */
    }

    return c;
}

static void enforthCursesEmit(char ch)
{
    /* Output the character and refresh the screen. */
    addch(ch);
    refresh();
}



/* -------------------------------------
 * Enforth storage primitives.
 */

static int enforthLoad(uint8_t * dictionary, int size)
{
    int fd = open("enforth.img", O_RDONLY);
    if (fd == -1)
    {
        return 0;
    }

    if (read(fd, dictionary, size) != size)
    {
        close(fd);
        return 0;
    }

    close(fd);
    return -1;
}

static int enforthSave(uint8_t * dictionary, int size)
{
    int fd = open("enforth.img", O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd == -1)
    {
        return 0;
    }

    if (write(fd, dictionary, size) != size)
    {
        close(fd);
        return 0;
    }

    close(fd);
    return -1;
}



/* -------------------------------------
 * Globals.
 */

static EnforthVM enforthVM;
static unsigned char enforthDict[512];



/* -------------------------------------
 * main()
 */

int main(int argc, char **argv)
{
    /* Initialize curses: disable line buffering and local echo, enable
     * line-oriented scrolling, do not block during reads. */
    initscr();
    cbreak();
    noecho();
    idlok(stdscr, TRUE);
    scrollok(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    /* Initialize Enforth. */
    enforth_init(
            &enforthVM,
            enforthDict, sizeof(enforthDict),
            LAST_FFI,
            enforthCursesKeyQuestion, enforthCursesKey, enforthCursesEmit,
            enforthLoad, enforthSave);

    /* Add a couple of definitions. */
    enforth_evaluate(&enforthVM, ": favnum 27 ;");
    enforth_evaluate(&enforthVM, ": 2x dup + ;");

    /* Launch the enforth interpreter. */
    enforth_go(&enforthVM);

    /* Destroy curses. */
    endwin();

    /* Exit the application. */
    return 0;
}
