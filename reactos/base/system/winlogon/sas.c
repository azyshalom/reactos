/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/sas.c
 * PURPOSE:         Secure Attention Sequence
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Herv� Poussineau (hpoussin@reactos.org)
 * UPDATE HISTORY:
 *                  Created 28/03/2004
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

/* GLOBALS ******************************************************************/

#define WINLOGON_SAS_CLASS L"SAS Window class"
#define WINLOGON_SAS_TITLE L"SAS window"

#define HK_CTRL_ALT_DEL   0
#define HK_CTRL_SHIFT_ESC 1

#define EWX_ACTION_MASK 0xffffffeb
#define EWX_FLAGS_MASK  0x00000014

typedef struct tagLOGOFF_SHUTDOWN_DATA
{
    UINT Flags;
    PWLSESSION Session;
} LOGOFF_SHUTDOWN_DATA, *PLOGOFF_SHUTDOWN_DATA;

/* FUNCTIONS ****************************************************************/

static BOOL
StartTaskManager(
    IN OUT PWLSESSION Session)
{
    LPVOID lpEnvironment;
    BOOL ret;

    if (!Session->Gina.Functions.WlxStartApplication)
        return FALSE;

    if (!CreateEnvironmentBlock(
        &lpEnvironment,
        Session->UserToken,
        TRUE))
    {
        return FALSE;
    }

    ret = Session->Gina.Functions.WlxStartApplication(
        Session->Gina.Context,
        L"Default",
        lpEnvironment,
        L"taskmgr.exe");

    DestroyEnvironmentBlock(lpEnvironment);
    return ret;
}

static BOOL
StartUserShell(
    IN OUT PWLSESSION Session)
{
    LPVOID lpEnvironment = NULL;
    BOOLEAN Old;
    BOOL ret;

    /* Create environment block for the user */
    if (!CreateEnvironmentBlock(&lpEnvironment, Session->UserToken, TRUE))
    {
        WARN("WL: CreateEnvironmentBlock() failed\n");
        return FALSE;
    }

    /* Get privilege */
    /* FIXME: who should do it? winlogon or gina? */
    /* FIXME: reverting to lower privileges after creating user shell? */
    RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, FALSE, &Old);

    ret = Session->Gina.Functions.WlxActivateUserShell(
                Session->Gina.Context,
                L"Default",
                NULL, /* FIXME */
                lpEnvironment);

    DestroyEnvironmentBlock(lpEnvironment);
    return ret;
}


