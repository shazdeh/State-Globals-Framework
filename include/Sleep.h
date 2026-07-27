#pragma once

namespace S_Sleep {
    int passedHours = 0;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        ValueMod mod{};
        std::optional<int> minHours;
        std::optional<int> maxHours;
        bool modByHoursSlept = true;
    };

    std::vector<Rule> globals;

    void Process(int hoursSlept) {
        for (auto& item : globals) {
            if (item.minHours.has_value() && hoursSlept < item.minHours.value()) continue;
            if (item.maxHours.has_value() && hoursSlept > item.maxHours.value()) continue;
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;

            UpdateGlobalValue(item.global, item.mod, item.modByHoursSlept ? hoursSlept : 1);
        }
    }

    int GetPassedHours() {
        return static_cast<int>(RE::Calendar::GetSingleton()->GetHoursPassed());
    }

    class EventSink : public BSTEventSink<TESSleepStartEvent>, public BSTEventSink<TESSleepStopEvent> {
        BSEventNotifyControl ProcessEvent(const TESSleepStartEvent* event, BSTEventSource<TESSleepStartEvent>*) {
            passedHours = GetPassedHours();
            return BSEventNotifyControl::kContinue;
        }

        BSEventNotifyControl ProcessEvent(const TESSleepStopEvent* event, BSTEventSource<TESSleepStopEvent>*) {
            int hoursSlept = GetPassedHours() - passedHours;
            Process(hoursSlept);
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("sleep")) return;
        auto& data = item.at("sleep");
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
        if (data.contains("modByHoursSlept")) {
            rule.modByHoursSlept = data.at("modByHoursSlept").get<bool>();
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESSleepStartEvent>(&g_sink);
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESSleepStopEvent>(&g_sink);
        }
    }
}