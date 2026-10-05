/* Bounded SHA-256 for the salted numeric password. No crypto firmware
 * protocol or external OpenSSL checkout is required.
 * SPDX-License-Identifier: BSD-3-Clause */
#include "SuperFbPassword.h"

STATIC INTN
SfbHex (CHAR8 Ch)
{
  if (Ch >= '0' && Ch <= '9') return Ch - '0';
  if (Ch >= 'a' && Ch <= 'f') return Ch - 'a' + 10;
  return -1;
}

BOOLEAN
SfbPasswordParse (CONST CHAR8 *Record, UINTN Bytes,
                  CHAR8 Salt[32], UINT8 Digest[32])
{
  CONST CHAR8 Tag[] = "SFBPW1:";
  UINTN Index;
  if (Record == NULL ||
      (Bytes != SFB_PASSWORD_RECORD_BYTES &&
       !(Bytes == SFB_PASSWORD_RECORD_BYTES + 1 &&
         Record[SFB_PASSWORD_RECORD_BYTES] == '\n'))) return FALSE;
  for (Index = 0; Index < sizeof (Tag) - 1; Index++)
    if (Record[Index] != Tag[Index]) return FALSE;
  if (Record[39] != ':') return FALSE;
  for (Index = 0; Index < 32; Index++) {
    if (SfbHex (Record[7 + Index]) < 0) return FALSE;
  }
  for (Index = 0; Index < 64; Index++) {
    if (SfbHex (Record[40 + Index]) < 0) return FALSE;
  }
  for (Index = 0; Index < 32; Index++) {
    Salt[Index] = Record[7 + Index];
    Digest[Index] = (UINT8)((SfbHex (Record[40 + 2 * Index]) << 4) |
                            SfbHex (Record[41 + 2 * Index]));
  }
  return TRUE;
}

STATIC UINT32
SfbRor (UINT32 Value, UINTN Bits)
{
  return (Value >> Bits) | (Value << (32 - Bits));
}

BOOLEAN
SfbPasswordHash (CONST CHAR8 Salt[32], CONST CHAR8 *Pin, UINTN Length,
                 UINT8 Digest[32])
{
  STATIC CONST UINT32 K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
  };
  UINT8 Message[128] = {0};
  UINT32 W[64];
  UINT32 H[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                 0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  UINT32 A,B,C,D,E,F,G,J,T1,T2;
  UINTN Index;
  UINTN BlockIndex;
  UINTN Blocks;
  UINT64 Bits;
  if (Salt == NULL || Pin == NULL || Digest == NULL ||
      Length < SFB_PIN_MIN_DIGITS || Length > SFB_PIN_MAX_DIGITS) return FALSE;
  for (Index = 0; Index < Length; Index++)
    if (Pin[Index] < '0' || Pin[Index] > '9') return FALSE;
  for (Index = 0; Index < 32; Index++) Message[Index] = (UINT8)Salt[Index];
  for (Index = 0; Index < Length; Index++) Message[32 + Index] = (UINT8)Pin[Index];
  Message[32 + Length] = 0x80;
  Blocks = (32 + Length + 9 + 63) / 64;
  Bits = (32 + Length) * 8;
  for (Index = 0; Index < 8; Index++)
    Message[Blocks * 64 - 1 - Index] = (UINT8)(Bits >> (Index * 8));
  for (BlockIndex = 0; BlockIndex < Blocks; BlockIndex++) {
    for (Index = 0; Index < 16; Index++) {
      UINTN Offset = BlockIndex * 64 + 4 * Index;
      W[Index] = ((UINT32)Message[Offset] << 24) | ((UINT32)Message[Offset + 1] << 16) |
                 ((UINT32)Message[Offset + 2] << 8) | Message[Offset + 3];
    }
    for (Index = 16; Index < 64; Index++) {
      UINT32 X = W[Index - 15], Y = W[Index - 2];
      W[Index] = W[Index - 16] + (SfbRor (X,7) ^ SfbRor (X,18) ^ (X >> 3)) +
                 W[Index - 7] + (SfbRor (Y,17) ^ SfbRor (Y,19) ^ (Y >> 10));
    }
    A=H[0]; B=H[1]; C=H[2]; D=H[3]; E=H[4]; F=H[5]; G=H[6]; J=H[7];
    for (Index = 0; Index < 64; Index++) {
      T1 = J + (SfbRor (E,6) ^ SfbRor (E,11) ^ SfbRor (E,25)) +
           ((E & F) ^ (~E & G)) + K[Index] + W[Index];
      T2 = (SfbRor (A,2) ^ SfbRor (A,13) ^ SfbRor (A,22)) +
           ((A & B) ^ (A & C) ^ (B & C));
      J=G; G=F; F=E; E=D+T1; D=C; C=B; B=A; A=T1+T2;
    }
    H[0]+=A; H[1]+=B; H[2]+=C; H[3]+=D; H[4]+=E; H[5]+=F; H[6]+=G; H[7]+=J;
  }
  for (Index = 0; Index < 32; Index++)
    Digest[Index] = (UINT8)(H[Index / 4] >> (24 - 8 * (Index % 4)));
  /* Volatile stores ensure PIN-derived intermediates are actually erased. */
  for (Index = 0; Index < sizeof (Message); Index++) ((volatile UINT8 *)Message)[Index] = 0;
  for (Index = 0; Index < 64; Index++) ((volatile UINT32 *)W)[Index] = 0;
  return TRUE;
}