BOOL
SetDefaultLanguage(
    IN BOOL UserProfile)
{
    HKEY BaseKey;
    LPCWSTR SubKey;
    LPCWSTR ValueName;
    LONG rc;
    HKEY hKey = NULL;
    DWORD dwType, dwSize;
    LPWSTR Value = NULL;
    UNICODE_STRING ValueString;
    NTSTATUS Status;
    LCID Lcid;
    BOOL ret = FALSE;

    if (UserProfile)
    {
        BaseKey = HKEY_CURRENT_USER;
        SubKey = L"Control Panel\\International";
        ValueName = L"Locale";
    }
    else
    {
        BaseKey = HKEY_LOCAL_MACHINE;
        SubKey = L"System\\CurrentControlSet\\Control\\Nls\\Language";
        ValueName = L"Default";
    }

    rc = RegOpenKeyExW(
        BaseKey,
        SubKey,
        0,
        KEY_READ,
        &hKey);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx() failed with error %lu\n", rc);
        goto cleanup;
    }
    rc = RegQueryValueExW(
        hKey,
        ValueName,
        NULL,
        &dwType,
        NULL,
        &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegQueryValueEx() failed with error %lu\n", rc);
        goto cleanup;
    }
    else if (dwType != REG_SZ)
    {
        TRACE("Wrong type for %S\\%S registry entry (got 0x%lx, expected 0x%x)\n",
            SubKey, ValueName, dwType, REG_SZ);
        goto cleanup;
    }

    Value = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!Value)
    {
        TRACE("HeapAlloc() failed\n");
        goto cleanup;
    }
    rc = RegQueryValueExW(
        hKey,
        ValueName,
        NULL,
        NULL,
        (LPBYTE)Value,
        &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegQueryValueEx() failed with error %lu\n", rc);
        goto cleanup;
    }

    /* Convert Value to a Lcid */
    ValueString.Length = ValueString.MaximumLength = (USHORT)dwSize;
    ValueString.Buffer = Value;
    Status = RtlUnicodeStringToInteger(&ValueString, 16, (PULONG)&Lcid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("RtlUnicodeStringToInteger() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    TRACE("%s language is 0x%08lx\n",
        UserProfile ? "User" : "System", Lcid);
    Status = NtSetDefaultLocale(UserProfile, Lcid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtSetDefaultLocale() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    ret = TRUE;

cleanup:
    if (hKey)
        RegCloseKey(hKey);
    if (Value)
        HeapFree(GetProcessHeap(), 0, Value);
    return ret;
}

BOOL
PlaySoundRoutine(
    IN LPCWSTR FileName,
    IN UINT bLogon,
    IN UINT Flags)
{
    typedef BOOL (WINAPI *PLAYSOUNDW)(LPCWSTR,HMODULE,DWORD);
    typedef UINT (WINAPI *WAVEOUTGETNUMDEVS)(VOID);
    PLAYSOUNDW Play;
    WAVEOUTGETNUMDEVS waveOutGetNumDevs;
    UINT NumDevs;
    HMODULE hLibrary;
    BOOL Ret = FALSE;

    hLibrary = LoadLibraryW(L"winmm.dll");
    if (hLibrary)
    {
        waveOutGetNumDevs = (WAVEOUTGETNUMDEVS)GetProcAddress(hLibrary, "waveOutGetNumDevs");
        if (waveOutGetNumDevs)
        {
            NumDevs = waveOutGetNumDevs();
            if (!NumDevs)
            {
                if (!bLogon)
                {
                    Beep(500, 500);
                }
                FreeLibrary(hLibrary);
                return FALSE;
            }
        }

        Play = (PLAYSOUNDW)GetProcAddress(hLibrary, "PlaySoundW");
        if (Play)
        {
            Ret = Play(FileName, NULL, Flags);
        }
        FreeLibrary(hLibrary);
    }

    return Ret;
}

DWORD
WINAPI
PlayLogonSoundThread(
    IN LPVOID lpParameter)
{
    BYTE TokenUserBuffer[256];
    PTOKEN_USER pTokenUser = (TOKEN_USER*)TokenUserBuffer;
    ULONG Length;
    HKEY hKey;
    WCHAR wszBuffer[MAX_PATH] = {0};
    WCHAR wszDest[MAX_PATH];
    DWORD dwSize = sizeof(wszBuffer), dwType;
    SERVICE_STATUS_PROCESS Info;
    UNICODE_STRING SidString;
    NTSTATUS Status;
    ULONG Index = 0;
    SC_HANDLE hSCManager, hService;

    /* Get SID of current user */
    Status = NtQueryInformationToken((HANDLE)lpParameter,
                                     TokenUser,
                                     TokenUserBuffer,
                                     sizeof(TokenUserBuffer),
                                     &Length);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQueryInformationToken failed: %x!\n", Status);
        return 0;
    }

    /* Convert SID to string */
    RtlInitEmptyUnicodeString(&SidString, wszBuffer, sizeof(wszBuffer));
    Status = RtlConvertSidToUnicodeString(&SidString, pTokenUser->User.Sid, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlConvertSidToUnicodeString failed: %x!\n", Status);
        return 0;
    }

    /* Build path to logon sound registry key.
       Note: We can't use HKCU here, because Winlogon is owned by SYSTEM user */
    if (FAILED(StringCbCopyW(wszBuffer + SidString.Length/sizeof(WCHAR),
                             sizeof(wszBuffer) - SidString.Length,
                             L"\\AppEvents\\Schemes\\Apps\\.Default\\WindowsLogon\\.Current")))
    {
        /* SID is too long. Should not happen. */
        ERR("StringCbCopyW failed!\n");
        return 0;
    }

    /* Open registry key and query sound path */
    if (RegOpenKeyExW(HKEY_USERS, wszBuffer, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW(%ls) failed!\n", wszBuffer);
        return 0;
    }

    if (RegQueryValueExW(hKey, NULL, NULL, &dwType,
                      (LPBYTE)wszBuffer, &dwSize) != ERROR_SUCCESS ||
        (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        ERR("RegQueryValueExW failed!\n");
        RegCloseKey(hKey);
        return 0;
    }

    RegCloseKey(hKey);

    if (!wszBuffer[0])
    {
        /* No sound has been set */
        ERR("No sound has been set\n");
        return 0;
    }

    /* Expand environment variables */
    if (!ExpandEnvironmentStringsW(wszBuffer, wszDest, MAX_PATH))
    {
        ERR("ExpandEnvironmentStringsW failed!\n");
        return 0;
    }

    /* Open service manager */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        ERR("OpenSCManager failed (%x)\n", GetLastError());
        return 0;
    }

    /* Open wdmaud service */
    hService = OpenServiceW(hSCManager, L"wdmaud", GENERIC_READ);
    if (!hService)
    {
        /* Sound is not installed */
        TRACE("Failed to open wdmaud service (%x)\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return 0;
    }

    /* Wait for wdmaud start */
    do
    {
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(SERVICE_STATUS_PROCESS), &dwSize))
        {
            TRACE("QueryServiceStatusEx failed (%x)\n", GetLastError());
            break;
        }

        if (Info.dwCurrentState == SERVICE_RUNNING)
            break;

        Sleep(1000);

    } while (Index++ < 20);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    /* If wdmaud is not running exit */
    if (Info.dwCurrentState != SERVICE_RUNNING)
    {
        WARN("wdmaud has not started!\n");
        return 0;
    }

    /* Sound subsystem is running. Play logon sound. */
    TRACE("Playing logon sound: %ls\n", wszDest);
    PlaySoundRoutine(wszDest, TRUE, SND_FILENAME);
    return 0;
}

