#include <windows.h>
#include <wininet.h>
#include <direct.h>
#include "MobileDevice.h"

extern void write_stuff();

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
char *szClassName = TEXT("WindowsApp");
HWND window, button, title, status, edit = NULL;
HHOOK hMsgBoxHook;

char *firmwareVersion = NULL, *deviceModel = NULL;
BOOL deviceConnected = FALSE;
BOOL jailbreakSucceeded = FALSE;
BOOL isJailbreaking = FALSE;
PROCESS_INFORMATION   pinfo;
char cfpath[MAX_PATH];
char mdpath[MAX_PATH];
DWORD exitCode = 0;
char *output = NULL;

HMODULE mobile_device_framework;
int (*am_device_notification_subscribe)(void *, int, int, void *, am_device_notification **);
CFStringRef (*am_copy_value)(am_device *, CFStringRef, CFStringRef);
mach_error_t (*am_device_connect)(struct am_device *device);
mach_error_t (*am_device_is_paired)(struct am_device *device);
mach_error_t (*am_device_validate_pairing)(struct am_device *device);
mach_error_t (*am_device_start_session)(struct am_device *device);

HMODULE corefoundation;
int (*cf_string_get_cstring)(CFStringRef, char *, CFIndex, CFStringEncoding);
CFStringRef (*cf_string_create_with_cstring)(CFAllocatorRef, char *, CFStringEncoding);
void (*cf_release)(CFTypeRef);
CFDataRef (*cf_data_create)(CFAllocatorRef, char *, int);
BOOL (*cf_dictionary_contains_key)(CFDictionaryRef, CFStringRef);
CFPropertyListRef (*cf_propertylist_create_with_data)(CFAllocatorRef, CFDataRef, CFOptionFlags, CFPropertyListFormat *, CFErrorRef *);

BOOL MessageLoop(BOOL blocking);

int load_core_foundation()
{
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Apple Inc.\\Apple Application Support"), 0, KEY_QUERY_VALUE, &hKey);
    DWORD itunesdll_size = MAX_PATH;

    if (RegQueryValueEx(hKey, TEXT("InstallDir"), NULL, NULL, cfpath, &itunesdll_size) != ERROR_SUCCESS) {
        return 1;
    }

    // Thanks geohot and psuskeels
    SetCurrentDirectory(cfpath);

    char path[MAX_PATH];
    sprintf(path, "%s%s", cfpath, "CoreFoundation.dll");
    corefoundation = LoadLibrary(path);
    if (corefoundation == NULL) {
        return 1;
    }
    
    // Strip trailing backslash
    cfpath[strlen(cfpath) - 1] = 0;

    cf_data_create = (void *) GetProcAddress(corefoundation, "CFDataCreate");
    cf_release = (void *) GetProcAddress(corefoundation, "CFRelease");
    cf_dictionary_contains_key = (void *) GetProcAddress(corefoundation, "CFDictionaryContainsKey");
    cf_propertylist_create_with_data = (void *) GetProcAddress(corefoundation, "CFPropertyListCreateWithData");
    cf_string_get_cstring = (void *) GetProcAddress(corefoundation, "CFStringGetCString");
    cf_string_create_with_cstring = (void *) GetProcAddress(corefoundation, "CFStringCreateWithCString");

    return (corefoundation                    == NULL ||
        cf_release                            == NULL ||
        cf_data_create                        == NULL ||
        cf_dictionary_contains_key            == NULL ||
        cf_propertylist_create_with_data    == NULL ||
        cf_string_get_cstring                == NULL ||
        cf_string_create_with_cstring        == NULL);
}

