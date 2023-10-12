#pragma once

#include <string>
#include <variant>
#include <optional>

class Settings {
public:
	using Value = std::variant<int>;
	// This enum must contain all the types supported by the Settings::Value.
	// Enum's values must be in the same order and hold the same zero-based index.
	enum class ValueType {
		Int = 0
	};

	// Supported options
	static const std::string ServerPort;
	static const std::string ClientPort;

	virtual ~Settings() {};
	template <typename T>
	std::optional<T> get(const std::string& settingName) const;
protected:
	virtual std::optional<Value> getValue(const std::string& settingName) const = 0;
};

template<typename T>
inline std::optional<T> Settings::get(const std::string& settingName) const {
	auto valueOptional = getValue(settingName);
	if (!valueOptional) {
		return {};
	}
	if (!std::holds_alternative<T>(*valueOptional)) {
		return {};
	}
	return std::optional<T>(std::get<T>(*valueOptional));
}