static
VOID
PlayLogonSound(
    IN OUT PWLSESSION Session)
{
    HANDLE hThread;

    hThread = CreateThread(NULL, 0, PlayLogonSoundThread, (PVOID)Session->UserToken, 0, NULL);
    if (hThread)
        CloseHandle(hThread);
}

static
BOOL
HandleLogon(
    IN OUT PWLSESSION Session)
{
    PROFILEINFOW ProfileInfo;
    BOOL ret = FALSE;

    /* Loading personal settings */
    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_LOADINGYOURPERSONALSETTINGS);
    ProfileInfo.hProfile = INVALID_HANDLE_VALUE;
    if (0 == (Session->Options & WLX_LOGON_OPT_NO_PROFILE))
    {
        if (Session->Profile == NULL
         || (Session->Profile->dwType != WLX_PROFILE_TYPE_V1_0
          && Session->Profile->dwType != WLX_PROFILE_TYPE_V2_0))
        {
            ERR("WL: Wrong profile\n");
            goto cleanup;
        }

        /* Load the user profile */
        ZeroMemory(&ProfileInfo, sizeof(PROFILEINFOW));
        ProfileInfo.dwSize = sizeof(PROFILEINFOW);
        ProfileInfo.dwFlags = 0;
        ProfileInfo.lpUserName = Session->MprNotifyInfo.pszUserName;
        ProfileInfo.lpProfilePath = Session->Profile->pszProfile;
        if (Session->Profile->dwType >= WLX_PROFILE_TYPE_V2_0)
        {
            ProfileInfo.lpDefaultPath = Session->Profile->pszNetworkDefaultUserProfile;
            ProfileInfo.lpServerName = Session->Profile->pszServerName;
            ProfileInfo.lpPolicyPath = Session->Profile->pszPolicy;
        }

        if (!LoadUserProfileW(Session->UserToken, &ProfileInfo))
        {
            ERR("WL: LoadUserProfileW() failed\n");
            goto cleanup;
        }
    }

    /* Create environment block for the user */
    if (!CreateUserEnvironment(Session))
    {
        WARN("WL: SetUserEnvironment() failed\n");
        goto cleanup;
    }

    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGYOURPERSONALSETTINGS);
    UpdatePerUserSystemParameters(0, TRUE);

    /* Set default language */
    if (!SetDefaultLanguage(TRUE))
    {
        WARN("WL: SetDefaultLanguage() failed\n");
        goto cleanup;
    }

    if (!StartUserShell(Session))
    {
        //WCHAR StatusMsg[256];
        WARN("WL: WlxActivateUserShell() failed\n");
        //LoadStringW(hAppInstance, IDS_FAILEDACTIVATEUSERSHELL, StatusMsg, sizeof(StatusMsg) / sizeof(StatusMsg[0]));
        //MessageBoxW(0, StatusMsg, NULL, MB_ICONERROR);
        goto cleanup;
    }

    if (!InitializeScreenSaver(Session))
        WARN("WL: Failed to initialize screen saver\n");

    Session->hProfileInfo = ProfileInfo.hProfile;

    /* Logon has successed. Play sound. */
    PlayLogonSound(Session);

    ret = TRUE;

cleanup:
    if (Session->Profile)
    {
        HeapFree(GetProcessHeap(), 0, Session->Profile->pszProfile);
        HeapFree(GetProcessHeap(), 0, Session->Profile);
    }
    Session->Profile = NULL;
    if (!ret
     && ProfileInfo.hProfile != INVALID_HANDLE_VALUE)
    {
        UnloadUserProfile(WLSession->UserToken, ProfileInfo.hProfile);
    }
    RemoveStatusMessage(Session);
    if (!ret)
    {
        CloseHandle(Session->UserToken);
        Session->UserToken = NULL;
    }
    return ret;
}


