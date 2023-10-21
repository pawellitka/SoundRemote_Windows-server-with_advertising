#include <windows.h>
#include <CommCtrl.h>
#include <functional>

#include "Controls.h"
#include "resource.h"

MuteButton::MuteButton(HWND hParent, const Rect& rect) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	button_ = CreateWindowW(WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_ICON | BS_FLAT | BS_AUTOCHECKBOX | BS_PUSHLIKE,
		rect.x, rect.y, rect.w, rect.h, hParent, NULL, hInst, NULL);
	iconSoundOn_ = LoadImage(hInst, MAKEINTRESOURCE(IDI_SOUND_ON), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	iconSoundOff_ = LoadImage(hInst, MAKEINTRESOURCE(IDI_SOUND_OFF), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	updateIcon(false);
}

void MuteButton::onClick() {
	bool muted = isMuted();
	updateIcon(muted);
	if (stateCallback_) {
		stateCallback_(muted);
	}
}

HWND MuteButton::handle() const {
	return button_;
}

bool MuteButton::isMuted() const {
	return SendMessage(button_, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void MuteButton::setStateCallback(std::function<void(bool)> callback) {
	stateCallback_ = callback;
}

void MuteButton::updateIcon(bool muted) {
	HANDLE iconHandle;
	if (muted) {
		iconHandle = iconSoundOff_;
	} else {
		iconHandle = iconSoundOn_;
	}
	SendMessage(button_, BM_SETIMAGE, IMAGE_ICON, (LPARAM)iconHandle);
}
