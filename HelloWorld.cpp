#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <vsstyle.h>

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "UxTheme.lib")

namespace
{
constexpr wchar_t kWindowClassName[] = L"UsesMyAgentHelloWorldWindow";
constexpr wchar_t kWindowTitle[] = L"Calculator";
constexpr wchar_t kSettingsRegistryPath[] = L"Software\\UsesMyAgent\\HelloWorld";
constexpr wchar_t kWindowPlacementValueName[] = L"WindowPlacement";
constexpr wchar_t kButtonColourValueName[] = L"ButtonColour";
constexpr wchar_t kBackgroundColourValueName[] = L"BackgroundColour";
constexpr wchar_t kOutputColourValueName[] = L"OutputColour";
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
constexpr int kButtonColoursCommandId = 1300;
constexpr int kBackgroundColoursCommandId = 1301;
constexpr int kResetColoursCommandId = 1302;
constexpr int kOutputColoursCommandId = 1303;
constexpr wchar_t kBackspaceSymbol[] = L"\u232B";
constexpr int kOuterMargin = 10;
constexpr int kControlGap = 6;
constexpr int kDisplayHeight = 50;
constexpr int kColumnCount = 4;
constexpr int kRowCount = 5;
constexpr int kMinimumButtonWidth = 52;
constexpr int kMinimumButtonHeight = 38;
constexpr int kControlFontHeightNumerator = 8;
constexpr int kControlFontHeightDenominator = 10;
constexpr int kButtonFontWidthNumerator = 9;
constexpr int kButtonFontWidthDenominator = 10;
constexpr int kDisplayTextHorizontalPadding = 12;
constexpr int kMinimumDisplayFontHeight = 12;
constexpr wchar_t kControlFontFace[] = L"Segoe UI";
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
    { kBackspaceButtonId, kBackspaceSymbol, 1, 0, 1, 1 },
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
    HFONT buttonFont = nullptr;
    HFONT equalsButtonFont = nullptr;
    HFONT displayFont = nullptr;
    int buttonFontHeight = 0;
    int equalsButtonFontHeight = 0;
    int displayFontHeight = 0;
    bool hasButtonColour = false;
    COLORREF buttonColour = 0;
    HBRUSH buttonColourBrush = nullptr;
    bool hasBackgroundColour = false;
    COLORREF backgroundColour = 0;
    HBRUSH backgroundColourBrush = nullptr;
    bool hasOutputColour = false;
    COLORREF outputColour = 0;
    HBRUSH outputColourBrush = nullptr;
    COLORREF customColours[16]{};
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

void UpdateDisplayFont(CalculatorState& state);

void UpdateDisplay(CalculatorState& state)
{
    if (state.display != nullptr)
    {
        SetWindowTextW(state.display, state.displayText.c_str());
        UpdateDisplayFont(state);
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

HFONT CreateBoldFont(int fontHeight, const wchar_t* fontFace)
{
    return CreateFontW(
        -fontHeight,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        fontFace);
}

HFONT CreateBoldControlFont(int fontHeight)
{
    return CreateBoldFont(fontHeight, kControlFontFace);
}

int MeasureTextWidth(HFONT font, const wchar_t* text)
{
    const HDC screenDeviceContext = GetDC(nullptr);
    if (screenDeviceContext == nullptr)
    {
        return 0;
    }

    const HGDIOBJ previousFont = SelectObject(screenDeviceContext, font);
    SIZE textSize{};
    const BOOL measured = GetTextExtentPoint32W(
        screenDeviceContext,
        text,
        lstrlenW(text),
        &textSize);

    if (previousFont != nullptr && previousFont != HGDI_ERROR)
    {
        SelectObject(screenDeviceContext, previousFont);
    }
    ReleaseDC(nullptr, screenDeviceContext);
    return measured != FALSE ? textSize.cx : 0;
}

int CalculateFittedFontHeight(
    const wchar_t* text,
    int targetHeight,
    int maximumWidth,
    int minimumHeight,
    const wchar_t* fontFace)
{
    int fontHeight = targetHeight;
    while (fontHeight > minimumHeight)
    {
        const HFONT measurementFont = CreateBoldFont(fontHeight, fontFace);
        if (measurementFont == nullptr)
        {
            return 0;
        }

        const int textWidth = MeasureTextWidth(measurementFont, text);
        DeleteObject(measurementFont);
        if (textWidth <= 0 || textWidth <= maximumWidth)
        {
            break;
        }

        int fittedHeight = (fontHeight * maximumWidth) / textWidth;
        if (fittedHeight >= fontHeight)
        {
            fittedHeight = fontHeight - 1;
        }
        fontHeight = fittedHeight < minimumHeight ? minimumHeight : fittedHeight;
    }

    return fontHeight;
}

void UpdateDisplayFont(CalculatorState& state)
{
    if (state.display == nullptr)
    {
        return;
    }

    RECT displayArea{};
    RECT displayBounds{};
    if (GetClientRect(state.display, &displayArea) == FALSE
        || GetWindowRect(state.display, &displayBounds) == FALSE)
    {
        return;
    }

    const int displayWidth = displayArea.right - displayArea.left;
    const int displayHeight = displayBounds.bottom - displayBounds.top;
    const int targetHeight = (displayHeight * kControlFontHeightNumerator)
        / kControlFontHeightDenominator;
    const int maximumWidth = displayWidth - (2 * kDisplayTextHorizontalPadding);
    const int fontHeight = CalculateFittedFontHeight(
        state.displayText.c_str(),
        targetHeight,
        maximumWidth,
        kMinimumDisplayFontHeight,
        kControlFontFace);
    if (fontHeight <= 0 || fontHeight == state.displayFontHeight)
    {
        return;
    }

    const HFONT newFont = CreateBoldControlFont(fontHeight);
    if (newFont == nullptr)
    {
        return;
    }

    SendMessageW(
        state.display,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(newFont),
        TRUE);

    if (state.displayFont != nullptr)
    {
        DeleteObject(state.displayFont);
    }
    state.displayFont = newFont;
    state.displayFontHeight = fontHeight;
}

COLORREF GetReadableTextColour(COLORREF backgroundColour)
{
    const int luminance = (299 * GetRValue(backgroundColour))
        + (587 * GetGValue(backgroundColour))
        + (114 * GetBValue(backgroundColour));
    return luminance > 128000 ? RGB(0, 0, 0) : RGB(255, 255, 255);
}

void DrawButtonFocus(const DRAWITEMSTRUCT& drawInformation)
{
    if ((drawInformation.itemState & ODS_FOCUS) != 0
        && (drawInformation.itemState & ODS_NOFOCUSRECT) == 0)
    {
        RECT focusArea = drawInformation.rcItem;
        InflateRect(&focusArea, -4, -4);
        DrawFocusRect(drawInformation.hDC, &focusArea);
    }
}

void DrawButtonBackground(const DRAWITEMSTRUCT& drawInformation, const CalculatorState& state)
{
    RECT buttonArea = drawInformation.rcItem;

    if (state.hasButtonColour)
    {
        HBRUSH fillBrush = state.buttonColourBrush;
        bool ownsFillBrush = false;
        if (fillBrush == nullptr)
        {
            fillBrush = CreateSolidBrush(state.buttonColour);
            ownsFillBrush = true;
        }
        if (fillBrush != nullptr)
        {
            FillRect(drawInformation.hDC, &buttonArea, fillBrush);
            if (ownsFillBrush)
            {
                DeleteObject(fillBrush);
            }
        }

        DrawEdge(
            drawInformation.hDC,
            &buttonArea,
            (drawInformation.itemState & ODS_SELECTED) != 0 ? EDGE_SUNKEN : EDGE_RAISED,
            BF_RECT);
        return;
    }

    int buttonState = PBS_NORMAL;
    if ((drawInformation.itemState & ODS_DISABLED) != 0)
    {
        buttonState = PBS_DISABLED;
    }
    else if ((drawInformation.itemState & ODS_SELECTED) != 0)
    {
        buttonState = PBS_PRESSED;
    }
    else if ((drawInformation.itemState & ODS_HOTLIGHT) != 0)
    {
        buttonState = PBS_HOT;
    }
    else if ((drawInformation.itemState & ODS_DEFAULT) != 0)
    {
        buttonState = PBS_DEFAULTED;
    }

    const HTHEME buttonTheme = OpenThemeData(drawInformation.hwndItem, L"BUTTON");
    if (buttonTheme != nullptr)
    {
        DrawThemeBackground(
            buttonTheme,
            drawInformation.hDC,
            BP_PUSHBUTTON,
            buttonState,
            &buttonArea,
            nullptr);
        CloseThemeData(buttonTheme);
    }
    else
    {
        UINT frameState = DFCS_BUTTONPUSH;
        if ((drawInformation.itemState & ODS_SELECTED) != 0)
        {
            frameState |= DFCS_PUSHED;
        }
        if ((drawInformation.itemState & ODS_DISABLED) != 0)
        {
            frameState |= DFCS_INACTIVE;
        }
        DrawFrameControl(
            drawInformation.hDC,
            &buttonArea,
            DFC_BUTTON,
            frameState);
    }
}

void DrawCenteredBackspaceButton(const DRAWITEMSTRUCT& drawInformation, const CalculatorState& state)
{
    RECT buttonArea = drawInformation.rcItem;
    DrawButtonBackground(drawInformation, state);

    const int buttonWidth = buttonArea.right - buttonArea.left;
    const int buttonHeight = buttonArea.bottom - buttonArea.top;
    const int graphicWidth = (buttonWidth * 7) / 10;
    const int graphicHeight = buttonHeight / 2;
    const int graphicLeft = buttonArea.left + ((buttonWidth - graphicWidth) / 2);
    const int graphicRight = buttonArea.right - ((buttonWidth - graphicWidth) / 2);
    const int graphicTop = buttonArea.top + ((buttonHeight - graphicHeight) / 2);
    const int graphicBottom = buttonArea.bottom - ((buttonHeight - graphicHeight) / 2);
    const int graphicCenterY = (graphicTop + graphicBottom) / 2;
    const int bodyLeft = graphicLeft + ((graphicRight - graphicLeft) / 4);
    const int crossPaddingX = (graphicRight - bodyLeft) / 4;
    const int crossPaddingY = (graphicBottom - graphicTop) / 4;
    const int pressedOffset = (drawInformation.itemState & ODS_SELECTED) != 0 ? 1 : 0;

    POINT outline[] =
    {
        { graphicLeft + pressedOffset, graphicCenterY + pressedOffset },
        { bodyLeft + pressedOffset, graphicTop + pressedOffset },
        { graphicRight + pressedOffset, graphicTop + pressedOffset },
        { graphicRight + pressedOffset, graphicBottom + pressedOffset },
        { bodyLeft + pressedOffset, graphicBottom + pressedOffset },
        { graphicLeft + pressedOffset, graphicCenterY + pressedOffset },
    };

    const bool isDisabled = (drawInformation.itemState & ODS_DISABLED) != 0;
    const COLORREF graphicColor = state.hasButtonColour && !isDisabled
        ? GetReadableTextColour(state.buttonColour)
        : GetSysColor(isDisabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT);
    const int penWidth = buttonHeight / 14 > 1 ? buttonHeight / 14 : 2;
    const HPEN graphicPen = CreatePen(PS_SOLID, penWidth, graphicColor);
    if (graphicPen != nullptr)
    {
        const HGDIOBJ previousPen = SelectObject(drawInformation.hDC, graphicPen);
        Polyline(
            drawInformation.hDC,
            outline,
            static_cast<int>(sizeof(outline) / sizeof(outline[0])));

        MoveToEx(
            drawInformation.hDC,
            bodyLeft + crossPaddingX + pressedOffset,
            graphicTop + crossPaddingY + pressedOffset,
            nullptr);
        LineTo(
            drawInformation.hDC,
            graphicRight - crossPaddingX + pressedOffset,
            graphicBottom - crossPaddingY + pressedOffset);
        MoveToEx(
            drawInformation.hDC,
            graphicRight - crossPaddingX + pressedOffset,
            graphicTop + crossPaddingY + pressedOffset,
            nullptr);
        LineTo(
            drawInformation.hDC,
            bodyLeft + crossPaddingX + pressedOffset,
            graphicBottom - crossPaddingY + pressedOffset);

        if (previousPen != nullptr && previousPen != HGDI_ERROR)
        {
            SelectObject(drawInformation.hDC, previousPen);
        }
        DeleteObject(graphicPen);
    }

    DrawButtonFocus(drawInformation);
}

void DrawTextButton(const DRAWITEMSTRUCT& drawInformation, const CalculatorState& state)
{
    RECT buttonArea = drawInformation.rcItem;
    DrawButtonBackground(drawInformation, state);

    wchar_t label[32]{};
    GetWindowTextW(
        drawInformation.hwndItem,
        label,
        static_cast<int>(sizeof(label) / sizeof(label[0])));

    const HFONT buttonFont = reinterpret_cast<HFONT>(
        SendMessageW(drawInformation.hwndItem, WM_GETFONT, 0, 0));
    const HGDIOBJ previousFont = buttonFont != nullptr
        ? SelectObject(drawInformation.hDC, buttonFont)
        : nullptr;

    const bool isDisabled = (drawInformation.itemState & ODS_DISABLED) != 0;
    const COLORREF textColour = state.hasButtonColour && !isDisabled
        ? GetReadableTextColour(state.buttonColour)
        : GetSysColor(isDisabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT);

    SetBkMode(drawInformation.hDC, TRANSPARENT);
    const COLORREF previousTextColour = SetTextColor(drawInformation.hDC, textColour);

    RECT textArea = buttonArea;
    if ((drawInformation.itemState & ODS_SELECTED) != 0)
    {
        OffsetRect(&textArea, 1, 1);
    }
    DrawTextW(
        drawInformation.hDC,
        label,
        -1,
        &textArea,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (previousTextColour != CLR_INVALID)
    {
        SetTextColor(drawInformation.hDC, previousTextColour);
    }
    if (previousFont != nullptr && previousFont != HGDI_ERROR)
    {
        SelectObject(drawInformation.hDC, previousFont);
    }

    DrawButtonFocus(drawInformation);
}

void UpdateButtonFonts(
    HWND window,
    CalculatorState& state,
    int buttonWidth,
    int buttonHeight,
    int equalsButtonWidth,
    int equalsButtonHeight)
{
    const int maximumButtonTextWidth = (buttonWidth * kButtonFontWidthNumerator)
        / kButtonFontWidthDenominator;
    const int targetButtonFontHeight = (buttonHeight * kControlFontHeightNumerator)
        / kControlFontHeightDenominator;
    const int buttonFontHeight = CalculateFittedFontHeight(
        L"\u00D7",
        targetButtonFontHeight,
        maximumButtonTextWidth,
        1,
        kControlFontFace);
    if (buttonFontHeight > 0 && buttonFontHeight != state.buttonFontHeight)
    {
        const HFONT newFont = CreateBoldControlFont(buttonFontHeight);
        if (newFont != nullptr)
        {
            for (const ButtonDefinition& definition : kButtonDefinitions)
            {
                if (definition.id == kBackspaceButtonId
                    || definition.id == kEqualsButtonId)
                {
                    continue;
                }

                const HWND button = GetDlgItem(window, definition.id);
                if (button != nullptr)
                {
                    SendMessageW(
                        button,
                        WM_SETFONT,
                        reinterpret_cast<WPARAM>(newFont),
                        TRUE);
                }
            }

            if (state.buttonFont != nullptr)
            {
                DeleteObject(state.buttonFont);
            }
            state.buttonFont = newFont;
            state.buttonFontHeight = buttonFontHeight;
        }
    }

    const int maximumEqualsTextWidth =
        (equalsButtonWidth * kButtonFontWidthNumerator) / kButtonFontWidthDenominator;
    const int targetEqualsFontHeight =
        (equalsButtonHeight * kControlFontHeightNumerator) / kControlFontHeightDenominator;
    const int equalsFontHeight = CalculateFittedFontHeight(
        L"=",
        targetEqualsFontHeight,
        maximumEqualsTextWidth,
        1,
        kControlFontFace);
    if (equalsFontHeight <= 0 || equalsFontHeight == state.equalsButtonFontHeight)
    {
        return;
    }

    const HFONT newEqualsFont = CreateBoldControlFont(equalsFontHeight);
    if (newEqualsFont == nullptr)
    {
        return;
    }

    const HWND equalsButton = GetDlgItem(window, kEqualsButtonId);
    if (equalsButton != nullptr)
    {
        SendMessageW(
            equalsButton,
            WM_SETFONT,
            reinterpret_cast<WPARAM>(newEqualsFont),
            TRUE);
    }

    if (state.equalsButtonFont != nullptr)
    {
        DeleteObject(state.equalsButtonFont);
    }
    state.equalsButtonFont = newEqualsFont;
    state.equalsButtonFontHeight = equalsFontHeight;
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

    CalculatorState* const state = GetCalculatorState(window);
    if (state != nullptr)
    {
        const int buttonWidth = usableButtonWidth / kColumnCount;
        const int buttonHeight = usableButtonHeight / kRowCount;
        const int equalsButtonWidth = buttonWidth;
        const int equalsButtonHeight = (2 * buttonHeight) + kControlGap;
        UpdateDisplayFont(*state);
        UpdateButtonFonts(
            window,
            *state,
            buttonWidth,
            buttonHeight,
            equalsButtonWidth,
            equalsButtonHeight);
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
        const DWORD buttonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW;
        const HWND button = CreateWindowExW(
            0,
            L"BUTTON",
            definition.label,
            buttonStyle,
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

HMENU CreateSettingsMenuBar()
{
    const HMENU settingsMenu = CreatePopupMenu();
    if (settingsMenu == nullptr)
    {
        return nullptr;
    }

    if (AppendMenuW(settingsMenu, MF_STRING, kButtonColoursCommandId, L"&Button Colours") == FALSE
        || AppendMenuW(settingsMenu, MF_STRING, kBackgroundColoursCommandId, L"Back&ground Colours") == FALSE
        || AppendMenuW(settingsMenu, MF_STRING, kOutputColoursCommandId, L"&Output Colours") == FALSE
        || AppendMenuW(settingsMenu, MF_SEPARATOR, 0, nullptr) == FALSE
        || AppendMenuW(settingsMenu, MF_STRING, kResetColoursCommandId, L"&Reset Colours") == FALSE)
    {
        DestroyMenu(settingsMenu);
        return nullptr;
    }

    const HMENU menuBar = CreateMenu();
    if (menuBar == nullptr)
    {
        DestroyMenu(settingsMenu);
        return nullptr;
    }

    if (AppendMenuW(
            menuBar,
            MF_POPUP,
            reinterpret_cast<UINT_PTR>(settingsMenu),
            L"&Settings")
        == FALSE)
    {
        DestroyMenu(settingsMenu);
        DestroyMenu(menuBar);
        return nullptr;
    }

    return menuBar;
}

bool LoadStoredColour(const wchar_t* valueName, COLORREF& colour)
{
    DWORD storedColour = 0;
    DWORD valueType = 0;
    DWORD valueSize = static_cast<DWORD>(sizeof(storedColour));
    const LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        kSettingsRegistryPath,
        valueName,
        RRF_RT_REG_DWORD,
        &valueType,
        &storedColour,
        &valueSize);

    if (status != ERROR_SUCCESS
        || valueType != REG_DWORD
        || valueSize != sizeof(storedColour))
    {
        return false;
    }

    colour = static_cast<COLORREF>(storedColour);
    return true;
}

void SaveStoredColour(const wchar_t* valueName, COLORREF colour)
{
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

    const DWORD storedColour = static_cast<DWORD>(colour);
    RegSetValueExW(
        settingsKey,
        valueName,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&storedColour),
        static_cast<DWORD>(sizeof(storedColour)));
    RegCloseKey(settingsKey);
}

void DeleteStoredColour(const wchar_t* valueName)
{
    HKEY settingsKey = nullptr;
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            kSettingsRegistryPath,
            0,
            KEY_SET_VALUE,
            &settingsKey)
        != ERROR_SUCCESS)
    {
        return;
    }

    RegDeleteValueW(settingsKey, valueName);
    RegCloseKey(settingsKey);
}

void ChooseButtonColour(HWND window, CalculatorState& state)
{
    CHOOSECOLORW options{};
    options.lStructSize = sizeof(options);
    options.hwndOwner = window;
    options.rgbResult = state.hasButtonColour ? state.buttonColour : GetSysColor(COLOR_BTNFACE);
    options.lpCustColors = state.customColours;
    options.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (ChooseColorW(&options) == FALSE)
    {
        return;
    }

    state.buttonColour = options.rgbResult;
    state.hasButtonColour = true;

    const HBRUSH newBrush = CreateSolidBrush(state.buttonColour);
    if (newBrush != nullptr)
    {
        if (state.buttonColourBrush != nullptr)
        {
            DeleteObject(state.buttonColourBrush);
        }
        state.buttonColourBrush = newBrush;
    }

    SaveStoredColour(kButtonColourValueName, state.buttonColour);

    for (const ButtonDefinition& definition : kButtonDefinitions)
    {
        const HWND button = GetDlgItem(window, definition.id);
        if (button != nullptr)
        {
            InvalidateRect(button, nullptr, TRUE);
        }
    }
}

void RevertButtonColour(HWND window, CalculatorState& state)
{
    state.hasButtonColour = false;
    state.buttonColour = 0;
    if (state.buttonColourBrush != nullptr)
    {
        DeleteObject(state.buttonColourBrush);
        state.buttonColourBrush = nullptr;
    }

    DeleteStoredColour(kButtonColourValueName);

    for (const ButtonDefinition& definition : kButtonDefinitions)
    {
        const HWND button = GetDlgItem(window, definition.id);
        if (button != nullptr)
        {
            InvalidateRect(button, nullptr, TRUE);
        }
    }
}

void ChooseBackgroundColour(HWND window, CalculatorState& state)
{
    CHOOSECOLORW options{};
    options.lStructSize = sizeof(options);
    options.hwndOwner = window;
    options.rgbResult = state.hasBackgroundColour
        ? state.backgroundColour
        : GetSysColor(COLOR_WINDOW);
    options.lpCustColors = state.customColours;
    options.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (ChooseColorW(&options) == FALSE)
    {
        return;
    }

    state.backgroundColour = options.rgbResult;
    state.hasBackgroundColour = true;

    const HBRUSH newBrush = CreateSolidBrush(state.backgroundColour);
    if (newBrush != nullptr)
    {
        if (state.backgroundColourBrush != nullptr)
        {
            DeleteObject(state.backgroundColourBrush);
        }
        state.backgroundColourBrush = newBrush;
    }

    SaveStoredColour(kBackgroundColourValueName, state.backgroundColour);

    InvalidateRect(window, nullptr, TRUE);
}

void RevertBackgroundColour(HWND window, CalculatorState& state)
{
    state.hasBackgroundColour = false;
    state.backgroundColour = 0;
    if (state.backgroundColourBrush != nullptr)
    {
        DeleteObject(state.backgroundColourBrush);
        state.backgroundColourBrush = nullptr;
    }

    DeleteStoredColour(kBackgroundColourValueName);

    InvalidateRect(window, nullptr, TRUE);
}

void ChooseOutputColour(HWND window, CalculatorState& state)
{
    CHOOSECOLORW options{};
    options.lStructSize = sizeof(options);
    options.hwndOwner = window;
    options.rgbResult = state.hasOutputColour
        ? state.outputColour
        : GetSysColor(COLOR_3DFACE);
    options.lpCustColors = state.customColours;
    options.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (ChooseColorW(&options) == FALSE)
    {
        return;
    }

    state.outputColour = options.rgbResult;
    state.hasOutputColour = true;

    const HBRUSH newBrush = CreateSolidBrush(state.outputColour);
    if (newBrush != nullptr)
    {
        if (state.outputColourBrush != nullptr)
        {
            DeleteObject(state.outputColourBrush);
        }
        state.outputColourBrush = newBrush;
    }

    SaveStoredColour(kOutputColourValueName, state.outputColour);

    if (state.display != nullptr)
    {
        InvalidateRect(state.display, nullptr, TRUE);
    }
}

void RevertOutputColour(CalculatorState& state)
{
    state.hasOutputColour = false;
    state.outputColour = 0;
    if (state.outputColourBrush != nullptr)
    {
        DeleteObject(state.outputColourBrush);
        state.outputColourBrush = nullptr;
    }

    DeleteStoredColour(kOutputColourValueName);

    if (state.display != nullptr)
    {
        InvalidateRect(state.display, nullptr, TRUE);
    }
}

void HandleSettingsCommand(HWND window, int commandId)
{
    CalculatorState* const state = GetCalculatorState(window);
    if (state == nullptr)
    {
        return;
    }

    switch (commandId)
    {
    case kButtonColoursCommandId:
        ChooseButtonColour(window, *state);
        break;

    case kBackgroundColoursCommandId:
        ChooseBackgroundColour(window, *state);
        break;

    case kOutputColoursCommandId:
        ChooseOutputColour(window, *state);
        break;

    case kResetColoursCommandId:
        RevertButtonColour(window, *state);
        RevertBackgroundColour(window, *state);
        RevertOutputColour(*state);
        break;

    default:
        break;
    }
}

bool IsCalculatorButtonId(int controlId)
{
    return (controlId >= kClearButtonId && controlId <= kDecimalButtonId)
        || (controlId >= kDigitButtonIdBase && controlId <= kDigitButtonIdBase + 9);
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
        if (lParam == 0 && HIWORD(wParam) == 0)
        {
            HandleSettingsCommand(window, LOWORD(wParam));
        }
        else if (lParam != 0 && HIWORD(wParam) == BN_CLICKED)
        {
            HandleCalculatorCommand(window, LOWORD(wParam));
        }
        return 0;

    case WM_DRAWITEM:
    {
        const DRAWITEMSTRUCT* const drawInformation =
            reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        CalculatorState* const state = GetCalculatorState(window);
        if (drawInformation == nullptr || state == nullptr)
        {
            return FALSE;
        }

        if (wParam == kBackspaceButtonId)
        {
            DrawCenteredBackspaceButton(*drawInformation, *state);
            return TRUE;
        }
        if (IsCalculatorButtonId(static_cast<int>(wParam)))
        {
            DrawTextButton(*drawInformation, *state);
            return TRUE;
        }
        return FALSE;
    }

    case WM_ERASEBKGND:
    {
        CalculatorState* const state = GetCalculatorState(window);
        if (state == nullptr || !state->hasBackgroundColour)
        {
            return DefWindowProcW(window, message, wParam, lParam);
        }

        RECT clientArea{};
        if (GetClientRect(window, &clientArea) != FALSE)
        {
            HBRUSH fillBrush = state->backgroundColourBrush;
            bool ownsFillBrush = false;
            if (fillBrush == nullptr)
            {
                fillBrush = CreateSolidBrush(state->backgroundColour);
                ownsFillBrush = true;
            }
            if (fillBrush != nullptr)
            {
                FillRect(reinterpret_cast<HDC>(wParam), &clientArea, fillBrush);
                if (ownsFillBrush)
                {
                    DeleteObject(fillBrush);
                }
            }
        }
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    {
        CalculatorState* const state = GetCalculatorState(window);
        if (state == nullptr
            || !state->hasOutputColour
            || reinterpret_cast<HWND>(lParam) != state->display)
        {
            return DefWindowProcW(window, message, wParam, lParam);
        }

        if (state->outputColourBrush == nullptr)
        {
            state->outputColourBrush = CreateSolidBrush(state->outputColour);
        }
        if (state->outputColourBrush == nullptr)
        {
            return DefWindowProcW(window, message, wParam, lParam);
        }

        const HDC displayDeviceContext = reinterpret_cast<HDC>(wParam);
        SetTextColor(displayDeviceContext, GetReadableTextColour(state->outputColour));
        SetBkColor(displayDeviceContext, state->outputColour);
        return reinterpret_cast<LRESULT>(state->outputColourBrush);
    }

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
                    TRUE,
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
    {
        CalculatorState* const state = GetCalculatorState(window);
        if (state != nullptr)
        {
            if (state->buttonFont != nullptr)
            {
                DeleteObject(state->buttonFont);
                state->buttonFont = nullptr;
            }
            if (state->equalsButtonFont != nullptr)
            {
                DeleteObject(state->equalsButtonFont);
                state->equalsButtonFont = nullptr;
            }
            if (state->displayFont != nullptr)
            {
                DeleteObject(state->displayFont);
                state->displayFont = nullptr;
            }
            if (state->buttonColourBrush != nullptr)
            {
                DeleteObject(state->buttonColourBrush);
                state->buttonColourBrush = nullptr;
            }
            if (state->backgroundColourBrush != nullptr)
            {
                DeleteObject(state->backgroundColourBrush);
                state->backgroundColourBrush = nullptr;
            }
            if (state->outputColourBrush != nullptr)
            {
                DeleteObject(state->outputColourBrush);
                state->outputColourBrush = nullptr;
            }
        }
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        return DefWindowProcW(window, message, wParam, lParam);
    }

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
    for (COLORREF& customColour : calculatorState.customColours)
    {
        customColour = RGB(255, 255, 255);
    }

    COLORREF storedButtonColour = 0;
    if (LoadStoredColour(kButtonColourValueName, storedButtonColour))
    {
        calculatorState.buttonColour = storedButtonColour;
        calculatorState.hasButtonColour = true;
        calculatorState.buttonColourBrush = CreateSolidBrush(storedButtonColour);
    }

    COLORREF storedBackgroundColour = 0;
    if (LoadStoredColour(kBackgroundColourValueName, storedBackgroundColour))
    {
        calculatorState.backgroundColour = storedBackgroundColour;
        calculatorState.hasBackgroundColour = true;
        calculatorState.backgroundColourBrush = CreateSolidBrush(storedBackgroundColour);
    }

    COLORREF storedOutputColour = 0;
    if (LoadStoredColour(kOutputColourValueName, storedOutputColour))
    {
        calculatorState.outputColour = storedOutputColour;
        calculatorState.hasOutputColour = true;
        calculatorState.outputColourBrush = CreateSolidBrush(storedOutputColour);
    }

    const HMENU menuBar = CreateSettingsMenuBar();
    if (menuBar == nullptr)
    {
        UnregisterClassW(kWindowClassName, instance);
        DestroyWindowIcons(icons);
        return 1;
    }

    const HWND window = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        defaultPositionX,
        defaultPositionY,
        defaultWindowWidth,
        defaultWindowHeight,
        nullptr,
        menuBar,
        instance,
        &calculatorState);

    if (window == nullptr)
    {
        DestroyMenu(menuBar);
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