static
DWORD
WINAPI
LogoffShutdownThread(
    LPVOID Parameter)
{
    PLOGOFF_SHUTDOWN_DATA LSData = (PLOGOFF_SHUTDOWN_DATA)Parameter;

    if (LSData->Session->UserToken != NULL && !ImpersonateLoggedOnUser(LSData->Session->UserToken))
    {
        ERR("ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        return 0;
    }

    /* Close processes of the interactive user */
    if (!ExitWindowsEx(
        EWX_INTERNAL_KILL_USER_APPS | (LSData->Flags & EWX_FLAGS_MASK) |
        (EWX_LOGOFF == (LSData->Flags & EWX_ACTION_MASK) ? EWX_INTERNAL_FLAG_LOGOFF : 0),
        0))
    {
        ERR("Unable to kill user apps, error %lu\n", GetLastError());
        RevertToSelf();
        return 0;
    }

    /* FIXME: Call ExitWindowsEx() to terminate COM processes */

    if (LSData->Session->UserToken)
        RevertToSelf();

    return 1;
}


static
NTSTATUS
CreateLogoffSecurityAttributes(
    OUT PSECURITY_ATTRIBUTES* ppsa)
{
    /* The following code is not working yet and messy */
    /* Still, it gives some ideas about data types and functions involved and */
    /* required to set up a SECURITY_DESCRIPTOR for a SECURITY_ATTRIBUTES */
    /* instance for a thread, to allow that  thread to ImpersonateLoggedOnUser(). */
    /* Specifically THREAD_SET_THREAD_TOKEN is required. */
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PSECURITY_ATTRIBUTES psa = 0;
    BYTE* pMem;
    PACL pACL;
    EXPLICIT_ACCESS Access;
    PSID pEveryoneSID = NULL;
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = { SECURITY_WORLD_SID_AUTHORITY };

    *ppsa = NULL;

    // Let's first try to enumerate what kind of data we need for this to ever work:
    // 1.  The Winlogon SID, to be able to give it THREAD_SET_THREAD_TOKEN.
    // 2.  The users SID (the user trying to logoff, or rather shut down the system).
    // 3.  At least two EXPLICIT_ACCESS instances:
    // 3.1 One for Winlogon itself, giving it the rights
    //     required to THREAD_SET_THREAD_TOKEN (as it's needed to successfully call
    //     ImpersonateLoggedOnUser).
    // 3.2 One for the user, to allow *that* thread to perform its work.
    // 4.  An ACL to hold the these EXPLICIT_ACCESS ACE's.
    // 5.  A SECURITY_DESCRIPTOR to hold the ACL, and finally.
    // 6.  A SECURITY_ATTRIBUTES instance to pull all of this required stuff
    //     together, to hand it to CreateThread.
    //
    // However, it seems struct LOGOFF_SHUTDOWN_DATA doesn't contain
    // these required SID's, why they'd have to be added.
    // The Winlogon's own SID should probably only be created once,
    // while the user's SID obviously must be created for each new user.
    // Might as well store it when the user logs on?

    if(!AllocateAndInitializeSid(&WorldAuthority,
                                 1,
                                 SECURITY_WORLD_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pEveryoneSID))
    {
        ERR("Failed to initialize security descriptor for logoff thread!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* set up the required security attributes to be able to shut down */
    /* To save space and time, allocate a single block of memory holding */
    /* both SECURITY_ATTRIBUTES and SECURITY_DESCRIPTOR */
    pMem = HeapAlloc(GetProcessHeap(),
                     0,
                     sizeof(SECURITY_ATTRIBUTES) +
                     SECURITY_DESCRIPTOR_MIN_LENGTH +
                     sizeof(ACL));
    if (!pMem)
    {
        ERR("Failed to allocate memory for logoff security descriptor!\n");
        return STATUS_NO_MEMORY;
    }

    /* Note that the security descriptor needs to be in _absolute_ format, */
    /* meaning its members must be pointers to other structures, rather */
    /* than the relative format using offsets */
    psa = (PSECURITY_ATTRIBUTES)pMem;
    SecurityDescriptor = (PSECURITY_DESCRIPTOR)(pMem + sizeof(SECURITY_ATTRIBUTES));
    pACL = (PACL)(((PBYTE)SecurityDescriptor) + SECURITY_DESCRIPTOR_MIN_LENGTH);

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow this thread to log off (and shut down the system, currently).
    ZeroMemory(&Access, sizeof(Access));
    Access.grfAccessPermissions = THREAD_SET_THREAD_TOKEN;
    Access.grfAccessMode = SET_ACCESS; // GRANT_ACCESS?
    Access.grfInheritance = NO_INHERITANCE;
    Access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    Access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    Access.Trustee.ptstrName = pEveryoneSID;

    if (SetEntriesInAcl(1, &Access, NULL, &pACL) != ERROR_SUCCESS)
    {
        ERR("Failed to set Access Rights for logoff thread. Logging out will most likely fail.\n");

        HeapFree(GetProcessHeap(), 0, pMem);
        return STATUS_UNSUCCESSFUL;
    }

    if (!InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("Failed to initialize security descriptor for logoff thread!\n");
        HeapFree(GetProcessHeap(), 0, pMem);
        return STATUS_UNSUCCESSFUL;
    }

    if (!SetSecurityDescriptorDacl(SecurityDescriptor,
                                   TRUE,     // bDaclPresent flag
                                   pACL,
                                   FALSE))   // not a default DACL
    {
        ERR("SetSecurityDescriptorDacl Error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, pMem);
        return STATUS_UNSUCCESSFUL;
    }

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = SecurityDescriptor;
    psa->bInheritHandle = FALSE;

    *ppsa = psa;

    return STATUS_SUCCESS;
}

static
VOID
DestroyLogoffSecurityAttributes(
    IN PSECURITY_ATTRIBUTES psa)
{
    if (psa)
    {
        HeapFree(GetProcessHeap(), 0, psa);
    }
}


static
NTSTATUS
HandleLogoff(
    IN OUT PWLSESSION Session,
    IN UINT Flags)
{
    PLOGOFF_SHUTDOWN_DATA LSData;
    PSECURITY_ATTRIBUTES psa;
    HANDLE hThread;
    DWORD exitCode;
    NTSTATUS Status;

    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_SAVEYOURSETTINGS);

    /* Prepare data for logoff thread */
    LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
    if (!LSData)
    {
        ERR("Failed to allocate mem for thread data\n");
        return STATUS_NO_MEMORY;
    }
    LSData->Flags = Flags;
    LSData->Session = Session;

    Status = CreateLogoffSecurityAttributes(&psa);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create a required security descriptor. Status 0x%08lx\n", Status);
        HeapFree(GetProcessHeap(), 0, LSData);
        return Status;
    }

    /* Run logoff thread */
    hThread = CreateThread(psa, 0, LogoffShutdownThread, (LPVOID)LSData, 0, NULL);

    /* we're done with the SECURITY_DESCRIPTOR */
    DestroyLogoffSecurityAttributes(psa);
    psa = NULL;

    if (!hThread)
    {
        ERR("Unable to create logoff thread, error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, LSData);
        return STATUS_UNSUCCESSFUL;
    }
    WaitForSingleObject(hThread, INFINITE);
    HeapFree(GetProcessHeap(), 0, LSData);
    if (!GetExitCodeThread(hThread, &exitCode))
    {
        ERR("Unable to get exit code of logoff thread (error %lu)\n", GetLastError());
        CloseHandle(hThread);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(hThread);
    if (exitCode == 0)
    {
        ERR("Logoff thread returned failure\n");
        return STATUS_UNSUCCESSFUL;
    }

    UnloadUserProfile(Session->UserToken, Session->hProfileInfo);
    CloseHandle(Session->UserToken);
    UpdatePerUserSystemParameters(0, FALSE);
    Session->LogonState = STATE_LOGGED_OFF;
    Session->UserToken = NULL;
    return STATUS_SUCCESS;
}

static
INT_PTR
CALLBACK
ShutdownComputerWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BTNSHTDOWNCOMPUTER:
                    EndDialog(hwndDlg, IDC_BTNSHTDOWNCOMPUTER);
                    return TRUE;
            }
            break;
        }
        case WM_INITDIALOG:
        {
            RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
            SetFocus(GetDlgItem(hwndDlg, IDC_BTNSHTDOWNCOMPUTER));
            return TRUE;
        }
    }
    return FALSE;
}

