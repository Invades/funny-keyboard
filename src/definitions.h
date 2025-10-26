// (blame winapi conflicting, make a PR if you know a better way)

typedef struct HWND__* HWND;
typedef struct HHOOK__* HHOOK;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef long LONG;
typedef int BOOL;
typedef unsigned int UINT;
typedef long long LONG_PTR;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef long long LRESULT;
typedef void* FARPROC;

typedef struct tagKBDLLHOOKSTRUCT {
    DWORD vkCode;
    DWORD scanCode;
    DWORD flags;
    DWORD time;
    DWORD* dwExtraInfo;
} KBDLLHOOKSTRUCT;

typedef LRESULT (__stdcall *HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);

extern "C" {
    __declspec(dllimport) LONG __stdcall GetWindowLongA(HWND hWnd, int nIndex);
    __declspec(dllimport) LONG __stdcall SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
    __declspec(dllimport) BOOL __stdcall SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
    __declspec(dllimport) BOOL __stdcall ShowWindow(HWND hWnd, int nCmdShow);
    __declspec(dllimport) HHOOK __stdcall SetWindowsHookExA(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId);
    __declspec(dllimport) BOOL __stdcall UnhookWindowsHookEx(HHOOK hhk);
    __declspec(dllimport) LRESULT __stdcall CallNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam);
    __declspec(dllimport) HINSTANCE __stdcall GetModuleHandleA(const char* lpModuleName);
    __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
    __declspec(dllimport) UINT __stdcall MapVirtualKeyA(UINT uCode, UINT uMapType);
    __declspec(dllimport) int __stdcall GetSystemMetrics(int nIndex);
    __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);
    __declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE hModule, const char* lpProcName);
    __declspec(dllimport) BOOL __stdcall InvalidateRect(HWND hWnd, const void* lpRect, BOOL bErase);
    __declspec(dllimport) BOOL __stdcall UpdateWindow(HWND hWnd);
    __declspec(dllimport) int __stdcall MessageBoxA(HWND hWnd, const char* lpText, const char* lpCaption, UINT uType);
}
#define KF_UP 0x8000
#define LLKHF_UP (KF_UP >> 8)

#define MB_OK 0x00000000L
#define MB_ICONERROR 0x00000010L

#define GetWindowLong GetWindowLongA
#define SetWindowLong SetWindowLongA
#define SetWindowsHookEx SetWindowsHookExA
#define GetModuleHandle GetModuleHandleA
#define MapVirtualKey MapVirtualKeyA

#define GWL_EXSTYLE (-20)
#define WS_EX_TOOLWINDOW 0x00000080L
#define WS_EX_APPWINDOW 0x00040000L
#define WS_EX_TRANSPARENT 0x00000020L
#define WS_EX_LAYERED 0x00080000L
#define HWND_TOPMOST ((HWND)-1)
#define WS_EX_NOACTIVATE 0x08000000L
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0001
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define SW_HIDE 0
#define SW_SHOW 5
#define SW_SHOWNOACTIVATE 4

#define WH_KEYBOARD_LL 13
#define HC_ACTION 0
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define VK_BACK 0x08
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_SPACE 0x20
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_F13 0x7C
#define VK_F14 0x7D
#define VK_F15 0x7E
#define VK_F16 0x7F
#define VK_F17 0x80
#define VK_F18 0x81
#define VK_F19 0x82
#define VK_F20 0x83
#define VK_F21 0x84
#define VK_F22 0x85
#define VK_F23 0x86
#define VK_F24 0x87
#define MAPVK_VK_TO_CHAR 2
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1