#pragma once

#include <set>
#include <string>

class Keystroke {
public:
    /// <summary>
    /// Creates Keystroke.
    /// </summary>
    /// <param name="key">A virtual-key code. The code must be a value in the range 1 to 254.</param>
    /// <param name="mods">Bit field of <c>Keystroke::ModKey</c> values.</param>
    Keystroke(int key, int mods);

    /// <summary>
    /// Emulate keystroke.
    /// </summary>
    void emulate() const;

    std::wstring toString() const;

private:
    enum class ModKey {
        Win = 1,
        Ctrl = 1 << 1,
        Shift = 1 << 2,
        Alt = 1 << 3,
    };
    enum class ModKeyVk {
        Win = 0x5B,
        Ctrl = 0x11,
        Shift = 0x10,
        Alt = 0x12
    };

    const int key_;
	std::set<ModKeyVk> mods_;

    std::wstring getVkCodeDescription(int vkCode) const;
    //Keystroke() = delete;
    //Keystroke(const Keystroke&) = delete;
    //Keystroke& operator= (const Keystroke&) = delete;
};