static
VOID
UninitializeSAS(
    IN OUT PWLSESSION Session)
{
    if (Session->SASWindow)
    {
        DestroyWindow(Session->SASWindow);
        Session->SASWindow = NULL;
    }
    if (Session->hEndOfScreenSaverThread)
        SetEvent(Session->hEndOfScreenSaverThread);
    UnregisterClassW(WINLOGON_SAS_CLASS, hAppInstance);
}

NTSTATUS
HandleShutdown(
    IN OUT PWLSESSION Session,
    IN DWORD wlxAction)
{
    PLOGOFF_SHUTDOWN_DATA LSData;
    HANDLE hThread;
    DWORD exitCode;

    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_REACTOSISSHUTTINGDOWN);

    /* Prepare data for shutdown thread */
    LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
    if (!LSData)
    {
        ERR("Failed to allocate mem for thread data\n");
        return STATUS_NO_MEMORY;
    }
    if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF)
        LSData->Flags = EWX_POWEROFF;
    else if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
        LSData->Flags = EWX_REBOOT;
    else
        LSData->Flags = EWX_SHUTDOWN;
    LSData->Session = Session;

    /* Run shutdown thread */
    hThread = CreateThread(NULL, 0, LogoffShutdownThread, (LPVOID)LSData, 0, NULL);
    if (!hThread)
    {
        ERR("Unable to create shutdown thread, error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, LSData);
        return STATUS_UNSUCCESSFUL;
    }
    WaitForSingleObject(hThread, INFINITE);
    HeapFree(GetProcessHeap(), 0, LSData);
    if (!GetExitCodeThread(hThread, &exitCode))
    {
        ERR("Unable to get exit code of shutdown thread (error %lu)\n", GetLastError());
        CloseHandle(hThread);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(hThread);
    if (exitCode == 0)
    {
        ERR("Shutdown thread returned failure\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Destroy SAS window */
    UninitializeSAS(Session);

    FIXME("FIXME: Call SMSS API #1\n");
    if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
        NtShutdownSystem(ShutdownReboot);
    else
    {
        if (FALSE)
        {
            /* FIXME - only show this dialog if it's a shutdown and the computer doesn't support APM */
            DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_SHUTDOWNCOMPUTER), GetDesktopWindow(), ShutdownComputerWindowProc);
        }
        NtShutdownSystem(ShutdownNoReboot);
    }
    return STATUS_SUCCESS;
}

static
VOID
DoGenericAction(
    IN OUT PWLSESSION Session,
    IN DWORD wlxAction)
{
    switch (wlxAction)
    {
        case WLX_SAS_ACTION_LOGON: /* 0x01 */
            if (HandleLogon(Session))
            {
                SwitchDesktop(Session->ApplicationDesktop);
                Session->LogonState = STATE_LOGGED_ON;
            }
            else
                Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
            break;
        case WLX_SAS_ACTION_NONE: /* 0x02 */
            if (Session->LogonState == STATE_LOGGED_OFF)
            {
                Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
            }
            break;
        case WLX_SAS_ACTION_LOCK_WKSTA: /* 0x03 */
            if (Session->Gina.Functions.WlxIsLockOk(Session->Gina.Context))
            {
                SwitchDesktop(WLSession->WinlogonDesktop);
                Session->LogonState = STATE_LOCKED;
                Session->Gina.Functions.WlxDisplayLockedNotice(Session->Gina.Context);
            }
            break;
        case WLX_SAS_ACTION_LOGOFF: /* 0x04 */
        case WLX_SAS_ACTION_SHUTDOWN: /* 0x05 */
        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF: /* 0x0a */
        case WLX_SAS_ACTION_SHUTDOWN_REBOOT: /* 0x0b */
            if (Session->LogonState != STATE_LOGGED_OFF)
            {
                if (!Session->Gina.Functions.WlxIsLogoffOk(Session->Gina.Context))
                    break;
                SwitchDesktop(WLSession->WinlogonDesktop);
                Session->Gina.Functions.WlxLogoff(Session->Gina.Context);
                if (!NT_SUCCESS(HandleLogoff(Session, EWX_LOGOFF)))
                {
                    RemoveStatusMessage(Session);
                    break;
                }
            }
            if (WLX_SHUTTINGDOWN(wlxAction))
            {
                Session->Gina.Functions.WlxShutdown(Session->Gina.Context, wlxAction);
                if (!NT_SUCCESS(HandleShutdown(Session, wlxAction)))
                {
                    RemoveStatusMessage(Session);
                    Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
                }
            }
            else
            {
                RemoveStatusMessage(Session);
                Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
            }
            break;
        case WLX_SAS_ACTION_TASKLIST: /* 0x07 */
            SwitchDesktop(WLSession->ApplicationDesktop);
            StartTaskManager(Session);
            break;
        case WLX_SAS_ACTION_UNLOCK_WKSTA: /* 0x08 */
            SwitchDesktop(WLSession->ApplicationDesktop);
            Session->LogonState = STATE_LOGGED_ON;
            break;
        default:
            WARN("Unknown SAS action 0x%lx\n", wlxAction);
    }
}

static
VOID
DispatchSAS(
    IN OUT PWLSESSION Session,
    IN DWORD dwSasType)
{
    DWORD wlxAction = WLX_SAS_ACTION_NONE;

    if (Session->LogonState == STATE_LOGGED_ON)
        wlxAction = (DWORD)Session->Gina.Functions.WlxLoggedOnSAS(Session->Gina.Context, dwSasType, NULL);
    else if (Session->LogonState == STATE_LOCKED)
        wlxAction = (DWORD)Session->Gina.Functions.WlxWkstaLockedSAS(Session->Gina.Context, dwSasType);
    else
    {
        /* Display a new dialog (if necessary) */
        switch (dwSasType)
        {
            case WLX_SAS_TYPE_TIMEOUT: /* 0x00 */
            {
                Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
                return;
            }
            default:
            {
                PSID LogonSid = NULL; /* FIXME */
                HWND hwnd;

                hwnd = GetTopDialogWindow();
                if (hwnd != NULL)
                {
                    SendMessage(hwnd, WM_USER, 0, 0);
                }

                Session->Options = 0;

                wlxAction = (DWORD)Session->Gina.Functions.WlxLoggedOutSAS(
                    Session->Gina.Context,
                    Session->SASAction,
                    &Session->LogonId,
                    LogonSid,
                    &Session->Options,
                    &Session->UserToken,
                    &Session->MprNotifyInfo,
                    (PVOID*)&Session->Profile);
                break;
            }
        }
    }

    if (dwSasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
    {
        BOOL bSecure = TRUE;
        if (!Session->Gina.Functions.WlxScreenSaverNotify(Session->Gina.Context, &bSecure))
        {
            /* Skip start of screen saver */
            SetEvent(Session->hEndOfScreenSaver);
        }
        else
        {
            StartScreenSaver(Session);
            if (bSecure)
                DoGenericAction(Session, WLX_SAS_ACTION_LOCK_WKSTA);
        }
    }
    else if (dwSasType == WLX_SAS_TYPE_SCRNSVR_ACTIVITY)
        SetEvent(Session->hUserActivity);

    DoGenericAction(Session, wlxAction);
}

static
BOOL
RegisterHotKeys(
    IN PWLSESSION Session,
    IN HWND hwndSAS)
{
    /* Register Ctrl+Alt+Del Hotkey */
    if (!RegisterHotKey(hwndSAS, HK_CTRL_ALT_DEL, MOD_CONTROL | MOD_ALT, VK_DELETE))
    {
        ERR("WL: Unable to register Ctrl+Alt+Del hotkey!\n");
        return FALSE;
    }

    /* Register Ctrl+Shift+Esc (optional) */
    Session->TaskManHotkey = RegisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC, MOD_CONTROL | MOD_SHIFT, VK_ESCAPE);
    if (!Session->TaskManHotkey)
        WARN("WL: Warning: Unable to register Ctrl+Alt+Esc hotkey!\n");
    return TRUE;
}

static
BOOL
UnregisterHotKeys(
    IN PWLSESSION Session,
    IN HWND hwndSAS)
{
    /* Unregister hotkeys */
    UnregisterHotKey(hwndSAS, HK_CTRL_ALT_DEL);

    if (Session->TaskManHotkey)
        UnregisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC);

    return TRUE;
}

