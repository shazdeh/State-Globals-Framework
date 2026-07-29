#pragma once

namespace S_Action {
    struct ActionBase {
        float value = 0.0;
        Compare compare = Compare::Equal;
    };

    struct ConsoleAction {
        std::optional<ActionBase> base;
        std::string command;
    };

    struct SpellAction {
        ActionBase base;
        TESForm* spell = nullptr;
    };

    struct Rule {
        std::vector<ConsoleAction> consoleActions;
        std::vector<SpellAction> spellActions;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("actions")) return;
        auto& data = item.at("actions");
        if (!data.is_array()) return;
        Rule actions;
        for (const auto& item : data) {
            if (!item.contains("type")) continue;
            auto type = item.at("type").get<std::string>();
            if (type == "console") {
                if (!item.contains("command")) continue;
                ConsoleAction action;
                action.command = item.at("command").get<std::string>();
                globals[global].consoleActions.push_back(action);
            }
        }
    }
    
    void RunActions(TESGlobal* global, float a_globalValue) {
        auto it = globals.find(global);
        if (it == globals.end()) return;
        
        if (!it->second.consoleActions.empty()) {
            for (auto& action : it->second.consoleActions) {
                if (action.base.has_value() &&
                    !Utils::DoCompare(a_globalValue, action.base.value().value, action.base.value().compare))
                    continue;
                
                Utils::ExecuteConsoleCommand(action.command);
            }
        }
    }
}