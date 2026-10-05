/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SUPER_FB_PASSWORD_H
#define SUPER_FB_PASSWORD_H
#include <Uefi.h>
#define SFB_PIN_MIN_DIGITS 6
#define SFB_PIN_MAX_DIGITS 64
#define SFB_PASSWORD_RECORD_BYTES 104

/* SFBPW1:<32 lowercase hex salt chars>:<64 lowercase SHA-256 hex chars>[LF].
 * Digest is SHA-256 of the ASCII salt followed by the ASCII PIN digits. */
BOOLEAN SfbPasswordParse (CONST CHAR8 *Record, UINTN Bytes,
                          CHAR8 Salt[32], UINT8 Digest[32]);
BOOLEAN SfbPasswordHash (CONST CHAR8 Salt[32], CONST CHAR8 *Pin, UINTN Length,
                         UINT8 Digest[32]);
#endif
