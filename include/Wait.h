#pragma once

namespace S_Wait {
    int passedHours = 0;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        ValueMod mod{};
        std::optional<int> minHours;
        std::optional<int> maxHours;
        bool modByHoursWaited = true;
    };

    std::vector<Rule> globals;

    void Process(int hoursWaited) {
        for (auto& item : globals) {
            if (item.minHours.has_value() && hoursWaited < item.minHours.value()) continue;
            if (item.maxHours.has_value() && hoursWaited > item.maxHours.value()) continue;
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;

            UpdateGlobalValue(item.global, item.mod, item.modByHoursWaited ? hoursWaited : 1.0f);
        }
    }

    int GetPassedHours() { return static_cast<int>(RE::Calendar::GetSingleton()->GetHoursPassed()); }

    class EventSink : public BSTEventSink<TESWaitStartEvent>, public BSTEventSink<TESWaitStopEvent> {
        BSEventNotifyControl ProcessEvent(const TESWaitStartEvent*, BSTEventSource<TESWaitStartEvent>*) {
            passedHours = GetPassedHours();
            return BSEventNotifyControl::kContinue;
        }

        BSEventNotifyControl ProcessEvent(const TESWaitStopEvent*, BSTEventSource<TESWaitStopEvent>*) {
            int hoursWaited = GetPassedHours() - passedHours;
            Process(hoursWaited);
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("wait")) return;
        auto& data = item.at("wait");
        Rule rule;
        rule.mod = ParseValueMod(data);

        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }
        if (data.contains("minHours")) {
            rule.minHours = data.at("minHours").get<int>();
        }
        if (data.contains("maxHours")) {
            rule.maxHours = data.at("maxHours").get<int>();
        }
        if (data.contains("modByHoursWaited")) {
            rule.modByHoursWaited = data.at("modByHoursWaited").get<bool>();
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESWaitStartEvent>(&g_sink);
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESWaitStopEvent>(&g_sink);
        }
    }
}