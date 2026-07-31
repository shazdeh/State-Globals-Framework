#pragma once

namespace S_Ini {
    struct Rule {
        TESGlobal* global;
        std::string section;
        std::string key;
    };

    std::unordered_map <std::string, std::vector<Rule>> globals;

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (item.contains("ini")) {
            auto& data = item.at("ini");
            if (!data.contains("file") || !data.contains("section") || !data.contains("key")) return;
            Rule rule;
            rule.section = data.at("section").get<std::string>();
            rule.key = data.at("key").get<std::string>();
            auto file = data.at("file").get<std::string>();
            rule.global = global;
            globals[file].push_back(rule);
        }
    };

    void OnLoadGame() {
        for (auto& [file, rules] : globals) {
            CSimpleIniA ini;
            if (ini.LoadFile(file.c_str()) != SI_OK) continue;
            for (auto& item : rules) {
                const char* str = ini.GetValue(item.section.c_str(), item.key.c_str());
                item.global->value = str ? std::strtof(str, nullptr) : 0.0f;
            }
        }
    }
}