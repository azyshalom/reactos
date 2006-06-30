/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/efi.c
 * PURPOSE:         I/O Functions for EFI Machines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NtAddBootEntry(IN PUNICODE_STRING EntryName,
               IN PUNICODE_STRING EntryValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtDeleteBootEntry(IN PUNICODE_STRING EntryName,
                  IN PUNICODE_STRING EntryValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtEnumerateBootEntries(IN PVOID Buffer,
                       IN PULONG BufferLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryBootEntryOrder(IN PULONG Ids,
                      IN PULONG Count)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryBootOptions(IN PBOOT_OPTIONS BootOptions,
                   IN PULONG BootOptionsLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetBootEntryOrder(IN PULONG Ids,
                    IN PULONG Count)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetBootOptions(IN PBOOT_OPTIONS BootOptions,
                 IN ULONG FieldsToChange)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtTranslateFilePath(PFILE_PATH InputFilePath,
                    ULONG OutputType,
                    PFILE_PATH OutputFilePath,
                    ULONG OutputFilePathLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