int load_mobile_device() 
{
    if (load_core_foundation()) {
        return 1;                                         
    }

    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Apple Inc.\\Apple Mobile Device Support\\Shared"), 0, KEY_QUERY_VALUE, &hKey);
    DWORD itunesdll_size = MAX_PATH;
    char dllpath[MAX_PATH];

    if (RegQueryValueEx(hKey, TEXT("iTunesMobileDeviceDLL"), NULL, NULL, dllpath, &itunesdll_size) != ERROR_SUCCESS) {
        return 1;
    }

    mobile_device_framework = LoadLibrary(dllpath);
    if (mobile_device_framework == NULL) {
        return 1;
    }
    
    // Strip off last path component and backslash
    _splitpath(dllpath, mdpath, mdpath + 2, NULL, NULL);
    mdpath[strlen(mdpath) - 1] = 0;

    am_device_notification_subscribe = (void *) GetProcAddress(mobile_device_framework, "AMDeviceNotificationSubscribe");
    am_device_connect = (void *) GetProcAddress(mobile_device_framework, "AMDeviceConnect");
    am_device_is_paired = (void *) GetProcAddress(mobile_device_framework, "AMDeviceIsPaired");
    am_device_validate_pairing = (void *) GetProcAddress(mobile_device_framework, "AMDeviceValidatePairing");
    am_device_start_session = (void *) GetProcAddress(mobile_device_framework, "AMDeviceStartSession");
    am_copy_value = (void *) GetProcAddress(mobile_device_framework, "AMDeviceCopyValue");

    return (mobile_device_framework            == NULL ||
        am_device_notification_subscribe    == NULL ||
        am_device_connect                    == NULL ||
        am_copy_value                        == NULL ||
        am_device_is_paired                    == NULL ||
        am_device_validate_pairing            == NULL ||
        am_device_start_session                == NULL);
}

static void path_for_file(char *file, char *where, int len)
{
    // ya can change this to change where the files are when extraction is added
    strncpy(where, file, len);
}

static BOOL jailbreak_ready()
{
    char key[0x20];
    sprintf(key, "%s_%s", deviceModel, firmwareVersion);
    
    char path[MAX_PATH];
    path_for_file("igor/map.plist", path, MAX_PATH);
    FILE *pf = fopen(path, "r");
    if (pf == NULL)
        return FALSE;
    fseek(pf, 0, SEEK_END);
    int size = ftell(pf);
    fseek(pf, 0, SEEK_SET);
    
    char *plistdata = malloc(size);
    fread(plistdata, size, 1, pf);
    fclose(pf);

    CFDataRef data = cf_data_create(NULL, plistdata, size);
    free(plistdata);
    
    CFDictionaryRef dict = (CFDictionaryRef) cf_propertylist_create_with_data(NULL, data, kCFPropertyListImmutable, NULL, NULL);
    if (dict == NULL) {
        cf_release(data);
        return FALSE;
    }
    
    CFStringRef keyref = cf_string_create_with_cstring(NULL, key, kCFStringEncodingASCII);
    BOOL ret = cf_dictionary_contains_key(dict, keyref);
    
    cf_release(data);
    cf_release(keyref);
    cf_release(dict);

    return ret;
}

static char *device_copy_value(am_device *device, char * key)
{
    char *buf = malloc(0x21);
    
    CFStringRef keyref = cf_string_create_with_cstring(NULL, key, kCFStringEncodingASCII);
    CFStringRef value = am_copy_value(device, NULL, keyref);
    cf_release(keyref);
    cf_string_get_cstring(value, buf, 0x20, kCFStringEncodingASCII);
    cf_release(value);
    
    return buf;
}

static char *device_firmware_string(am_device *device)
{
    return device_copy_value(device, "ProductVersion");
}

static char *device_model_string(am_device *device)
{
    return device_copy_value(device, "ProductType");
}

