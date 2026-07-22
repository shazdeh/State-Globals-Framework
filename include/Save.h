#pragma once

namespace S_Save {
    struct Rule {
        TESGlobal* global;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (item.contains("save")) {
            auto& data = item.at("save");
            Rule rule;
            rule.mod = ParseValueMod(data);
            rule.global = global;
            globals.push_back(rule);
        }
    };

    void Process() {
        for (auto& item : globals) {
            UpdateGlobalValue(item.global, item.mod);
        }
    }
}