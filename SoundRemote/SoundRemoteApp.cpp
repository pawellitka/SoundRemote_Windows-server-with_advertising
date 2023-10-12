// sound_remote_app.cpp : Defines the entry point for the application.
//

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <boost/asio.hpp>

#include <windows.h>
#include <CommCtrl.h>
#include <Windowsx.h>

#include "framework.h"

#include "Util.h"
#include "CapturePipe.h"
#include "AudioUtil.h"
#include "NetUtil.h"
#include "Server.h"
#include "SettingsImpl.h"

#include "SoundRemoteApp.h"

using namespace std::placeholders;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    std::unique_ptr<SoundRemoteApp> app = SoundRemoteApp::create(_In_ hInstance);
    return app ? app->exec(nCmdShow) : 1;
}

SoundRemoteApp::SoundRemoteApp(_In_ HINSTANCE hInstance): hInst_(hInstance), ioContext_() {}

SoundRemoteApp::~SoundRemoteApp() {
    boost::asio::post(ioContext_, std::bind(&SoundRemoteApp::shutdown, this));

    if (ioContextThread_ && ioContextThread_->joinable()) {
        ioContextThread_->join();
    }
}

std::unique_ptr<SoundRemoteApp> SoundRemoteApp::create(_In_ HINSTANCE hInstance){
    return std::make_unique<SoundRemoteApp>(hInstance);
}

int SoundRemoteApp::exec(int nCmdShow) {
    initStrings();
    registerClass();

    // Application initialization:
    if (!initInstance(nCmdShow)) {
        return 1;
    }

    run();

    HACCEL hAccelTable = LoadAccelerators(hInst_, MAKEINTRESOURCE(IDC_SOUNDREMOTE));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(mainWindow_, &msg) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

void SoundRemoteApp::run() {
    Util::setMainWindow(mainWindow_);
    initSettings();
    try {
        server_ = std::make_shared<Server>(ioContext_, settings_);
        server_->setClientListCallback(std::bind(&SoundRemoteApp::onClientListUpdate, this, _1));
        server_->setKeystrokeCallback(std::bind(&SoundRemoteApp::onReceiveKeystroke, this, _1));
        // io_context will run as long as the server works and waiting for incoming packets.
        ioContextThread_ = std::make_unique<std::thread>(std::bind(&SoundRemoteApp::asioEventLoop, this, _1), std::ref(ioContext_));
    }
    catch (const std::exception& e) {
        Util::showError(e.what());
        std::exit(EXIT_FAILURE);
    }
    catch (...) {
        Util::showError("Start server: unknown error");
        std::exit(EXIT_FAILURE);
    }
    // Create audio source by selecting a device.
    onDeviceSelect();
}

void SoundRemoteApp::shutdown() {
    stopCapture();
    ioContext_.stop();
}

void SoundRemoteApp::addDevices(HWND comboBox, EDataFlow flow) {
    const auto devices = Audio::getEndpointDevices(flow);
    if (devices.empty()) {
        return;
    }
    if (flow == eRender || flow == eAll) {                      // Add default playback
        addDefaultDevice(comboBox, eRender);
    }
    if (flow == eCapture || flow == eAll) {                     // Add default recording
        addDefaultDevice(comboBox, eCapture);
    }
    for (auto&& iter = devices.cbegin(); iter != devices.end(); ++iter) {
        const auto index = ComboBox_AddString(deviceComboBox_, iter->first.c_str());
        ComboBox_SetItemData(deviceComboBox_, index, index);
        deviceIds_[index] = iter->second;
    }
}

void SoundRemoteApp::addDefaultDevice(HWND comboBox, EDataFlow flow) {
    assert(flow == eRender || flow == eCapture);

    int newItemIndex, deviceId;
    if (flow == eRender) {
        newItemIndex = ComboBox_AddString(comboBox, sDefaultRenderDevice_.data());
        deviceId = Audio::DEFAULT_RENDER_DEVICE_ID;
    } else {
        newItemIndex = ComboBox_AddString(comboBox, sDefaultCaptureDevice_.data());
        deviceId = Audio::DEFAULT_CAPTURE_DEVICE_ID;
    }
    ComboBox_SetItemData(comboBox, newItemIndex, (LPARAM)deviceId);
}

std::wstring SoundRemoteApp::getDeviceId(const int deviceIndex) const {
    if (deviceIds_.find(deviceIndex) == deviceIds_.end()) {
        assert(deviceIndex == Audio::DEFAULT_CAPTURE_DEVICE_ID || deviceIndex == Audio::DEFAULT_RENDER_DEVICE_ID);
        EDataFlow flow = (deviceIndex == Audio::DEFAULT_CAPTURE_DEVICE_ID) ? eCapture : eRender;
        return Audio::getDefaultDevice(flow);
    }
    return deviceIds_.at(deviceIndex);
}

long SoundRemoteApp::getCharHeight(HWND hWnd) const {
    HDC hdc = GetDC(hWnd);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hWnd, hdc);
    return tm.tmHeight;
}

