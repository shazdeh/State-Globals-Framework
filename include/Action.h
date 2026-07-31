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

    struct ModEventAction {
        std::optional<ActionBase> base;
        std::string eventName;
    };

    struct SpellAction {
        ActionBase base;
        TESForm* spell = nullptr;
    };

    struct Rule {
        std::vector<ConsoleAction> consoleActions;
        std::vector<SpellAction> spellActions;
        std::vector<ModEventAction> modEventActions;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    std::optional<ActionBase> ParseActionBase(const nlohmann::json_abi_v3_12_0::json& item) {
        if (item.contains("value")) {
            ActionBase base;
            base.value = item.at("value").get<float>();
            if (item.contains("compare")) {
                base.compare = Utils::ParseCompareOperator(item.at("compare").get<std::string>());
            }
            return base;
        }
        return std::nullopt;
    }

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("actions")) return;
        auto& data = item.at("actions");
        if (!data.is_array()) return;
        Rule actions;
        for (const auto& condition : data) {
            if (!condition.contains("type")) continue;
            auto type = condition.at("type").get<std::string>();
            if (type == "console") {
                if (!condition.contains("command")) continue;
                ConsoleAction action;
                action.command = condition.at("command").get<std::string>();
                action.base = ParseActionBase(condition);
                globals[global].consoleActions.push_back(action);
            } else if (type == "modEvent") {
                if (!condition.contains("eventName")) continue;
                ModEventAction action;
                action.base = ParseActionBase(condition);
                action.eventName = condition.at("eventName").get<std::string>();
                globals[global].modEventActions.push_back(action);
            }
        }
    }
    
    void RunActions(TESGlobal* global, float a_globalValue) {
        auto it = globals.find(global);
        if (it == globals.end()) return;
        
        // console command
        if (!it->second.consoleActions.empty()) {
            for (auto& action : it->second.consoleActions) {
                if (action.base.has_value() &&
                    !Utils::DoCompare(a_globalValue, action.base.value().value, action.base.value().compare))
                    continue;
                
                Utils::ExecuteConsoleCommand(action.command);
            }
        }

        // mod event
        if (!it->second.modEventActions.empty()) {
            for (auto& action : it->second.modEventActions) {
                if (action.base.has_value() &&
                    !Utils::DoCompare(a_globalValue, action.base.value().value, action.base.value().compare))
                    continue;

                SKSE::ModCallbackEvent evt{.eventName = action.eventName, .numArg = a_globalValue};
                SKSE::GetModCallbackEventSource()->SendEvent(&evt);
            }
        }
    }
}