/* Copyright (c) 2008-2014, Michael Alyn Miller <malyn@strangeGizmo.com>.
 * All rights reserved.
 * vi:ts=4:sts=4:et:sw=4:sr:et:tw=72:fo=tcrq
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice unmodified, this list of conditions, and the following
 *     disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 3.  Neither the name of Michael Alyn Miller nor the names of the
 *     contributors to this software may be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
