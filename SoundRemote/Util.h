#pragma once

#include <string>

enum class ErrorCode {
    REMOVE_CAPTURE_PIPE_CLIENTS_LISTENER = 1,
};

struct Util {
    enum class Endian {
        Big,
        Little
    };

    static void setMainWindow(void* mainWindowHWND);
    static void showError(const std::string& text);
    static void showInfo(const std::wstring& text, const std::wstring& caption);
    static std::string contructAppExceptionText(const std::string& where, const std::string& error);
    static std::string makeFatalErrorText(ErrorCode error);
private:
    static void* mainWindow_;
};

//template <typename T, auto fn>
//struct Deleter {
//	void operator()(T* ptr) {
//		fn(ptr);
//	}
//};
