/* Exercise the merged launcher with firmware calls mocked, including the
 * fixed Android fallback and the upstream image-verification hook. */
#include "../submodules/uefi/edk2/QcomModulePkg/Application/LinuxLoader/SuperFbEntries.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EFI_GUID gEfiSecurityArchProtocolGuid, gEfiSecurity2ArchProtocolGuid;
EFI_GUID gEfiLoadedImageProtocolGuid;
EFI_HANDLE gImageHandle;
static EFI_BOOT_SERVICES Services;
EFI_BOOT_SERVICES *gBS = &Services;
static EFI_SECURITY2_ARCH_PROTOCOL Security;
static EFI_FILE_PROTOCOL Root;
static BOOLEAN Unlocked;
static EFI_STATUS LoadStatus, VerifyStatus, PersistStatus;
static UINTN Loads, Starts, DriverLists, Verifications, Frees;
static CHAR16 LoadedPath[SFB_PATH_CHARS];
static EFI_LOADED_IMAGE_PROTOCOL Loaded;

VOID *EFIAPI ZeroMem (VOID *Out, UINTN Bytes) { return memset (Out, 0, Bytes); }
VOID *EFIAPI AllocateZeroPool (UINTN Bytes) { return calloc (1, Bytes); }
VOID EFIAPI FreePool (VOID *Buffer) { Frees++; free (Buffer); }
UINTN EFIAPI __StrLen (CONST CHAR16 *Text) { UINTN Length = 0; while (Text[Length]) Length++; return Length; }
RETURN_STATUS EFIAPI __StrnCpyS (CHAR16 *Out, UINTN Size, CONST CHAR16 *Text, UINTN Count) {
  UINTN At = 0;
  while (At < Count && At + 1 < Size && Text[At]) { Out[At] = Text[At]; At++; }
  Out[At] = 0; return RETURN_SUCCESS;
}
RETURN_STATUS EFIAPI __StrnCatS (CHAR16 *Out, UINTN Size, CONST CHAR16 *Text, UINTN Count) {
  UINTN At = __StrLen (Out); return __StrnCpyS (Out + At, Size - At, Text, Count);
}
BOOLEAN SfbAuthIsUnlocked (VOID) { return Unlocked; }
EFI_STATUS SfbGetPersistVolume (EFI_HANDLE *Volume) { *Volume = (EFI_HANDLE)1; return PersistStatus; }
EFI_STATUS SfbGetVolumeAddress (EFI_HANDLE Volume, UINT32 *Disk, UINT32 *Part) { *Disk = 0; *Part = 2; return EFI_SUCCESS; }
EFI_HANDLE SfbFindVolumeByAddress (UINT32 Disk, UINT32 Part) { return (EFI_HANDLE)1; }
EFI_DEVICE_PATH_PROTOCOL *EFIAPI FileDevicePath (EFI_HANDLE Volume, CONST CHAR16 *Path) {
  StrnCpyS (LoadedPath, SFB_PATH_CHARS, Path, SFB_PATH_CHARS - 1);
  return calloc (1, sizeof (EFI_DEVICE_PATH_PROTOCOL));
}
VOID SfbShowBootingScreen (CONST CHAR16 *Name, BOOLEAN ClearScreen) {}
EFI_STATUS SfbSecurityVerifyImage (CONST VOID *Image, UINTN Size) { Verifications++; return VerifyStatus; }
EFI_STATUS SfbOpenVolumeRoot (EFI_HANDLE Volume, EFI_FILE_PROTOCOL **Out) { *Out = &Root; return EFI_SUCCESS; }
EFI_STATUS SfbReadFileBytes (EFI_FILE_PROTOCOL *File, CONST CHAR16 *Path, VOID *Buffer, UINTN Max, UINTN *Bytes) {
  DriverLists++; *Bytes = 0; return EFI_NOT_FOUND;
}
VOID SfbConnectAll (VOID) {}
BOOLEAN SfbEntryNextLine (CONST CHAR8 **Cursor, CHAR8 *Line, UINTN Bytes) { return FALSE; }
static EFI_STATUS EFIAPI CloseRoot (EFI_FILE_PROTOCOL *File) { return EFI_SUCCESS; }
static EFI_STATUS EFIAPI Locate (EFI_GUID *Guid, VOID *Registration, VOID **Out) {
  if (Guid == &gEfiSecurity2ArchProtocolGuid) { *Out = &Security; return EFI_SUCCESS; }
  return EFI_NOT_FOUND;
}
static EFI_STATUS EFIAPI Load (BOOLEAN Policy, EFI_HANDLE Parent, EFI_DEVICE_PATH_PROTOCOL *Path,
                              VOID *Source, UINTN Size, EFI_HANDLE *Image) {
  EFI_STATUS Status;
  Loads++;
  Status = Security.FileAuthentication (&Security, Path, (VOID *)"PE", 2, FALSE);
  if (EFI_ERROR (Status)) return Status;
  *Image = (EFI_HANDLE)3; return LoadStatus;
}
static EFI_STATUS EFIAPI Start (EFI_HANDLE Image, UINTN *Size, CHAR16 **Data) { Starts++; return EFI_LOAD_ERROR; }
static EFI_STATUS EFIAPI Open (EFI_HANDLE Handle, EFI_GUID *Guid, VOID **Out, EFI_HANDLE Agent,
                              EFI_HANDLE Controller, UINT32 Attributes) { *Out = &Loaded; return EFI_SUCCESS; }
