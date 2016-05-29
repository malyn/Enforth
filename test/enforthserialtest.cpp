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
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

/* Test includes. */
typedef int EnforthVM;
#include "enforthtesthelper.h"

/* Catch includes. */
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"



/* -------------------------------------
 * Harness-provided functions.
 */

extern "C" EnforthVM * const get_test_vm();
extern "C" void enforth_evaluate(EnforthVM * const vm, const char * const text);
extern "C" bool enforth_test(EnforthVM * const vm, const char * const text);



/* -------------------------------------
 * Test harness globals.
 */

static EnforthVM gSerialPort = -1;



/* -------------------------------------
 * Helper functions.
 */

/* Enforth responds with "ok " when valid (including empty) input has
 * been processed.  Error responses are terminated with a "?" and
 * linefeed.  After every enforth_evaluate we need to read until we see
 * either "ok " or "?\n".  Then we can return to the caller.  Everything
 * else should be printed out.
 *
 * Startup should involve sending LFs until an "ok " is seen. */
static int OpenEnforthConnection(const char * const path)
{
	/* Open the serial port, reset the target, then close the serial
	 * port. */
	printf("*** Resetting Target ***\n");

	int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1)
	{
		perror("open failed");
		exit(1);
	}

	int flags;
	ioctl(fd, TIOCMGET, &flags);

	flags |= TIOCM_DTR; /* Set DTR low */
	ioctl(fd, TIOCMSET, &flags);

	flags &= ~TIOCM_DTR; /* Set DTR high */
	ioctl(fd, TIOCMSET, &flags);

	close(fd);

	/* Wait one second and then reopen the serial port, this time with a
	 * specific set of flags, baud rate, etc.. */
	sleep(1);

	printf("*** Opening Serial Port ***\n");
	fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1)
	{
		perror("open failed");
		exit(1);
	}

	struct termios tios;
	memset(&tios, 0, sizeof(tios));

	tios.c_iflag = 0;

	tios.c_oflag = 0;

	tios.c_cflag = CS8 | CREAD | CLOCAL;

	tios.c_lflag = 0;

	/* We're operating in asynchronous mode, so we never want to block
	 * in read. */
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;

	cfsetispeed(&tios, B9600);
	cfsetospeed(&tios, B9600);

	/* Change attributes after sending all data (there is none) and
	 * flushing any input. */
	tcsetattr(fd, TCSAFLUSH, &tios);

    /* Set DTR high in order to take us out of reset.  This is not
     * required on a USB-connected Arduino, but may be required on
     * devices connected via FTDI chips or when the serial driver is
     * automatically de-asserting DTR after open. */
	ioctl(fd, TIOCMGET, &flags);
	flags &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);

	/* Drain the port watching for an "ok ", sending LFs every second. */
	printf("*** Sending LFs; waiting for \"ok \" ***\n");
	enum { waitingForO, waitingForK, waitingForSpace } okState = waitingForO;
	time_t lastLfSec = 0;
	for (;;)
	{
		/* Configure the select sets; we only care about writability if
		 * it has been more than one second since our last LF. */
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);

		fd_set wset;
		FD_ZERO(&wset);

		struct timeval tm;
		gettimeofday(&tm, NULL);
		if (tm.tv_sec != lastLfSec)
		{
			FD_SET(fd, &wset);
		}

		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int count = select(fd + 1, &rset, &wset, NULL, &tv);
		if (count == -1)
		{
			perror("select failed");
			exit(1);
		}
		else if (count == 0)
		{
			/* This means that we didn't get any data since the last LF.
			 * Check to see if the overall connection interval has taken
			 * too long and abort if so.  Otherwise just go back to the
			 * top of the loop (at which point we will check for
			 * writability and then send another LF). */
			//printf("... Timeout ...\n");

			/* TODO Check for an overall timeout */
			continue;
		}

		/* Read any characters that are available and run them through
		 * our "ok " state machine. */
		if (FD_ISSET(fd, &rset))
		{
			for (;;)
			{
				char ch;
				int rc = read(fd, &ch, 1);
				//printf("... Read returned %d; c='%c' (%d) ...\n",
				//		rc,
				//		ch != 0 ? ch : ' ',
				//		(int)ch);

				if (rc == -1)
				{
					if (errno == EAGAIN)
					{
						break;
					}
					else
					{
						perror("read failed");
						exit(1);
					}
				}
				else if (rc == 0)
				{
					/* No more bytes available right now. */
					break;
				}

				switch (okState)
				{
					case waitingForO:
						if (ch == 'o')
						{
							okState = waitingForK;
						}
						break;

					case waitingForK:
						if (ch == 'k')
						{
							okState = waitingForSpace;
						}
						else
						{
							okState = waitingForO;
						}
						break;

					case waitingForSpace:
						if (ch == ' ')
						{
							/* We're done. */
							// printf("... Saw \"ok \"; we're done ... \n");
							return fd;
						}
						else
						{
							okState = waitingForO;
						}
						break;
				}
			}
		}

		/* Send another LF if the port is writable (we already know that
		 * at least one second has past since the last LF since we would
		 * not have checked for writability otherwise). */
		if (FD_ISSET(fd, &wset))
		{
			printf("... Sending another LF ... \n");
			lastLfSec = tm.tv_sec;

			char lf = '\n';
			int sent = write(fd, &lf, 1);
			if (sent == -1)
			{
				perror("send failed");
				exit(1);
			}
		}
	}

	/* Return the serial port. */
	return fd;
}