static void device_notification_callback(am_device_notification_callback_info *info, void *foo) 
{
    if (isJailbreaking) return;
    
    deviceConnected = TRUE;

    if (info->msg != ADNCI_MSG_CONNECTED) { 
        deviceConnected = FALSE; 
    }
    if (am_device_connect(info->dev)) { 
        deviceConnected = FALSE; 
    }
    if (!am_device_is_paired(info->dev)) { 
        deviceConnected = FALSE; 
    }
    if (am_device_validate_pairing(info->dev)) { 
        deviceConnected = FALSE; 
    }
    if (am_device_start_session(info->dev)) { 
        deviceConnected = FALSE; 
    }

    if (deviceConnected) {
        if (jailbreakSucceeded) jailbreakSucceeded = FALSE;
        
        firmwareVersion = device_firmware_string(info->dev);
        deviceModel = device_model_string(info->dev);
        
        char display[0x200];
        strcpy(display, deviceModel);
        if (!strcmp(deviceModel, "iPhone1,1")) strcpy(display, "iPhone 2G");
        if (!strcmp(deviceModel, "iPhone1,2")) strcpy(display, "iPhone 3G");
        if (!strcmp(deviceModel, "iPhone2,1")) strcpy(display, "iPhone 3GS");
        if (!strcmp(deviceModel, "iPod1,1")) strcpy(display, "iPod touch 1G");
        if (!strcmp(deviceModel, "iPod2,1")) strcpy(display, "iPod touch 2G");
        if (!strcmp(deviceModel, "iPod3,1")) strcpy(display, "iPod touch 3G");
        if (!strcmp(deviceModel, "iPad1,1")) strcpy(display, "iPad");
        
        char text[0x200];
        if (jailbreak_ready()) {
            sprintf(text, "Ready: %s (%s) connected.", display, firmwareVersion);
            EnableWindow(button, TRUE);
        } else {
            sprintf(text, "Device %s (%s) is not supported.", display, firmwareVersion);
            EnableWindow(button, FALSE);
        }
        SendMessage(status, WM_SETTEXT, (WPARAM) NULL, (LPARAM) text);
    } else {
        if (jailbreakSucceeded) return;
        
        SendMessage(status, WM_SETTEXT, (WPARAM) NULL, (LPARAM) TEXT("Please connect device."));
        SendMessage(button, WM_SETTEXT, (WPARAM) NULL, (LPARAM) TEXT("Jailbreak"));
        EnableWindow(button, FALSE);
    }
}

BOOL CALLBACK window_callback(HWND hwnd, LPARAM lParam) 
{
    char windowclass[0x20];
    GetClassName(hwnd, windowclass, 0x20);
    if (!_stricmp(windowclass, "BUTTON")) {
        RECT r;
        POINT tl, br;
        
        GetWindowRect(hwnd, &r);
        tl.x = r.left; tl.y = r.top;
        br.x = r.right; br.y = r.bottom;
        
        ScreenToClient(GetParent(hwnd), &br);
        ScreenToClient(GetParent(hwnd), &tl);
        
        MoveWindow(hwnd, tl.x, tl.y + 265, br.x - tl.x, br.y - tl.y , TRUE);
        UpdateWindow(GetParent(hwnd));
    }
}

LRESULT CALLBACK message_box_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if(nCode < 0)
        return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);

    if (nCode == HCBT_ACTIVATE) {
        hwnd = (HWND) wParam;
        
        RECT r;
        GetWindowRect(hwnd, &r);
        MoveWindow(hwnd, r.left, r.top, r.right - r.left, 400, TRUE);
        UpdateWindow(hwnd);
        UpdateWindow(GetParent(hwnd));
        
        EnumChildWindows(hwnd, window_callback, 0);
            
           edit = CreateWindowEx(0, TEXT("EDIT"), NULL, WS_VISIBLE | WS_CHILD | ES_LEFT | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 20, 60, r.right - r.left - 40, 240, hwnd, NULL, NULL, NULL);
           SendMessage(edit, WM_SETTEXT, 0, (LPARAM) output); 
           SendMessage(edit, WM_SETFONT, (WPARAM) CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma")), TRUE);
               
        UnhookWindowsHookEx(hMsgBoxHook);
        return 0;
    }

    return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
}

