#include <sstream>
#include <stdexcept>

#include <Windows.h>

#include "Util.h"

void* Util::mainWindow_ = nullptr;

void Util::setMainWindow(void* mainWindowHWND) {
    mainWindow_ = mainWindowHWND;
}

void Util::showError(const std::string& text) {
    MessageBoxA(reinterpret_cast<HWND>(mainWindow_), text.c_str(), "Error", MB_ICONERROR | MB_OK);
}

void Util::showInfo(const std::wstring& text, const std::wstring& caption) {
    MessageBoxW(reinterpret_cast<HWND>(mainWindow_), text.c_str(), caption.c_str(), MB_ICONINFORMATION | MB_OK);
}

std::string Util::contructAppExceptionText(const std::string& where, const std::string& error) {
    return where + ": " + error;
}

std::string Util::makeFatalErrorText(ErrorCode ec) {
    std::ostringstream ss;
    ss << "Fatal error: " << static_cast<int>(ec);
    return ss.str();
}