/* Reads from the serial connection until the "ok " prompt is found. */
static void ReadToOkPrompt(int fd)
{
	/* Drain the port watching for an "ok ". */
	enum { waitingForO, waitingForK, waitingForSpace } okState = waitingForO;
	for (;;)
	{
		/* Configure the select sets. */
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		int count = select(fd + 1, &rset, NULL, NULL, &tv);
		if (count == -1)
		{
			perror("select failed");
			exit(1);
		}
		else if (count == 0)
		{
			/* Timeout; something went wrong in the test. */
			printf("!!! Timeout !!!\n");
			exit(1);
		}

		/* Read any characters that are available and run them through
		 * our "ok " state machine. */
		if (FD_ISSET(fd, &rset))
		{
			for (;;)
			{
				char ch;
				int rc = read(fd, &ch, 1);
				//printf("... Read returned %d; c='%c' (%d) ...\n",
				//		rc,
				//		ch != 0 ? ch : ' ',
				//		(int)ch);

				if (rc == -1)
				{
					if (errno == EAGAIN)
					{
						break;
					}
					else
					{
						perror("read failed");
						exit(1);
					}
				}
				else if (rc == 0)
				{
					/* No more bytes available right now. */
					break;
				}

				putchar(ch);

				switch (okState)
				{
					case waitingForO:
						if (ch == 'o')
						{
							okState = waitingForK;
						}
						break;

					case waitingForK:
						if (ch == 'k')
						{
							okState = waitingForSpace;
						}
						else
						{
							okState = waitingForO;
						}
						break;

					case waitingForSpace:
						if (ch == ' ')
						{
							/* We're done. */
							// printf("... Saw \"ok \"; we're done ... \n");
							return;
						}
						else
						{
							okState = waitingForO;
						}
						break;
				}
			}
		}
	}
}

/* Sends text to Enforth and then reads everything until <CR><LF>.  Note
 * that a leading "ok " may still be sticking around from the prevous
 * command. */
