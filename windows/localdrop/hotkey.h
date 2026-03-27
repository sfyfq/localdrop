#pragma once
#include <functional>

enum class HotkeyAction {
	COPY, // Ctrl + Shift + Alt + C 
	PASTE // Ctrl + Shift + Alt + V
};

using HotkeyCallback = std::function<void(HotkeyAction)>;

void StartHook(HotkeyCallback cb);
void StopHook();