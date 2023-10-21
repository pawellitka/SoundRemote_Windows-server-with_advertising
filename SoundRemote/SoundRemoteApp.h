#pragma once

#include <memory>
#include <atomic>

#include "resource.h"

class SoundRemoteApp {
public:
	SoundRemoteApp(_In_ HINSTANCE hInstance);
	~SoundRemoteApp();
	static std::unique_ptr<SoundRemoteApp> create(_In_ HINSTANCE hInstance);
	int exec(int nCmdShow);
private:
	static const int WINDOW_WIDTH = 300;            // main window width
	static const int WINDOW_HEIGHT = 300;			// main window height
	static const int TIMER_ID_PEAK_METER = 1;
	static const int TIMER_PERIOD_PEAK_METER = 33;	// in milliseconds

	HINSTANCE hInst_ = nullptr;						// current instance
	// Strings
	std::wstring sTitle_;							// the title bar text
	std::wstring sWindowClass_;						// the main window class name
	std::wstring sServerAddresses_;					// server addresses description
	std::wstring sDefaultRenderDevice_;				// default render device description
	std::wstring sDefaultCaptureDevice_;			// default capture device description
	std::wstring sClients_;							// clients label
	std::wstring sKeystrokes_;						// keystrokes label
	std::wstring muteButtonText;					// mute button text
	// Controls
	HWND mainWindow_ = nullptr;
	HWND deviceComboBox_ = nullptr;
	HWND clientsList_ = nullptr;
	HWND addressButton_ = nullptr;
	HWND peakMeterProgress_ = nullptr;
	HWND keystrokes_ = nullptr;
	std::unique_ptr<MuteButton> muteButton_;
	// Data
	std::wstring currentDeviceId_;
	std::unordered_map<int, std::wstring> deviceIds_;	// <Number stored as data in CombBox items, Device id string>
	// Utility
	boost::asio::io_context ioContext_;
	std::unique_ptr<std::thread> ioContextThread_;
	std::shared_ptr<Server> server_;
	std::unique_ptr<CapturePipe> capturePipe_;
	std::shared_ptr<Settings> settings_;

	void initStrings();
	ATOM registerClass();
	bool initInstance(int nCmdShow);
	// UI related
	void initInterface(HWND hWndParent);
	void initControls();
	void startPeakMeter();
	void stopPeakMeter();
	/// <summary>
	/// Adds devices for passed <code>EDataFlow</code>, including items for default devices.
	/// </summary>
	/// <param name="comboBox">ComboBox to add to.</param>
	/// <param name="flow">Can be eRender, eCapture or eAll</param>
	void addDevices(HWND comboBox, EDataFlow flow);
	/// <summary>
	/// Adds default device for EDataFlow::eRender or EDataFlow::eCapture.
	/// </summary>
	/// <param name="comboBox">ComboBox to add to.</param>
	/// <param name="flow">Must be EDataFlow::eRender or EDataFlow::eCapture.</param>
	void addDefaultDevice(HWND comboBox, EDataFlow flow);
	std::wstring getDeviceId(const int deviceIndex) const;
	long getCharHeight(HWND hWnd) const;

	// Description:
	//   Creates a tooltip for a control.
	// Parameters:
	//   toolWindow - window handle of the control to add the tooltip to.
	//   text - string to use as the tooltip text.
	//   parentWindow - parent window handle.
	// Returns:
	//   The handle to the tooltip.
	HWND setTooltip(HWND toolWindow, PTSTR text, HWND parentWindow);
	std::wstring loadStringResource(UINT resourceId);
	void initSettings();

	// Event handlers
	void onDeviceSelect();
	void onClientListUpdate(std::forward_list<std::string> clients);
	void onAddressButtonClick() const;
	void updatePeakMeter();
	void onReceiveKeystroke(const Keystroke& keystroke);

	/// <summary>
	/// Starts server and audio processing.
	/// </summary>
	void run();
	void shutdown();
	void changeCaptureDevice(const std::wstring& deviceId);
	void stopCapture();
	void asioEventLoop(boost::asio::io_context& ctx);

	static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT wndProc(UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK about(HWND, UINT, WPARAM, LPARAM);
};
