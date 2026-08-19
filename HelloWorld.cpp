#include <windows.h>
#include <shellapi.h>

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

#pragma comment(lib, "Shell32.lib")

namespace
{
constexpr wchar_t kWindowClassName[] = L"UsesMyAgentHelloWorldWindow";
constexpr wchar_t kWindowTitle[] = L"Calculator";
constexpr wchar_t kSettingsRegistryPath[] = L"Software\\UsesMyAgent\\HelloWorld";
constexpr wchar_t kWindowPlacementValueName[] = L"WindowPlacement";
constexpr wchar_t kShortcutIconPath[] = L"%SystemRoot%\\System32\\main.cpl";
constexpr int kShortcutIconIndex = 0;
constexpr int kDisplayControlId = 1001;
constexpr int kClearButtonId = 1100;
constexpr int kBackspaceButtonId = 1101;
constexpr int kDivideButtonId = 1102;
constexpr int kMultiplyButtonId = 1103;
constexpr int kSubtractButtonId = 1104;
constexpr int kAddButtonId = 1105;
constexpr int kEqualsButtonId = 1106;
constexpr int kDecimalButtonId = 1107;
constexpr int kDigitButtonIdBase = 1200;
constexpr int kOuterMargin = 10;
constexpr int kControlGap = 6;
constexpr int kDisplayHeight = 50;
constexpr int kColumnCount = 4;
constexpr int kRowCount = 5;
constexpr int kMinimumButtonWidth = 52;
constexpr int kMinimumButtonHeight = 38;
constexpr int kMinimumClientWidth = (2 * kOuterMargin)
    + (kColumnCount * kMinimumButtonWidth)
    + ((kColumnCount - 1) * kControlGap);
constexpr int kMinimumClientHeight = (2 * kOuterMargin)
    + kDisplayHeight
    + kControlGap
    + (kRowCount * kMinimumButtonHeight)
    + ((kRowCount - 1) * kControlGap);
constexpr size_t kMaximumInputLength = 18;

struct ButtonDefinition
{
    int id;
    const wchar_t* label;
    int column;
    int row;
    int columnSpan;
    int rowSpan;
};

constexpr ButtonDefinition kButtonDefinitions[] =
{
    { kClearButtonId, L"C", 0, 0, 1, 1 },
    { kBackspaceButtonId, L"\u232B", 1, 0, 1, 1 },
    { kDivideButtonId, L"\u00F7", 2, 0, 1, 1 },
    { kMultiplyButtonId, L"\u00D7", 3, 0, 1, 1 },
    { kDigitButtonIdBase + 7, L"7", 0, 1, 1, 1 },
    { kDigitButtonIdBase + 8, L"8", 1, 1, 1, 1 },
    { kDigitButtonIdBase + 9, L"9", 2, 1, 1, 1 },
    { kSubtractButtonId, L"-", 3, 1, 1, 1 },
    { kDigitButtonIdBase + 4, L"4", 0, 2, 1, 1 },
    { kDigitButtonIdBase + 5, L"5", 1, 2, 1, 1 },
    { kDigitButtonIdBase + 6, L"6", 2, 2, 1, 1 },
    { kAddButtonId, L"+", 3, 2, 1, 1 },
    { kDigitButtonIdBase + 1, L"1", 0, 3, 1, 1 },
    { kDigitButtonIdBase + 2, L"2", 1, 3, 1, 1 },
    { kDigitButtonIdBase + 3, L"3", 2, 3, 1, 1 },
    { kEqualsButtonId, L"=", 3, 3, 1, 2 },
    { kDigitButtonIdBase, L"0", 0, 4, 2, 1 },
    { kDecimalButtonId, L".", 2, 4, 1, 1 },
};

struct CalculatorState
{
    HWND display = nullptr;
    std::wstring displayText = L"0";
    double accumulator = 0.0;
    wchar_t pendingOperator = L'\0';
    bool replaceDisplay = false;
    bool hasError = false;
};

CalculatorState* GetCalculatorState(HWND window)
{
    return reinterpret_cast<CalculatorState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
}

void UpdateDisplay(const CalculatorState& state)
{
    if (state.display != nullptr)
    {
        SetWindowTextW(state.display, state.displayText.c_str());
    }
}

void ClearCalculator(CalculatorState& state)
{
    state.displayText = L"0";
    state.accumulator = 0.0;
    state.pendingOperator = L'\0';
    state.replaceDisplay = false;
    state.hasError = false;
}

void ShowCalculationError(CalculatorState& state)
{
    ClearCalculator(state);
    state.displayText = L"Error";
    state.replaceDisplay = true;
    state.hasError = true;
}

bool TryParseDisplay(const CalculatorState& state, double& value)
{
    wchar_t* parseEnd = nullptr;
    errno = 0;
    const double parsedValue = std::wcstod(state.displayText.c_str(), &parseEnd);
    if (parseEnd == state.displayText.c_str()
        || parseEnd == nullptr
        || *parseEnd != L'\0'
        || errno == ERANGE
        || !std::isfinite(parsedValue))
    {
        return false;
    }

    value = parsedValue;
    return true;
}

std::wstring FormatNumber(double value)
{
    if (value == 0.0)
    {
        return L"0";
    }

    std::wostringstream stream;
    stream << std::setprecision(15) << value;
    return stream.str();
}

bool TryApplyOperation(double left, double right, wchar_t operation, double& result)
{
    switch (operation)
    {
    case L'+':
        result = left + right;
        break;

    case L'-':
        result = left - right;
        break;

    case L'*':
        result = left * right;
        break;

    case L'/':
        if (right == 0.0)
        {
            return false;
        }
        result = left / right;
        break;

    default:
        return false;
    }

    return std::isfinite(result);
}

void EnterDigit(CalculatorState& state, wchar_t digit)
{
    if (state.hasError || state.replaceDisplay)
    {
        state.displayText.assign(1, digit);
        state.replaceDisplay = false;
        state.hasError = false;
        return;
    }

    if (state.displayText == L"0")
    {
        if (digit != L'0')
        {
            state.displayText.assign(1, digit);
        }
        return;
    }

    if (state.displayText.length() < kMaximumInputLength)
    {
        state.displayText.push_back(digit);
    }
}

void EnterDecimalPoint(CalculatorState& state)
{
    if (state.hasError || state.replaceDisplay)
    {
        state.displayText = L"0.";
        state.replaceDisplay = false;
        state.hasError = false;
        return;
    }

    if (state.displayText.find(L'.') == std::wstring::npos
        && state.displayText.length() < kMaximumInputLength)
    {
        state.displayText.push_back(L'.');
    }
}

void Backspace(CalculatorState& state)
{
    if (state.hasError)
    {
        ClearCalculator(state);
        return;
    }

    if (state.replaceDisplay)
    {
        return;
    }

    if (state.displayText.length() > 1)
    {
        state.displayText.pop_back();
    }
    else
    {
        state.displayText = L"0";
    }
}

void SelectOperator(CalculatorState& state, wchar_t operation)
{
    if (state.hasError)
    {
        return;
    }

    if (state.pendingOperator != L'\0' && state.replaceDisplay)
    {
        state.pendingOperator = operation;
        return;
    }

    double displayedValue = 0.0;
    if (!TryParseDisplay(state, displayedValue))
    {
        ShowCalculationError(state);
        return;
    }

    if (state.pendingOperator != L'\0')
    {
        double result = 0.0;
        if (!TryApplyOperation(
                state.accumulator,
                displayedValue,
                state.pendingOperator,
                result))
        {
            ShowCalculationError(state);
            return;
        }

        state.accumulator = result;
        state.displayText = FormatNumber(result);
    }
    else
    {
        state.accumulator = displayedValue;
    }

    state.pendingOperator = operation;
    state.replaceDisplay = true;
}

void CalculateResult(CalculatorState& state)
{
    if (state.hasError
        || state.pendingOperator == L'\0'
        || state.replaceDisplay)
    {
        return;
    }

    double displayedValue = 0.0;
    double result = 0.0;
    if (!TryParseDisplay(state, displayedValue)
        || !TryApplyOperation(
            state.accumulator,
            displayedValue,
            state.pendingOperator,
            result))
    {
        ShowCalculationError(state);
        return;
    }

    state.displayText = FormatNumber(result);
    state.accumulator = result;
    state.pendingOperator = L'\0';
    state.replaceDisplay = true;
}

void LayoutCalculatorControls(HWND window, int clientWidth, int clientHeight)
{
    const HWND display = GetDlgItem(window, kDisplayControlId);
    if (display != nullptr)
    {
        MoveWindow(
            display,
            kOuterMargin,
            kOuterMargin,
            clientWidth - (2 * kOuterMargin),
            kDisplayHeight,
            TRUE);
    }

    const int buttonAreaTop = kOuterMargin + kDisplayHeight + kControlGap;
    const int buttonAreaWidth = clientWidth - (2 * kOuterMargin);
    const int buttonAreaHeight = clientHeight - buttonAreaTop - kOuterMargin;
    const int usableButtonWidth = buttonAreaWidth - ((kColumnCount - 1) * kControlGap);
    const int usableButtonHeight = buttonAreaHeight - ((kRowCount - 1) * kControlGap);

    for (const ButtonDefinition& definition : kButtonDefinitions)
    {
        const HWND button = GetDlgItem(window, definition.id);
        if (button == nullptr)
        {
            continue;
        }

        const int left = kOuterMargin
            + ((usableButtonWidth * definition.column) / kColumnCount)
            + (definition.column * kControlGap);
        const int right = kOuterMargin
            + ((usableButtonWidth * (definition.column + definition.columnSpan))
                / kColumnCount)
            + ((definition.column + definition.columnSpan - 1) * kControlGap);
        const int top = buttonAreaTop
            + ((usableButtonHeight * definition.row) / kRowCount)
            + (definition.row * kControlGap);
        const int bottom = buttonAreaTop
            + ((usableButtonHeight * (definition.row + definition.rowSpan)) / kRowCount)
            + ((definition.row + definition.rowSpan - 1) * kControlGap);

        MoveWindow(button, left, top, right - left, bottom - top, TRUE);
    }
}

bool CreateCalculatorControls(HWND window)
{
    CalculatorState* const state = GetCalculatorState(window);
    if (state == nullptr)
    {
        return false;
    }

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    const HWND display = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"STATIC",
        L"0",
        WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE | SS_NOPREFIX,
        0,
        0,
        0,
        0,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDisplayControlId)),
        instance,
        nullptr);
    if (display == nullptr)
    {
        return false;
    }
    state->display = display;
    SendMessageW(display, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    for (const ButtonDefinition& definition : kButtonDefinitions)
    {
        const HWND button = CreateWindowExW(
            0,
            L"BUTTON",
            definition.label,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            0,
            0,
            0,
            0,
            window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(definition.id)),
            instance,
            nullptr);
        if (button == nullptr)
        {
            return false;
        }
        SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    RECT clientArea{};
    GetClientRect(window, &clientArea);
    LayoutCalculatorControls(
        window,
        clientArea.right - clientArea.left,
        clientArea.bottom - clientArea.top);
    return true;
}

