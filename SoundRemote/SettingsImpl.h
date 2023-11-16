#pragma once

#include <optional>
#include <regex>
#include <string>
#include <unordered_map>

#include "Settings.h"

class SettingsImpl : public Settings {
public:
	SettingsImpl();
	void addSetting(const std::string& name, Value defautlValue);
	void setFile(const std::string& fileName);
protected:
	std::optional<Value> getValue(const std::string& settingName) const override;
private:
	using SettingsMap = std::unordered_map<std::string, Value>;

	SettingsMap settings_;
	SettingsMap defaults_;
	const std::regex section_{ "^\\[.*\\]$" };
	const std::regex property_{ "^\\s*(.+?)\\s*=\\s*(.+?)\\s*$" };

	bool parseLine(const std::string& line, const SettingsMap& defaults, SettingsMap& settings);
	void writeToFile(const std::string& fileName, const SettingsMap& defaults, const SettingsMap& settings);
};