void performJailbreak() 
{            
    isJailbreaking = TRUE;
    EnableWindow(button, FALSE);
    SendMessage(button, WM_SETTEXT, (WPARAM) NULL, (LPARAM) TEXT("Jailbreaking..."));
    
    char statusText[0x200];
    
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO startup;

    int ret;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    HANDLE pipe_read = CreateNamedPipe("\\\\.\\pipe\\spirit", PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, PIPE_UNLIMITED_INSTANCES, 4096, 4096, INFINITE, &sa);
    SetHandleInformation(pipe_read, HANDLE_FLAG_INHERIT, 0);
    HANDLE pipe_write = CreateFile("\\\\.\\pipe\\spirit", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, NULL);
    
    HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = pipe_write;
    startup.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    startup.dwFlags    = STARTF_USESTDHANDLES;

    ZeroMemory(&pinfo, sizeof(pinfo));
    
    // Create argv for dl
    char dlpath[MAX_PATH];
    path_for_file("dl/dl.exe", dlpath, MAX_PATH);
    char argv[MAX_PATH + 5];
    char buf[MAX_PATH];
    GetModuleFileName(NULL, buf, MAX_PATH);
    sprintf(argv, "dl \"%s\"", buf);
    
    // Create PATH for dl
    char oldpath[4096];
    GetEnvironmentVariable("PATH", oldpath, 4096);
    char newpath[4096];
    sprintf(newpath, "PATH=%s;%s;%s", oldpath, mdpath, cfpath);
    
    LPTCH envp = GetEnvironmentStrings();
    char *newenvp[0x4000];
    char *cur = (char *) envp, *newcur = (char *) newenvp;
    
    while (*cur) {
        if (!_strnicmp(cur, "PATH", 4)) {
            strcpy(newcur, newpath);
            newcur += strlen(newpath) + 1;
        } else {
            strcpy(newcur, cur);
            newcur += strlen(cur) + 1;
        }
        
        cur += strlen(cur) + 1;
    }
    
    FreeEnvironmentStrings(envp);

    // Launch dl
    if (CreateProcess(                          
            dlpath,              /* program path  */
            argv,                /* argv */
            NULL,                   /* process handle is not inheritable */
            NULL,                    /* thread handle is not inheritable */
            TRUE,                          /* yes, inherit some handles */
            DETACHED_PROCESS, /* the new process doesn't have a console */
            newenvp,                     /* environment block */
            NULL,                    /* use parent's starting directory */
            &startup,                 /* startup info, i.e. std handles */
            &pinfo) == FALSE) {
        output = malloc(0x200);
        strcpy(output, "ERROR: Unable to launch dl.");
        exitCode = 0x00000149;
        goto complete;
    }
    
    CloseHandle(pipe_write);

    int outputsize = 0x200;
    output = malloc(0x200);
    int offset = 0;
    BOOL reading = FALSE;
    OVERLAPPED overlapped;
    DWORD count_;
   
    // Wait for the process to complete
    while (1) {
        //MessageBox(NULL, reading ? "reading" : "not reading", "", MB_OK);
        if(!reading) {
            ResetEvent(event);
            ZeroMemory(&overlapped, sizeof(overlapped));
            overlapped.hEvent = event;
            ReadFile(pipe_read, offset + output, 0x200, &count_, &overlapped);
            reading = TRUE;
        }

        HANDLE objs[2] = {pinfo.hProcess, event};

        DWORD ret = MsgWaitForMultipleObjects(2, objs, FALSE, INFINITE, QS_ALLINPUT);
        if(ret == WAIT_OBJECT_0) {
            // Process finished
            break;
        } else if(ret == WAIT_OBJECT_0 + 1) {
            // We got some input
            DWORD count = overlapped.InternalHigh;
            outputsize += count;
            offset += count;
            output = realloc(output, outputsize);
            reading = FALSE;
        } else {
            // A message
            while(MessageLoop(FALSE));
        }
    }

    if(overlapped.Internal != STATUS_PENDING) {
        DWORD count = overlapped.InternalHigh;
        outputsize += count;
        offset += count;
        output = realloc(output, outputsize);
    }

    CancelIo(pipe_read);

    // \n -> \r\n
    //{char buf[500]; sprintf(buf, "=%d", offset); MessageBox(NULL, buf, buf, MB_OK);}
    output[offset] = 0;
    char *output2 = malloc(2*offset+1);
    char *p = output, *q = output2;
    char c;
    while(c = *p++) {
        if(c == '\n') *q++ = '\r';
        *q++ = c;
    }
    *q++ = 0;
    free(output);
    output = output2;
    
    GetExitCodeProcess(pinfo.hProcess, &exitCode);
    
    CloseHandle(pipe_read);
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);
    
