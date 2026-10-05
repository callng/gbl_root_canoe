/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef SUPER_FB_AUTH_H
#define SUPER_FB_AUTH_H
#include <Uefi.h>

/* All privileged entry points start denied. Initialization is once per boot. */
VOID SfbAuthInitialize (VOID);
BOOLEAN SfbAuthIsUnlocked (VOID);
BOOLEAN SfbAuthRequest (VOID);
EFI_STATUS SfbGetPersistVolume (OUT EFI_HANDLE *Volume);
EFI_STATUS SfbLaunchAndroid (IN BOOLEAN ClearScreen);
#endif
