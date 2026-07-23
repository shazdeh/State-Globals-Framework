#pragma once

namespace S_SoulTrap {
    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    void Process(Actor* victim) {
        for (auto& item : globals) {
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(victim, item.formFilter.value())) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<SoulsTrapped::Event> {
        BSEventNotifyControl ProcessEvent(const SoulsTrapped::Event* event, BSTEventSource<SoulsTrapped::Event>*) {
            if (event->trapper->IsPlayerRef()) {
                Process(event->target);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("soulTrap")) return;
        auto& data = item.at("soulTrap");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            SoulsTrapped::GetEventSource()->AddEventSink<SoulsTrapped::Event>(&g_sink);
        }
    }
}