static const char * const SendEnforthLineAndWaitForResponse(
		int fd,
		const char * const text)
{
	/* Initialize our receive buffer. */
	static char buf[1024];
	char * pBuf = buf, * pBufEnd = pBuf + sizeof(buf) - 1;

	/* Initialize our send pointer. */
	const char * pSend = text, * pSendEnd = text + strlen(text);

	/* Send our output text, draining the port and watching for
	 * "<CR><LF>". */
	enum { waitingForCR, waitingForLF } readState = waitingForCR;

	for (;;)
	{
		/* Configure the select sets; we only care about writability if
		 * we have more data to send. */
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);

		fd_set wset;
		FD_ZERO(&wset);

		if (pSend != pSendEnd)
		{
			FD_SET(fd, &wset);
		}

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		int count = select(fd + 1, &rset, &wset, NULL, &tv);
		if (count == -1)
		{
			perror("select failed");
			exit(1);
		}
		else if (count == 0)
		{
			/* Timeout; something went wrong in the test. */
			printf("!!! Timeout !!!\n");
			return NULL;
		}

		/* Read any characters that are available and run them through
		 * our read state machine. */
		if (FD_ISSET(fd, &rset))
		{
			for (;;)
			{
				char ch;
				int rc = read(fd, &ch, 1);
#if 0
				printf("... Read returned %d; c='%c' (%d) ...\n",
						rc,
						ch != 0 ? ch : ' ',
						(int)ch);
#endif

				if (rc == -1)
				{
					if (errno == EAGAIN)
					{
						break;
					}
					else
					{
						perror("read failed");
						exit(1);
					}
				}
				else if (rc == 0)
				{
					/* No more bytes available right now. */
					break;
				}

				/* Add the character to our buffer; aborting if we would
				 * exhaust the buffer. */
				if (pBuf == pBufEnd)
				{
					printf("!!! Read buffer exhausted !!!\n");
				}

				*pBuf++ = ch;

				switch (readState)
				{
					case waitingForCR:
						if (ch == '\r')
						{
							readState = waitingForLF;
						}
						else
						{
							readState = waitingForCR;
						}
						break;

					case waitingForLF:
						if (ch == '\n')
						{
							/* We're done. */
							// printf("... Saw \"<CR><LF>\"; we're done ... \n");
							*pBuf = '\0';
							return buf;
						}
						else
						{
							readState = waitingForCR;
						}
						break;
				}
			}
		}

		/* Send more text if the port is writable (we only check for
		 * writability if we have text to send, so we don't check for
		 * that here). */
		if (FD_ISSET(fd, &wset))
		{
			int sent = write(fd, pSend, pSendEnd - pSend);
			if (sent == -1)
			{
				perror("send failed");
				exit(1);
			}

			pSend += sent;
		}
	}
}



/* -------------------------------------
 * Test harness functions.
 */

EnforthVM * const get_test_vm()
{
	/* Close the existing serial port if necessary. */
	if (gSerialPort != -1)
	{
		close(gSerialPort);
		gSerialPort = -1;
	}

	/* Open a new connection to the target. */
	gSerialPort = OpenEnforthConnection(getenv("ENFORTH_PORT"));

    /* Compile the tester words. */
    compile_tester(&gSerialPort);

	/* Return "VM" -- our serial port. */
	return &gSerialPort;
}

void enforth_evaluate(EnforthVM * const vm, const char * const text)
{
	//printf(">>> %s\n", text);

	/* Append an LF to the text. */
	char buf[1024];
	memcpy(buf, text, strlen(text));
	buf[strlen(text)] = '\n';
	buf[strlen(text) + 1] = '\0';

	/* Send the linefeed-terminated buffer to the serial port. */
	const char * response = SendEnforthLineAndWaitForResponse(*vm, buf);
	if (response == NULL)
	{
		printf("!!! Evaluation timed out !!!\n");
		exit(1);
	}
	else
	{
		printf("<<< %s", response);
	}
}

bool enforth_test(EnforthVM * const vm, const char * const text)
{
    /* Run the test. */
    enforth_evaluate(vm, text);

	/* Read to the "ok " prompt. */
	ReadToOkPrompt(*vm);

	/* Print out, read and return the flag. */
	/* 1+ because we are probably in HEX mode and so -1 will show up as
	 * FFFF, whereas 0 always ends in 0. */
	static char getresult[] = "1+ .\n";
	const char * response = SendEnforthLineAndWaitForResponse(*vm, getresult);
	if (response == NULL)
	{
		printf("!!! Evaluation timed out !!!\n");
		exit(1);
	}

	/* Response should be "1 + . 0|1 <CR><LF>" */
	if (strlen(response) != 9)
	{
		printf("!!! Unexpected response: '%s' (len=%d) !!!\n",
				response, strlen(response));
		exit(1);
	}

	return response[5] == '0';
}



/* -------------------------------------
 * main()
 */

int main(int argc, char * const argv[])
{
	// Run the tests.
	int result = Catch::Session().run(argc, argv);

	// Close the serial port.
	if (gSerialPort != -1)
	{
		close(gSerialPort);
		gSerialPort = -1;
	}

	// We're done.
	return result;
}
