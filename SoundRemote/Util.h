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

    /// <summary>
    /// Shows an error message box with the given text.
    /// </summary>
    /// <param name="text">- message to show</param>
    static void showError(const std::string& text);

    /// <summary>
    /// Shows an informational message box with the given text and caption.
    /// </summary>
    /// <param name="text">- message to show</param>
    /// <param name="caption">- message box caption</param>
    static void showInfo(const std::wstring& text, const std::wstring& caption);

    /// <summary>
    /// Makes a text describing the origin of an error and the error itself.
    /// </summary>
    /// <param name="where">- error origin</param>
    /// <param name="what">- error explanation</param>
    /// <returns>error message</returns>
    static std::string makeAppErrorText(const std::string& where, const std::string& what);

    /// <summary>
    /// Makes a text for a fatal error with <c>ErrorCode</c>.
    /// </summary>
    /// <param name="ec">error code</param>
    /// <returns>error message</returns>
    static std::string makeFatalErrorText(ErrorCode ec);

private:
    static void* mainWindow_;
};

//template <typename T, auto fn>
//struct Deleter {
//	void operator()(T* ptr) {
//		fn(ptr);
//	}
//};
