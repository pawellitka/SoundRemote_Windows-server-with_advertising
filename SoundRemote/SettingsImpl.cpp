#include <fstream>
#include <array>
#include <sstream>

#include "Util.h"
#include "SettingsImpl.h"

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );
    return s;
}

SettingsImpl::SettingsImpl() {
}

std::optional<Settings::Value> SettingsImpl::getValue(const std::string& settingName) const {
    if (settings_.contains(settingName)) {
        return settings_.at(settingName);
    }
    if (defaults_.contains(settingName)) {
        return defaults_.at(settingName);
    }
    return {};
}

void SettingsImpl::addSetting(const std::string& name, Settings::Value defautlValue) {
    defaults_[name] = defautlValue;
}

void SettingsImpl::setFile(const std::string& fileName) {
    settings_.clear();

    std::fstream fs{ fileName, std::ios::in };
    if (fs.is_open()) {
        bool parseSuccess = true;
        // Read until an eof/error encountered or all settings are read.
        while (fs.good() && parseSuccess && (settings_.size() < defaults_.size())) {
            std::string lineStr;
            std::getline(fs, lineStr);
            if (fs.good() || fs.eof()) {
                parseSuccess = parseLine(lineStr, defaults_, settings_);
            }
        }
        const bool readSuccessful = (fs.good() || fs.eof()) && (settings_.size() == defaults_.size());
        if (readSuccessful) {
            return;
        }
    }
    writeToFile(fileName, defaults_, settings_);
}

bool SettingsImpl::parseLine(const std::string& line, const SettingsMap& defaults, SettingsMap& settings) {
    // Blank
    if (line.empty()) {
        return true;
    }
    // Comment
    if (line.at(0) == ';') {
        return true;
    }
    // Section
    if (std::regex_match(line, section_)) {
        return true;
    }
    // Property
    std::smatch match;
    if (std::regex_search(line, match, property_)) {
        // Settings' names are stored in lowercase.
        const std::string propName = toLower(match[1]);
        const std::string propValue = match[2];
        if (!defaults.contains(propName)) {
            return false;
        }

        const auto valueTypeIndex = defaults.at(propName).index();
        Value settingValue;
        try {
            switch (valueTypeIndex) {
            case static_cast<int>(ValueType::Int):
                settingValue = std::stoi(propValue);
                break;
            default:
                return false;
            }
        }
        catch (...) {
            return false;
        }
        settings[propName] = settingValue;
        return true;
    }
    // Failed to parse
    return false;
}

void SettingsImpl::writeToFile(const std::string& fileName, const SettingsMap& defaults, const SettingsMap& settings) {
    std::fstream fs{ fileName, std::ios::out };
    if (!fs.is_open()) {
        //todo: print full path
        Util::showError("Can't create settings file.");
        return;
    }
    for (auto&& p : defaults) {
        const auto name = p.first;
        Value value;
        if (settings.contains(name)) {
            value = settings.at(name);
        } else {
            value = p.second;
        }
        std::string valueStr;
        switch (value.index()) {
        case static_cast<int>(ValueType::Int):
            valueStr = std::to_string(std::get<int>(value));
            break;
        default:
            break;
        }
        fs << name << '=' << valueStr << std::endl;
    }
}
