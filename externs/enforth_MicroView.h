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
