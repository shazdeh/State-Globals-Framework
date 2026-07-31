#pragma once

namespace S_ModEvent {

    struct Rule {
        TESGlobal* global;
        std::optional<ConditionFilter> condition;
        BSFixedString eventName;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    class EventSink : public BSTEventSink<SKSE::ModCallbackEvent> {
        BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* event, BSTEventSource<SKSE::ModCallbackEvent>*) {
            if (!event) return BSEventNotifyControl::kContinue;
            if (bLogIDs) ConsoleLog::GetSingleton()->Print(fmt::format("ModEvent: {}", event->eventName).c_str());
            for (auto& item : globals) {
                if (item.eventName != event->eventName) continue;
                if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;

                UpdateGlobalValue(item.global, item.mod);
            }

            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("modEvent")) return;
        auto& data = item.at("modEvent");
        if (!data.contains("eventName")) return;
        Rule rule;
        rule.mod = ParseValueMod(data);
        rule.eventName = data.at("eventName").get<std::string>();
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }
        
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink theSink;
            SKSE::GetModCallbackEventSource()->AddEventSink(&theSink);
        }
    }
}
