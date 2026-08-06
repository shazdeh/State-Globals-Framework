#pragma once

namespace S_Action {
    struct ActionBase {
        float value = 0.0;
        Compare compare = Compare::Equal;
    };

    struct ConsoleAction {
        std::optional<ActionBase> base;
        std::string command;
        std::optional<ConditionFilter> condition;
    };

    struct ModEventAction {
        std::optional<ActionBase> base;
        std::string eventName;
        std::optional<ConditionFilter> condition;
    };

    struct SpellAction {
        std::optional<ActionBase> base;
        SpellItem* spell = nullptr;
        std::optional<ConditionFilter> condition;
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
        for (const auto& actionRule : data) {
            if (!actionRule.contains("type")) continue;
            auto type = actionRule.at("type").get<std::string_view>();
            if (type == "console"sv) {
                if (!actionRule.contains("command")) continue;
                ConsoleAction action;
                action.command = actionRule.at("command").get<std::string>();
                action.base = ParseActionBase(actionRule);
                if (actionRule.contains("condition")) {
                    action.condition = ParseConditionFilter(actionRule.at("condition"));
                    if (action.condition == std::nullopt) continue;
                }
                globals[global].consoleActions.push_back(action);
            } else if (type == "modEvent"sv) {
                if (!actionRule.contains("eventName")) continue;
                ModEventAction action;
                action.base = ParseActionBase(actionRule);
                action.eventName = actionRule.at("eventName").get<std::string>();
                if (actionRule.contains("condition")) {
                    action.condition = ParseConditionFilter(actionRule.at("condition"));
                    if (action.condition == std::nullopt) continue;
                }
                globals[global].modEventActions.push_back(action);
            } else if (type == "spell"sv) {
                if (!actionRule.contains("spell")) continue;
                SpellAction action;
                action.base = ParseActionBase(actionRule);
                action.spell = Utils::GetForm<SpellItem>(actionRule.at("spell").get<std::string>());
                if (!action.spell) continue;
                if (actionRule.contains("condition")) {
                    action.condition = ParseConditionFilter(actionRule.at("condition"));
                    if (action.condition == std::nullopt) continue;
                }
                globals[global].spellActions.push_back(action);
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
                if (action.condition.has_value() && !ValidateConditionForm(action.condition.value())) continue;
                
                Utils::ExecuteConsoleCommand(action.command);
            }
        }

        // mod event
        if (!it->second.modEventActions.empty()) {
            for (auto& action : it->second.modEventActions) {
                if (action.base.has_value() &&
                    !Utils::DoCompare(a_globalValue, action.base.value().value, action.base.value().compare))
                    continue;
                if (action.condition.has_value() && !ValidateConditionForm(action.condition.value())) continue;

                SKSE::ModCallbackEvent evt{.eventName = action.eventName, .numArg = a_globalValue};
                SKSE::GetModCallbackEventSource()->SendEvent(&evt);
            }
        }

        if (!it->second.spellActions.empty()) {
            for (auto& action : it->second.spellActions) {
                if (action.base.has_value() &&
                    !Utils::DoCompare(a_globalValue, action.base.value().value, action.base.value().compare))
                    continue;
                if (action.condition.has_value() && !ValidateConditionForm(action.condition.value())) continue;

                if (auto player = PlayerCharacter::GetSingleton(); player) {
                    auto type = action.spell->GetSpellType();
                    if (type == MagicSystem::SpellType::kAbility || type == MagicSystem::SpellType::kDisease) {
                        player->AddSpell(action.spell);
                    } else {
                        player->GetMagicCaster(MagicSystem::CastingSource::kInstant)
                            ->CastSpellImmediate(action.spell, false, nullptr, 1.0f, false, 0.0f, nullptr);
                    }
                }
            }
        }
    }
}