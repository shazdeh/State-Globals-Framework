#pragma once

namespace S_LevelUp {
    struct Rule {
        TESGlobal* global;
        std::optional<ConditionFilter> condition;
        std::unordered_set<uint16_t> levels;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    void Process(uint16_t newLevel) {
        for (auto& item : globals) {
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (!item.levels.empty() && !item.levels.contains(newLevel)) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<LevelIncrease::Event> {
        BSEventNotifyControl ProcessEvent(const LevelIncrease::Event* event, BSTEventSource<LevelIncrease::Event>*) {
            Process(event->newLevel);
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("craft")) return;
        auto& data = item.at("craft");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }
        if (data.contains("levels")) {
            Utils::FillSet(data.at("levels"), rule.levels);
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink levelSink;
            LevelIncrease::GetEventSource()->AddEventSink(&levelSink);
        }
    }
}