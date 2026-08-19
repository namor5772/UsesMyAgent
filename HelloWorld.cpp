#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")

namespace
{
constexpr wchar_t kWindowClassName[] = L"UsesMyAgentHelloWorldWindow";
constexpr wchar_t kWindowTitle[] = L"Hello World";
constexpr wchar_t kGreeting[] = L"Hello World!";
constexpr wchar_t kSettingsRegistryPath[] = L"Software\\UsesMyAgent\\HelloWorld";
constexpr wchar_t kWindowPlacementValueName[] = L"WindowPlacement";
constexpr wchar_t kShortcutIconPath[] = L"%SystemRoot%\\System32\\main.cpl";
constexpr int kShortcutIconIndex = 0;
constexpr int kGreetingControlId = 1001;

struct WindowIcons
{
    HICON largeIcon = nullptr;
    HICON smallIcon = nullptr;
};

WindowIcons LoadShortcutIcons()
{
    wchar_t expandedIconPath[MAX_PATH]{};
    if (ExpandEnvironmentStringsW(
            kShortcutIconPath,
            expandedIconPath,
            static_cast<DWORD>(MAX_PATH))
        == 0)
    {
        return {};
    }

    WindowIcons icons{};
    ExtractIconExW(
        expandedIconPath,
        kShortcutIconIndex,
        &icons.largeIcon,
        nullptr,
        1);

    if (icons.largeIcon != nullptr)
    {
        icons.smallIcon = reinterpret_cast<HICON>(CopyImage(
            icons.largeIcon,
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            0));
    }

    return icons;
}

void DestroyWindowIcons(const WindowIcons& icons)
{
    if (icons.largeIcon != nullptr)
    {
        DestroyIcon(icons.largeIcon);
    }

    if (icons.smallIcon != nullptr && icons.smallIcon != icons.largeIcon)
    {
        DestroyIcon(icons.smallIcon);
    }
}

bool IsWindowPlacementUsable(const WINDOWPLACEMENT& placement)
{
    const RECT& bounds = placement.rcNormalPosition;
    if (placement.length != sizeof(WINDOWPLACEMENT)
        || bounds.right <= bounds.left
        || bounds.bottom <= bounds.top)
    {
        return false;
    }

    return MonitorFromRect(&bounds, MONITOR_DEFAULTTONULL) != nullptr;
}

bool LoadWindowPlacement(WINDOWPLACEMENT& placement)
{
    placement = {};
    placement.length = sizeof(placement);

    DWORD valueType = 0;
    DWORD valueSize = static_cast<DWORD>(sizeof(placement));
    const LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        kSettingsRegistryPath,
        kWindowPlacementValueName,
        RRF_RT_REG_BINARY,
        &valueType,
        &placement,
        &valueSize);

    if (status != ERROR_SUCCESS
        || valueType != REG_BINARY
        || valueSize != sizeof(placement)
        || !IsWindowPlacementUsable(placement))
    {
        return false;
    }

    const bool restoreMaximized = placement.showCmd == SW_SHOWMAXIMIZED
        || (placement.flags & WPF_RESTORETOMAXIMIZED) != 0;
    placement.flags = 0;
    placement.showCmd = restoreMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
    return true;
}

void SaveWindowPlacement(HWND window)
{
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    if (GetWindowPlacement(window, &placement) == FALSE
        || !IsWindowPlacementUsable(placement))
    {
        return;
    }

    const bool restoreMaximized = placement.showCmd == SW_SHOWMAXIMIZED
        || (placement.flags & WPF_RESTORETOMAXIMIZED) != 0;
    placement.flags = 0;
    placement.showCmd = restoreMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;

    HKEY settingsKey = nullptr;
    if (RegCreateKeyExW(
            HKEY_CURRENT_USER,
            kSettingsRegistryPath,
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            nullptr,
            &settingsKey,
            nullptr)
        != ERROR_SUCCESS)
    {
        return;
    }

    RegSetValueExW(
        settingsKey,
        kWindowPlacementValueName,
        0,
        REG_BINARY,
        reinterpret_cast<const BYTE*>(&placement),
        static_cast<DWORD>(sizeof(placement)));
    RegCloseKey(settingsKey);
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        const HWND greeting = CreateWindowExW(
            0,
            L"STATIC",
            kGreeting,
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            0,
            0,
            0,
            0,
            window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kGreetingControlId)),
            GetModuleHandleW(nullptr),
            nullptr);

        if (greeting == nullptr)
        {
            return -1;
        }

        SendMessageW(
            greeting,
            WM_SETFONT,
            reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
            TRUE);
        return 0;
    }

    case WM_SIZE:
    {
        const HWND greeting = GetDlgItem(window, kGreetingControlId);
        if (greeting != nullptr)
        {
            MoveWindow(
                greeting,
                0,
                0,
                LOWORD(lParam),
                HIWORD(lParam),
                TRUE);
        }
        return 0;
    }

    case WM_CLOSE:
        SaveWindowPlacement(window);
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}
} // namespace

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    const WindowIcons icons = LoadShortcutIcons();

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.hIcon = icons.largeIcon;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hIconSm = icons.smallIcon != nullptr ? icons.smallIcon : icons.largeIcon;

    if (RegisterClassExW(&windowClass) == 0)
    {
        DestroyWindowIcons(icons);
        return 1;
    }

    constexpr int defaultWindowWidth = 640;
    constexpr int defaultWindowHeight = 360;
    const int defaultPositionX = (GetSystemMetrics(SM_CXSCREEN) - defaultWindowWidth) / 2;
    const int defaultPositionY = (GetSystemMetrics(SM_CYSCREEN) - defaultWindowHeight) / 2;

    WINDOWPLACEMENT savedPlacement{};
    const bool hasSavedPlacement = LoadWindowPlacement(savedPlacement);

    const HWND window = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        defaultPositionX,
        defaultPositionY,
        defaultWindowWidth,
        defaultWindowHeight,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (window == nullptr)
    {
        UnregisterClassW(kWindowClassName, instance);
        DestroyWindowIcons(icons);
        return 1;
    }

    SendMessageW(
        window,
        WM_SETICON,
        ICON_BIG,
        reinterpret_cast<LPARAM>(icons.largeIcon));
    SendMessageW(
        window,
        WM_SETICON,
        ICON_SMALL,
        reinterpret_cast<LPARAM>(
            icons.smallIcon != nullptr ? icons.smallIcon : icons.largeIcon));

    if (hasSavedPlacement)
    {
        SetWindowPlacement(window, &savedPlacement);
    }

    ShowWindow(
        window,
        hasSavedPlacement ? static_cast<int>(savedPlacement.showCmd) : showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    UnregisterClassW(kWindowClassName, instance);
    DestroyWindowIcons(icons);
    return static_cast<int>(message.wParam);
}