static void reset (BOOLEAN Accepted) {
  Unlocked = Accepted; LoadStatus = VerifyStatus = PersistStatus = EFI_SUCCESS;
  Loads = Starts = DriverLists = Verifications = Frees = 0;
  memset (&Loaded, 0, sizeof (Loaded));
  Services.LocateProtocol = Locate; Services.LoadImage = Load;
  Services.StartImage = Start; Services.OpenProtocol = Open; Root.Close = CloseRoot;
}
#define CHECK(X) do { if (!(X)) { fprintf (stderr, "Entry launch test failed at line %d: %s\n", __LINE__, #X); return 1; } } while (0)
int main (void) {
  SFB_BOOT_ENTRY Entry;
  BOOLEAN NotPe = TRUE;
  EFI_STATUS Status;
  reset (FALSE);
  memset (&Entry, 0, sizeof (Entry));
  CHECK (SfbLaunchEntry (&Entry, TRUE, &NotPe) == EFI_ACCESS_DENIED && !NotPe && Loads == 0 && DriverLists == 0);
  CHECK (SfbLoadDriver ((EFI_HANDLE)1, L"\\driver.efi") == EFI_ACCESS_DENIED && Loads == 0);
  Status = SfbLaunchAndroid (FALSE);
  CHECK (Status == EFI_LOAD_ERROR && Loads == 1 && Starts == 1 && DriverLists == 0 && Verifications == 1 && Frees == 1);
  CHECK (memcmp (LoadedPath, L"\\efisp\\boot.efi", sizeof (L"\\efisp\\boot.efi")) == 0 && Loaded.LoadOptions == NULL);
  reset (FALSE);
  VerifyStatus = EFI_SECURITY_VIOLATION;
  CHECK (SfbLaunchAndroid (FALSE) == EFI_SECURITY_VIOLATION && Starts == 0 && Verifications == 1 && Frees == 1);
  reset (FALSE);
  VerifyStatus = EFI_UNSUPPORTED;
  CHECK (SfbLaunchAndroid (FALSE) == EFI_ACCESS_DENIED && Starts == 0);
  reset (FALSE);
  PersistStatus = EFI_ACCESS_DENIED;
  CHECK (SfbLaunchAndroid (FALSE) == EFI_ACCESS_DENIED && Loads == 0);
  reset (TRUE);
  CHECK (SfbMakeFileEntry ((EFI_HANDLE)1, L"\\other.efi", L"Other", &Entry) == EFI_SUCCESS);
  StrnCpyS (Entry.Spec.Args, SFB_ARGS_CHARS, L"--test", 6);
  CHECK (SfbLaunchEntry (&Entry, TRUE, &NotPe) == EFI_LOAD_ERROR && !NotPe && DriverLists == 1 && Starts == 1);
  CHECK (Loaded.LoadOptions == Entry.Spec.Args && Loaded.LoadOptionsSize == sizeof (L"--test"));
  LoadStatus = EFI_UNSUPPORTED;
  CHECK (SfbLaunchEntry (&Entry, TRUE, &NotPe) == EFI_UNSUPPORTED && NotPe);
  FreePool (Entry.DevicePath);
  puts ("Entry launch: denied EFI/drivers, fixed Android without args/drivers, mandatory certificate verification and upstream arguments/non-PE reporting passed");
  return 0;
}