HWND SoundRemoteApp::setTooltip(HWND toolWindow, PTSTR text, HWND parentWindow) {
    HWND tooltip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentWindow, NULL, hInst_, NULL);

    // Must explicitly define a tooltip control as topmost
    SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Associate the tooltip with the tool.
    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = parentWindow;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = reinterpret_cast<UINT_PTR>(toolWindow);
    toolInfo.lpszText = text;
    SendMessage(tooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));

    return tooltip;
}

std::wstring SoundRemoteApp::loadStringResource(UINT resourceId) {
    const WCHAR* unterminatedString;
    const auto stringLength = LoadStringW(hInst_, resourceId, (LPWSTR)&unterminatedString, 0);
    return { unterminatedString, static_cast<size_t>(stringLength) };
}

void SoundRemoteApp::onDeviceSelect() {
// Get selected item's deviceIndex
    const auto itemIndex = ComboBox_GetCurSel(deviceComboBox_);
    if (CB_ERR == itemIndex) { return; }
    const auto itemData = ComboBox_GetItemData(deviceComboBox_, itemIndex);
    const int deviceIndex = static_cast<int>(itemData);

// Get device id string
    const std::wstring deviceId = getDeviceId(deviceIndex);

// Change capture device
    boost::asio::post(ioContext_, std::bind(&SoundRemoteApp::changeCaptureDevice, this, deviceId));

    startPeakMeter();
}

void SoundRemoteApp::changeCaptureDevice(const std::wstring& deviceId) {
    if (currentDeviceId_ == deviceId) {
        return;
    }
    currentDeviceId_.clear();
    stopCapture();
    capturePipe_ = std::make_unique<CapturePipe>(deviceId, server_, ioContext_);
    currentDeviceId_ = deviceId;
    capturePipe_->start();
}

void SoundRemoteApp::stopCapture() {
    if (capturePipe_) {
        capturePipe_.reset();
    }
}

void SoundRemoteApp::onClientListUpdate(std::forward_list<std::string> clients) {
    std::ostringstream addresses;
    for (const auto& client : clients) {
        addresses << client << "\r\n";
    }
    SetWindowTextA(clientsList_, addresses.str().c_str());
}

void SoundRemoteApp::onAddressButtonClick() const {
    auto addresses = Net::getLocalAddresses();
    std::wstring addressesStr;
    for (auto adr: addresses) {
        addressesStr += adr + L"\n";
    }
    Util::showInfo(addressesStr, sServerAddresses_);
}

void SoundRemoteApp::updatePeakMeter() {
    if (capturePipe_) {
        auto peakValue = capturePipe_->getPeakValue();
        const int peak = static_cast<int>(peakValue * 100);
        SendMessage(peakMeterProgress_, PBM_SETPOS, peak, 0);
    } else {
        stopPeakMeter();
    }
}

void SoundRemoteApp::onReceiveKeystroke(const Keystroke& keystroke) {
    auto currentTextLength = Edit_GetTextLength(keystrokes_);
    Edit_SetSel(keystrokes_, currentTextLength, currentTextLength);
    tm now;
    const auto t = time(nullptr);
    localtime_s(&now, &t);
    wchar_t timeStr[20];
    wcsftime(timeStr, sizeof(timeStr), L"%T%t", &now);
    std::wstring keystrokeDesc = timeStr + keystroke.toString() + L"\r\n";
    Edit_ReplaceSel(keystrokes_, keystrokeDesc.c_str());
}

void SoundRemoteApp::asioEventLoop(boost::asio::io_context& ctx) {
    for (;;) {
        try {
            ctx.run();
            break;
        }
        catch (const Audio::Error& e) {
            Util::showError(e.what());
            stopCapture();
        }
        catch (const std::exception& e) {
            //logger.log(LOG_ERR) << "[eventloop] An unexpected error occurred running " << name << " task: " << e.what();
            Util::showError(e.what());
            std::exit(EXIT_FAILURE);
        }
        catch (...) {
            Util::showError("Event loop: unknown error");
            std::exit(EXIT_FAILURE);
        }
    }
}