void HandleCalculatorCommand(HWND window, int controlId)
{
    CalculatorState* const state = GetCalculatorState(window);
    if (state == nullptr)
    {
        return;
    }

    if (controlId >= kDigitButtonIdBase && controlId <= kDigitButtonIdBase + 9)
    {
        EnterDigit(*state, static_cast<wchar_t>(L'0' + controlId - kDigitButtonIdBase));
    }
    else
    {
        switch (controlId)
        {
        case kClearButtonId:
            ClearCalculator(*state);
            break;

        case kBackspaceButtonId:
            Backspace(*state);
            break;

        case kDivideButtonId:
            SelectOperator(*state, L'/');
            break;

        case kMultiplyButtonId:
            SelectOperator(*state, L'*');
            break;

        case kSubtractButtonId:
            SelectOperator(*state, L'-');
            break;

        case kAddButtonId:
            SelectOperator(*state, L'+');
            break;

        case kEqualsButtonId:
            CalculateResult(*state);
            break;

        case kDecimalButtonId:
            EnterDecimalPoint(*state);
            break;

        default:
            return;
        }
    }

    UpdateDisplay(*state);
}

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
    case WM_NCCREATE:
    {
        const CREATESTRUCTW* const createInformation =
            reinterpret_cast<const CREATESTRUCTW*>(lParam);
        if (createInformation == nullptr || createInformation->lpCreateParams == nullptr)
        {
            return FALSE;
        }

        SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(createInformation->lpCreateParams));
        return DefWindowProcW(window, message, wParam, lParam);
    }

    case WM_CREATE:
        return CreateCalculatorControls(window) ? 0 : -1;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            HandleCalculatorCommand(window, LOWORD(wParam));
        }
        return 0;

    case WM_SIZE:
        LayoutCalculatorControls(window, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* const sizeInformation = reinterpret_cast<MINMAXINFO*>(lParam);
        if (sizeInformation != nullptr)
        {
            RECT minimumWindowBounds{ 0, 0, kMinimumClientWidth, kMinimumClientHeight };
            if (AdjustWindowRectEx(
                    &minimumWindowBounds,
                    WS_OVERLAPPEDWINDOW,
                    FALSE,
                    0)
                != FALSE)
            {
                sizeInformation->ptMinTrackSize.x =
                    minimumWindowBounds.right - minimumWindowBounds.left;
                sizeInformation->ptMinTrackSize.y =
                    minimumWindowBounds.bottom - minimumWindowBounds.top;
            }
        }
        return 0;
    }

    case WM_CLOSE:
        SaveWindowPlacement(window);
        DestroyWindow(window);
        return 0;

    case WM_NCDESTROY:
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        return DefWindowProcW(window, message, wParam, lParam);

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

    constexpr int defaultWindowWidth = 360;
    constexpr int defaultWindowHeight = 500;
    const int defaultPositionX = (GetSystemMetrics(SM_CXSCREEN) - defaultWindowWidth) / 2;
    const int defaultPositionY = (GetSystemMetrics(SM_CYSCREEN) - defaultWindowHeight) / 2;

    WINDOWPLACEMENT savedPlacement{};
    const bool hasSavedPlacement = LoadWindowPlacement(savedPlacement);
    CalculatorState calculatorState{};

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
        &calculatorState);

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
