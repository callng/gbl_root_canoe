/* Generated header contains the actual SfbWaitForKey production function. */
#include "SfbWaitForKeyUnderTest.h"
#include <stdio.h>
static EFI_BOOT_SERVICES Services;
static EFI_SYSTEM_TABLE System;
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL Input;
EFI_BOOT_SERVICES *gBS = &Services;
EFI_SYSTEM_TABLE *gST = &System;
static EFI_STATUS CreateResult, TimerResult;
static UINTN Waits, Closes;
static BOOLEAN KeyPressed;
static EFI_STATUS EFIAPI CreateEvent (UINT32 Type, EFI_TPL Tpl, EFI_EVENT_NOTIFY Notify,
                                     VOID *Context, EFI_EVENT *Event) {
  *Event = (EFI_EVENT)1; return CreateResult;
}
static EFI_STATUS EFIAPI SetTimer (EFI_EVENT Event, EFI_TIMER_DELAY Type, UINT64 Time) { return TimerResult; }
static EFI_STATUS EFIAPI WaitEvent (UINTN Count, EFI_EVENT *Events, UINTN *Index) {
  Waits++;
  if (Count != 2) return EFI_DEVICE_ERROR;
  *Index = KeyPressed ? 0 : 1;
  return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI CloseEvent (EFI_EVENT Event) { Closes++; return EFI_SUCCESS; }
static EFI_STATUS EFIAPI ReadKey (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key) {
  Key->ScanCode = SCAN_UP; Key->UnicodeChar = 0; return EFI_SUCCESS;
}
#define CHECK(X) do { if (!(X)) { fprintf (stderr, "Timer test failed at line %d: %s\n", __LINE__, #X); return 1; } } while (0)
int main (void) {
  Services.CreateEvent = CreateEvent; Services.SetTimer = SetTimer;
  Services.WaitForEvent = WaitEvent; Services.CloseEvent = CloseEvent;
  Input.ReadKeyStroke = ReadKey; Input.WaitForKey = (EFI_EVENT)2; System.ConIn = &Input;
  CreateResult = EFI_OUT_OF_RESOURCES;
  CHECK (SfbWaitForKey (30000) == SfbKeyTimeout && Waits == 0 && Closes == 0);
  CreateResult = EFI_SUCCESS; TimerResult = EFI_DEVICE_ERROR;
  CHECK (SfbWaitForKey (30000) == SfbKeyTimeout && Waits == 0 && Closes == 1);
  TimerResult = EFI_SUCCESS;
  CHECK (SfbWaitForKey (30000) == SfbKeyTimeout && Waits == 1 && Closes == 2);
  KeyPressed = TRUE;
  CHECK (SfbWaitForKey (30000) == SfbKeyUp && Waits == 2 && Closes == 3);
  puts ("Key timeout: event allocation failure, timer setup failure, normal timeout and key input passed");
  return 0;
}