void SoundRemoteApp::initInterface(HWND hWndParent) {
    RECT wndRect;
    GetClientRect(hWndParent, &wndRect);
    const int windowW = wndRect.right;
    const int windowH = wndRect.bottom;

    auto const charH = getCharHeight(hWndParent);
    constexpr int padding = 5;
    constexpr int rightBlockW = 30;
    const int leftBlockW = windowW - rightBlockW - padding * 3;

// Device combobox
    const int deviceComboX = padding;
    const int deviceComboY = padding;
    const int deviceComboW = windowW - padding * 2;
    const int deviceComboH = 100;
    deviceComboBox_ = CreateWindowW(WC_COMBOBOX, (LPCWSTR)NULL, CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP,
        deviceComboX, deviceComboY, deviceComboW, deviceComboH, hWndParent, NULL, hInst_, NULL);

    RECT deviceComboRect;
    GetClientRect(deviceComboBox_, &deviceComboRect);
    MapWindowPoints(deviceComboBox_, hWndParent, (LPPOINT)&deviceComboRect, 2);
    
// Clients label
    const int clientsLabelX = padding;
    const int clientsLabelY = deviceComboRect.bottom + padding;
    const int clientsLabelW = leftBlockW;
    const int clientsLabelH = charH;
    HWND clientsLabel = CreateWindow(WC_STATIC, sClients_.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
        clientsLabelX, clientsLabelY, clientsLabelW, clientsLabelH, hWndParent, NULL, hInst_, NULL);

// Clients
    const int clientListX = padding;
    const int clientListY = clientsLabelY + clientsLabelH + padding;
    const int clientListW = leftBlockW;
    const int clientListH = 60;
    clientsList_ = CreateWindow(WC_EDIT, (LPCWSTR)NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY,
        clientListX, clientListY, clientListW, clientListH, hWndParent, NULL, hInst_, NULL);

// Keystrokes label
    const int keystrokesLabelX = padding;
    const int keystrokesLabelY = clientListY + clientListH + padding;
    const int keystrokesLabelW = leftBlockW;
    const int keystrokesLabelH = charH;
    HWND keystrokesLabel = CreateWindow(WC_STATIC, sKeystrokes_.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
        keystrokesLabelX, keystrokesLabelY, keystrokesLabelW, keystrokesLabelH, hWndParent, NULL, hInst_, NULL);

// Keystrokes
    const int keystrokesX = padding;
    const int keystrokesY = keystrokesLabelY + keystrokesLabelH + padding;
    const int keystrokesW = leftBlockW;
    const int keystrokesH = windowH - keystrokesY - padding;
    keystrokes_ = CreateWindow(WC_EDIT, (LPCWSTR)NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY,
        keystrokesX, keystrokesY, keystrokesW, keystrokesH, hWndParent, NULL, hInst_, NULL);

// Address button
    const int addressButtonX = windowW - rightBlockW - padding;
    const int addressButtonY = deviceComboRect.bottom + padding;
    const int addressButtonW = rightBlockW;
    const int addressButtonH = rightBlockW;
    addressButton_ = CreateWindowW(WC_BUTTON, L"IP", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_FLAT,
        addressButtonX, addressButtonY, addressButtonW, addressButtonH, hWndParent, NULL, hInst_, NULL);
    setTooltip(addressButton_, sServerAddresses_.data(), hWndParent);

// Peak meter
    const int peakMeterX = addressButtonX;
    const int peakMeterY = addressButtonY + addressButtonH + padding;
    const int peakMeterW = rightBlockW;
    const int peakMeterH = windowH - peakMeterY - padding;
    peakMeterProgress_ = CreateWindowW(PROGRESS_CLASS, (LPCWSTR)NULL, WS_CHILD | WS_VISIBLE | PBS_VERTICAL | PBS_SMOOTH,
        peakMeterX, peakMeterY, peakMeterW, peakMeterH, hWndParent, NULL, hInst_, NULL);
}

void SoundRemoteApp::initControls() {
    //Device ComboBox
    ComboBox_ResetContent(deviceComboBox_);
    deviceIds_.clear();
    addDevices(deviceComboBox_, eRender);
    addDevices(deviceComboBox_, eCapture);
    ComboBox_SetCurSel(deviceComboBox_, 0);
}

