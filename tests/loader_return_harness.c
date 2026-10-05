/* Exercise the real loader's failure/control flow with firmware mocks. */
#include "../submodules/uefi/edk2/QcomModulePkg/Application/LinuxLoader/LinuxLoader.c"
#include <stdio.h>
#include <stdlib.h>

static EFI_BOOT_SERVICES Services;
static EFI_SYSTEM_TABLE System;
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL Input;
EFI_BOOT_SERVICES *gBS = &Services;
EFI_SYSTEM_TABLE *gST = &System;
UINTN __stack_chk_guard;
static EFI_STATUS InitStatus;
static BOOLEAN MenuKey, Unlocked, Verify, WantFastboot;
static UINTN DefaultCalls, AndroidCalls, AuthCalls, MenuCalls, CleanupCalls, Stalls;
static FASTBOOT_AUTHORIZATION_CHECK Check;

EFI_STATUS InitThreadUnsafeStack (VOID) { return InitStatus; }
VOID DeInitThreadUnsafeStack (VOID) { CleanupCalls++; }
EFI_STATUS EnumeratePartitions (VOID) { return EFI_SUCCESS; }
VOID UpdatePartitionEntries (VOID) {}
EFI_STATUS SfbStartFatStack (VOID) { return EFI_SUCCESS; }
VOID SfbAuthInitialize (VOID) {}
BOOLEAN SfbAuthIsUnlocked (VOID) { return Unlocked; }
BOOLEAN SfbAuthRequest (VOID) { AuthCalls++; if (Verify) Unlocked = TRUE; return Unlocked; }
VOID FastbootSetAuthorizationCheck (FASTBOOT_AUTHORIZATION_CHECK Callback) { Check = Callback; }
BOOLEAN SfbLaunchDefaultEntry (VOID) { DefaultCalls++; return FALSE; }
EFI_STATUS SfbLaunchAndroid (BOOLEAN ClearScreen) { AndroidCalls++; return EFI_LOAD_ERROR; }
VOID SfbShowEnteringMenu (VOID) {}
BOOLEAN SfbRunBootMenu (VOID) { MenuCalls++; return WantFastboot; }
VOID SfbShowFastbootMode (VOID) {}
EFI_STATUS FastbootInitialize (VOID) { return Check && Check () ? EFI_DEVICE_ERROR : EFI_ACCESS_DENIED; }

static EFI_STATUS EFIAPI ResetInput (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, BOOLEAN Extended) { return EFI_SUCCESS; }
static EFI_STATUS EFIAPI ReadKey (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key) {
  Key->ScanCode = SCAN_UP; Key->UnicodeChar = 0; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI CreateEvent (UINT32 Type, EFI_TPL Tpl, EFI_EVENT_NOTIFY Notify,
                                     VOID *Context, EFI_EVENT *Event) {
  *Event = (EFI_EVENT)1; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI SetTimer (EFI_EVENT Event, EFI_TIMER_DELAY Type, UINT64 Time) { return EFI_SUCCESS; }
static EFI_STATUS EFIAPI WaitEvent (UINTN Count, EFI_EVENT *Events, UINTN *Index) {
  *Index = MenuKey ? 0 : 1; return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI CloseEvent (EFI_EVENT Event) { return EFI_SUCCESS; }
static EFI_STATUS EFIAPI Stall (UINTN Microseconds) {
  if (++Stalls > 5) { fputs ("Unexpected failure-path busy loop\n", stderr); exit (1); }
  return EFI_SUCCESS;
}
static void reset (BOOLEAN PasswordAbsent, BOOLEAN Requested, BOOLEAN Accepted) {
  InitStatus = EFI_SUCCESS;
  MenuKey = Requested; Unlocked = PasswordAbsent; Verify = Accepted; WantFastboot = FALSE;
  DefaultCalls = AndroidCalls = AuthCalls = MenuCalls = CleanupCalls = Stalls = 0;
  Input.Reset = ResetInput; Input.ReadKeyStroke = ReadKey; Input.WaitForKey = (EFI_EVENT)2;
  System.ConIn = &Input;
  Services.CreateEvent = CreateEvent; Services.SetTimer = SetTimer;
  Services.WaitForEvent = WaitEvent; Services.CloseEvent = CloseEvent; Services.Stall = Stall;
}
#define CHECK(X) do { if (!(X)) { fprintf (stderr, "Loader test failed at line %d: %s\n", __LINE__, #X); return 1; } } while (0)

int main (void) {
  EFI_STATUS Status;
  reset (TRUE, FALSE, FALSE);
  Status = LinuxLoaderEntry (NULL, &System);
  CHECK (Status == EFI_SUCCESS && DefaultCalls == 1 && MenuCalls == 1 && CleanupCalls == 1);
  CHECK (AndroidCalls == 0); /* Original default failure opens the menu. */
  reset (FALSE, FALSE, TRUE);
  Status = LinuxLoaderEntry (NULL, &System);
  CHECK (Status == EFI_SUCCESS && AndroidCalls == 1 && AuthCalls == 1 && MenuCalls == 1);
  reset (FALSE, FALSE, FALSE);
  Status = LinuxLoaderEntry (NULL, &System);
  CHECK (Status == EFI_LOAD_ERROR && AndroidCalls == 1 && MenuCalls == 0 && CleanupCalls == 1);
  reset (FALSE, TRUE, FALSE); /* Timeout, cancellation or exhausted verification. */
  Status = LinuxLoaderEntry (NULL, &System);
  CHECK (Status == EFI_LOAD_ERROR && AndroidCalls == 1 && MenuCalls == 0 && CleanupCalls == 1);
  reset (FALSE, TRUE, TRUE);
  WantFastboot = TRUE;
  Status = LinuxLoaderEntry (NULL, &System);
  CHECK (Status == EFI_DEVICE_ERROR && AndroidCalls == 0 && MenuCalls == 1 && CleanupCalls == 1);
  reset (FALSE, FALSE, FALSE);
  InitStatus = EFI_OUT_OF_RESOURCES;
  Status = LinuxLoaderEntry (NULL, &System);
  CHECK (Status == EFI_OUT_OF_RESOURCES && CleanupCalls == 1 && Stalls == 0 && MenuCalls == 0);
  puts ("Loader: original default-to-menu flow, authenticated failure recovery, denied access, error cleanup and no added busy loops passed");
  return 0;
}