static
NTSTATUS
CheckForShutdownPrivilege(
    IN DWORD RequestingProcessId)
{
    HANDLE Process;
    HANDLE Token;
    BOOL CheckResult;
    PPRIVILEGE_SET PrivSet;

    TRACE("CheckForShutdownPrivilege()\n");

    Process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, RequestingProcessId);
    if (!Process)
    {
        WARN("OpenProcess() failed with error %lu\n", GetLastError());
        return STATUS_INVALID_HANDLE;
    }
    if (!OpenProcessToken(Process, TOKEN_QUERY, &Token))
    {
        WARN("OpenProcessToken() failed with error %lu\n", GetLastError());
        CloseHandle(Process);
        return STATUS_INVALID_HANDLE;
    }
    CloseHandle(Process);
    PrivSet = HeapAlloc(GetProcessHeap(), 0, sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES));
    if (!PrivSet)
    {
        ERR("Failed to allocate mem for privilege set\n");
        CloseHandle(Token);
        return STATUS_NO_MEMORY;
    }
    PrivSet->PrivilegeCount = 1;
    PrivSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &PrivSet->Privilege[0].Luid))
    {
        WARN("LookupPrivilegeValue() failed with error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, PrivSet);
        CloseHandle(Token);
        return STATUS_UNSUCCESSFUL;
    }
    if (!PrivilegeCheck(Token, PrivSet, &CheckResult))
    {
        WARN("PrivilegeCheck() failed with error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, PrivSet);
        CloseHandle(Token);
        return STATUS_ACCESS_DENIED;
    }
    HeapFree(GetProcessHeap(), 0, PrivSet);
    CloseHandle(Token);

    if (!CheckResult)
    {
        WARN("SE_SHUTDOWN privilege not enabled\n");
        return STATUS_ACCESS_DENIED;
    }
    return STATUS_SUCCESS;
}

BOOL
WINAPI
HandleMessageBeep(UINT uType)
{
    LPWSTR EventName;

    switch(uType)
    {
    case 0xFFFFFFFF:
        EventName = NULL;
        break;
    case MB_OK:
        EventName = L"SystemDefault";
        break;
    case MB_ICONASTERISK:
        EventName = L"SystemAsterisk";
        break;
    case MB_ICONEXCLAMATION:
        EventName = L"SystemExclamation";
        break;
    case MB_ICONHAND:
        EventName = L"SystemHand";
        break;
    case MB_ICONQUESTION:
        EventName = L"SystemQuestion";
        break;
    default:
        WARN("Unhandled type %d\n", uType);
        EventName = L"SystemDefault";
    }

    return PlaySoundRoutine(EventName, FALSE, SND_ALIAS | SND_NOWAIT | SND_NOSTOP | SND_ASYNC);
}

static
LRESULT
CALLBACK
SASWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PWLSESSION Session = (PWLSESSION)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_HOTKEY:
        {
            switch (lParam)
            {
                case MAKELONG(MOD_CONTROL | MOD_ALT, VK_DELETE):
                {
                    TRACE("SAS: CONTROL+ALT+DELETE\n");
                    if (!Session->Gina.UseCtrlAltDelete)
                        break;
                    PostMessageW(Session->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_CTRL_ALT_DEL, 0);
                    return TRUE;
                }
                case MAKELONG(MOD_CONTROL | MOD_SHIFT, VK_ESCAPE):
                {
                    TRACE("SAS: CONTROL+SHIFT+ESCAPE\n");
                    DoGenericAction(Session, WLX_SAS_ACTION_TASKLIST);
                    return TRUE;
                }
            }
            break;
        }
        case WM_CREATE:
        {
            /* Get the session pointer from the create data */
            Session = (PWLSESSION)((LPCREATESTRUCT)lParam)->lpCreateParams;

            /* Save the Session pointer */
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)Session);
            if (GetSetupType())
                return TRUE;
            return RegisterHotKeys(Session, hwndDlg);
        }
        case WM_DESTROY:
        {
            if (!GetSetupType())
                UnregisterHotKeys(Session, hwndDlg);
            return TRUE;
        }
        case WM_SETTINGCHANGE:
        {
            UINT uiAction = (UINT)wParam;
            if (uiAction == SPI_SETSCREENSAVETIMEOUT
             || uiAction == SPI_SETSCREENSAVEACTIVE)
            {
                SetEvent(Session->hScreenSaverParametersChanged);
            }
            return TRUE;
        }
        case WM_LOGONNOTIFY:
        {
            switch(wParam)
            {
                case LN_MESSAGE_BEEP:
                {
                    return HandleMessageBeep(lParam);
                }
                case LN_SHELL_EXITED:
                {
                    /* lParam is the exit code */
                    if(lParam != 1)
                    {
                        SetTimer(hwndDlg, 1, 1000, NULL);
                    }
                    break;
                }
                case LN_START_SCREENSAVE:
                {
                    DispatchSAS(Session, WLX_SAS_TYPE_SCRNSVR_TIMEOUT);
                    break;
                }
                case LN_LOCK_WORKSTATION:
                {
                    DoGenericAction(Session, WLX_SAS_ACTION_LOCK_WKSTA);
                    break;
                }
                default:
                {
                    ERR("WM_LOGONNOTIFY case %d is unimplemented\n", wParam);
                }
            }
            return 0;
        }
        case WM_TIMER:
        {
            if (wParam == 1)
            {
                KillTimer(hwndDlg, 1);
                StartUserShell(Session);
            }
            break;
        }
        case WLX_WM_SAS:
        {
            DispatchSAS(Session, (DWORD)wParam);
            return TRUE;
        }
        case PM_WINLOGON_EXITWINDOWS:
        {
            UINT Flags = (UINT)lParam;
            UINT Action = Flags & EWX_ACTION_MASK;
            DWORD wlxAction;

            /* Check parameters */
            switch (Action)
            {
                case EWX_LOGOFF: wlxAction = WLX_SAS_ACTION_LOGOFF; break;
                case EWX_SHUTDOWN: wlxAction = WLX_SAS_ACTION_SHUTDOWN; break;
                case EWX_REBOOT: wlxAction = WLX_SAS_ACTION_SHUTDOWN_REBOOT; break;
                case EWX_POWEROFF: wlxAction = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF; break;
                default:
                {
                    ERR("Invalid ExitWindows action 0x%x\n", Action);
                    return STATUS_INVALID_PARAMETER;
                }
            }

            if (WLX_SHUTTINGDOWN(wlxAction))
            {
                NTSTATUS Status = CheckForShutdownPrivilege((DWORD)wParam);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
            DoGenericAction(Session, wlxAction);
            return 1;
        }
    }

    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