void SoundRemoteApp::startPeakMeter() {
    SetTimer(mainWindow_, TIMER_ID_PEAK_METER, TIMER_PERIOD_PEAK_METER, nullptr);
}

void SoundRemoteApp::stopPeakMeter() {
    KillTimer(mainWindow_, TIMER_ID_PEAK_METER);
    SendMessage(peakMeterProgress_, PBM_SETPOS, 0, 0);
}

void SoundRemoteApp::initSettings() {
    auto settings = std::make_shared<SettingsImpl>();
    settings->addSetting(Settings::ServerPort, Net::DEFAULT_PORT_IN);
    settings->addSetting(Settings::ClientPort, Net::DEFAULT_PORT_OUT);
    settings->setFile("settings.ini");
    settings_ = settings;
}

void SoundRemoteApp::initStrings() {
    sTitle_ = loadStringResource(IDS_APP_TITLE);
    sWindowClass_ = loadStringResource(IDC_SOUNDREMOTE);
    sServerAddresses_ = loadStringResource(IDS_SERVER_ADDRESSES);
    sDefaultRenderDevice_ = loadStringResource(IDS_DEFAULT_RENDER);
    sDefaultCaptureDevice_ = loadStringResource(IDS_DEFAULT_CAPTURE);
    sClients_ = loadStringResource(IDS_CLIENTS);
    sKeystrokes_ = loadStringResource(IDS_KEYSTROKES);
}

//
//  FUNCTION: registerClass()
//
//  PURPOSE: Registers the window class.
//
ATOM SoundRemoteApp::registerClass() {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = staticWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst_;
    wcex.hIcon = LoadIcon(hInst_, MAKEINTRESOURCE(IDI_SOUNDREMOTE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SOUNDREMOTE);
    wcex.lpszClassName = sWindowClass_.data();
    wcex.hIconSm = nullptr;

    return RegisterClassExW(&wcex);
}

bool SoundRemoteApp::initInstance(int nCmdShow) {
    mainWindow_ = CreateWindowW(sWindowClass_.data(), sTitle_.data(), WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInst_, this);

    if (!mainWindow_) {
        return false;
    }

    initInterface(mainWindow_);
    initControls();

    ShowWindow(mainWindow_, nCmdShow);
    UpdateWindow(mainWindow_);

    return true;
}

INT_PTR SoundRemoteApp::about(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT SoundRemoteApp::staticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    SoundRemoteApp* app = nullptr;
    if (message == WM_CREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        app = static_cast<SoundRemoteApp*>(lpcs->lpCreateParams);
        app->mainWindow_ = hWnd;
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<SoundRemoteApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    if (app) {
        return app->wndProc(message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT SoundRemoteApp::wndProc(UINT message, WPARAM wParam, LPARAM lParam) {
    bool wasHandled = true;
    LRESULT result = 0;
    switch (message)
    {
    case WM_COMMAND:
    {
        const int wmType = HIWORD(wParam);
        const int wmId = LOWORD(wParam);
        if (lParam == 0) {  // If Menu or Accelerator
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), mainWindow_, about);
                break;
            case IDM_EXIT:
                DestroyWindow(mainWindow_);
                break;
            default:
                wasHandled = false;
                break;
            }
        } else {    // If Control
            switch (wmType)
            {
            case CBN_SELCHANGE: {
                // The only combobox is device select.
                onDeviceSelect();
            }
            break;
            case BN_CLICKED: {
                if (addressButton_ == reinterpret_cast<HWND>(lParam)) {
                    onAddressButtonClick();
                } else {
                    wasHandled = false;
                }
            }
            break;
            default:    // Other controls
                wasHandled = false;
                break;
            }
        }
    }
    break;
    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE) {
            DestroyWindow(mainWindow_);
        } else {
            wasHandled = false;
        }
    break;
    // Ignore WM_CLOSE because multiline edit sends it on Esc.
    case WM_CLOSE:
    break;
    case WM_TIMER:
    {
        switch ((int)wParam) {
        case TIMER_ID_PEAK_METER:
            updatePeakMeter();
            break;
        default:
            wasHandled = false;
            break;
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(mainWindow_, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(mainWindow_, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        wasHandled = false;
        break;
    }

    if (!wasHandled) {
        result = DefWindowProc(mainWindow_, message, wParam, lParam);
    }
    return result;
}
