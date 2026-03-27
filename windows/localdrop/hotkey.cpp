#include "hotkey.h"
#include <windows.h>
#include <map>
#include <vector>

// Internal State (Hidden from main.cpp)
static HHOOK hHook = NULL;
static HotkeyCallback g_callback = nullptr;
static std::map<HANDLE, int> g_DeviceModifierMap;
static HANDLE g_LastPhysicalHandle = NULL;

// Helper: Updates the bitmask for a specific hardware device
static void UpdateModifierMap(HANDLE hDevice, USHORT vKey, bool isDown) {
    int mask = 0;
    if (vKey == VK_CONTROL || vKey == VK_LCONTROL || vKey == VK_RCONTROL) mask = 1;
    else if (vKey == VK_SHIFT || vKey == VK_LSHIFT || vKey == VK_RSHIFT)   mask = 2;
    else if (vKey == VK_MENU || vKey == VK_LMENU || vKey == VK_RMENU)      mask = 4;

    if (mask > 0) {
        if (isDown) g_DeviceModifierMap[hDevice] |= mask;
        else        g_DeviceModifierMap[hDevice] &= ~mask;
    }
}

// Internal Window Proc for Raw Input
static LRESULT CALLBACK MessageWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_INPUT) {
        UINT dwSize=0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        std::vector<BYTE> buffer(dwSize);
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) >= 0) {
            RAWINPUT* raw = (RAWINPUT*)buffer.data();
            if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                UpdateModifierMap(raw->header.hDevice, raw->data.keyboard.VKey, !(raw->data.keyboard.Flags & RI_KEY_BREAK));
                if (!(raw->data.keyboard.Flags & RI_KEY_BREAK)) {
                    g_LastPhysicalHandle = raw->header.hDevice;
                }
            }
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// The Hook Procedure
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;

        if (!(pKey->flags & LLKHF_INJECTED)) {
            // Check the modifier state of the specific device that sent this key
            if (g_DeviceModifierMap[g_LastPhysicalHandle] == 0x7) {
                if (pKey->vkCode == 'C' || pKey->vkCode == 'V') {
                    if (g_callback) g_callback(pKey->vkCode == 'C' ? HotkeyAction::COPY : HotkeyAction::PASTE);
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void StartHook(HotkeyCallback callback) {
    g_callback = callback;

    // 1. Create Hidden Message-Only Window
    WNDCLASSEXW wx = { sizeof(WNDCLASSEXW) };
    wx.lpfnWndProc = MessageWindowProc;
    wx.lpszClassName = L"InterceptorInternalMsgWin";
    RegisterClassExW(&wx);
    HWND hWnd = CreateWindowExW(0, wx.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    // 2. Register Raw Input
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hWnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    // 3. Set the Hook
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    // 4. Message Loop (Keeps hook and raw input window alive)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void StopHook() {
    // 1. Post the quit message to break the GetMessage loop in StartHook
    PostQuitMessage(0);
}