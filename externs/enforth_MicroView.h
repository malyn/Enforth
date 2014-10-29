/* Copyright (c) 2014, Michael Alyn Miller <malyn@strangeGizmo.com>.
 * All rights reserved.
 * vi:ts=4:sts=4:et:sw=4:sr:et:tw=72:fo=crq
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

#ifndef EXTERNS_ENFORTH_MICROVIEW_H_
#define EXTERNS_ENFORTH_MICROVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------
 * Includes.
 */

#include <enforth.h>
#include <MicroView.h>



/* -------------------------------------
 * FFI Definitions.
 */

ENFORTH_EXTERN_VOID_METHOD(uview_clear, { uView.clear(PAGE); }, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(uview_clear)

ENFORTH_EXTERN_VOID_METHOD(uview_circle, { uView.circle(a.u, b.u, c.u); }, 3)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(uview_circle)

ENFORTH_EXTERN_VOID_METHOD(uview_display, { uView.display(); }, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(uview_display)

ENFORTH_EXTERN_VOID_METHOD(uview_print, { uView.print((char *)a.ram); }, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(uview_print)


#ifdef __cplusplus
}
#endif

#endif /* EXTERNS_ENFORTH_MICROVIEW_H_ */