complete:
    EnableWindow(button, TRUE);
    if (exitCode == 0) {
        jailbreakSucceeded = TRUE;
        
        sprintf(statusText, "Jailbreak succeeded!");
        SendMessage(button, WM_SETTEXT, (WPARAM) NULL, (LPARAM) TEXT("Quit"));
        SendMessage(status, WM_SETTEXT, (WPARAM) NULL, (LPARAM) statusText);
    } else {
        jailbreakSucceeded = FALSE;
        
        sprintf(statusText, "Failed to jailbreak (error code: %x).", exitCode);
        SendMessage(status, WM_SETTEXT, (WPARAM) NULL, (LPARAM) statusText);
        SendMessage(button, WM_SETTEXT, (WPARAM) NULL, (LPARAM) TEXT("Retry"));
        
        // To add teh textbox
        hMsgBoxHook = SetWindowsHookEx(WH_CBT, message_box_hook, NULL, GetCurrentThreadId());
        int selection = MessageBox(window, TEXT("Spirit encountered an error while jailbreaking your device. Do you want to send a log of what happened?"), TEXT("Welp!"), MB_ICONEXCLAMATION | MB_YESNO);
        
        if (selection == IDYES) {
            static TCHAR headers[] = "Content-Type: text/plain";
            static LPCTSTR accept[2] = {"*/*", NULL};
            HINTERNET session, connect, request;
            
            // Cascading ifs: they all return NULL on failure
            if (session = InternetOpen("Spirit", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC)) {
                if (connect = InternetConnect(session, "spiritjb.com", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1)) {
                    if (request = HttpOpenRequest(connect, "POST", "/post.php", HTTP_VERSION, NULL, accept, INTERNET_FLAG_RELOAD, 1)) {
                        int ret = HttpSendRequest(request, headers, strlen(headers), output, strlen(output));
                    }
                }
            }
        }
    }
    
    if (output) free(output);
    isJailbreaking = FALSE;
}

BOOL MessageLoop(BOOL blocking) {
    MSG messages;

    if (!(blocking ? 
        GetMessage(&messages, NULL, 0, 0) :
        PeekMessage(&messages, NULL, 0, 0, PM_REMOVE)
    ))
        return FALSE;

    TranslateMessage(&messages);
    DispatchMessage(&messages);

    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
     // Load iTunes
    if (load_mobile_device() != 0) {
        MessageBox(NULL, TEXT("Spirit requires iTunes 9 to function. Please install a correct version of iTunes then run Spirit again."), TEXT("Error"), 64);
        return 0;
    }
    
    // Setup device callback
    am_device_notification *notif;
    am_device_notification_subscribe(device_notification_callback, 0, 0, NULL, &notif);
    
    // Register window class
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WindowProcedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), TEXT("ID"));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szClassName;
    wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), TEXT("ID"), IMAGE_ICON, 16, 16, 0);
    if(!RegisterClassEx(&wc)) return 0;

    // Create the window
    window = CreateWindowEx(0, szClassName, TEXT("Spirit"), WS_OVERLAPPED | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 320, 130, HWND_DESKTOP, NULL, hInstance, NULL);

    // Title label
    title = CreateWindowEx(0, TEXT("STATIC"), TEXT("Spirit"), WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 10, 320, 25, window, NULL, NULL, NULL);
    SendMessage(title, WM_SETFONT, (WPARAM) CreateFont(25, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma")), TRUE);

    // Jailbreak button
    button = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Jailbreak"), BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD, 100, 42, 120, 25, window, NULL, NULL, NULL);
    SendMessage(button, WM_SETFONT, (WPARAM) CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma")), TRUE);
    EnableWindow(button, FALSE);
    
    // Status label
    status = CreateWindowEx(0, TEXT("STATIC"), TEXT("Please connect device."), WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 75, 320, 18, window, NULL, NULL, NULL);
    SendMessage(status, WM_SETFONT, (WPARAM) CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma")), TRUE);

    // Show the window
    ShowWindow(window, nFunsterStil);
    
    char path[MAX_PATH];
    GetTempPath(MAX_PATH, path);
    strcat(path, "spirit");
    sprintf(path + strlen(path), "%d", GetCurrentProcessId());
    _mkdir(path);
    SetCurrentDirectory(path);
    write_stuff();

    // Run the main runloop.
    while(MessageLoop(TRUE));

    return 0;
}


LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (jailbreakSucceeded) {
            PostQuitMessage(0);
            return 0;
        }
        
        performJailbreak();
        
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        
        if (isJailbreaking)
            TerminateProcess(pinfo.hProcess, 0);
            
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