BOOL
InitializeSAS(
    IN OUT PWLSESSION Session)
{
    WNDCLASSEXW swc;
    BOOL ret = FALSE;

    if (!SwitchDesktop(Session->WinlogonDesktop))
    {
        ERR("WL: Failed to switch to winlogon desktop\n");
        goto cleanup;
    }

    /* Register SAS window class */
    swc.cbSize = sizeof(WNDCLASSEXW);
    swc.style = CS_SAVEBITS;
    swc.lpfnWndProc = SASWindowProc;
    swc.cbClsExtra = 0;
    swc.cbWndExtra = 0;
    swc.hInstance = hAppInstance;
    swc.hIcon = NULL;
    swc.hCursor = NULL;
    swc.hbrBackground = NULL;
    swc.lpszMenuName = NULL;
    swc.lpszClassName = WINLOGON_SAS_CLASS;
    swc.hIconSm = NULL;
    if (RegisterClassExW(&swc) == 0)
    {
        ERR("WL: Failed to register SAS window class\n");
        goto cleanup;
    }

    /* Create invisible SAS window */
    Session->SASWindow = CreateWindowExW(
        0,
        WINLOGON_SAS_CLASS,
        WINLOGON_SAS_TITLE,
        WS_POPUP,
        0, 0, 0, 0, 0, 0,
        hAppInstance, Session);
    if (!Session->SASWindow)
    {
        ERR("WL: Failed to create SAS window\n");
        goto cleanup;
    }

    /* Register SAS window to receive SAS notifications */
    if (!SetLogonNotifyWindow(Session->SASWindow, Session->InteractiveWindowStation))
    {
        ERR("WL: Failed to register SAS window\n");
        goto cleanup;
    }

    if (!SetDefaultLanguage(FALSE))
        return FALSE;

    ret = TRUE;

cleanup:
    if (!ret)
        UninitializeSAS(Session);
    return ret;
}
