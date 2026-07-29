#pragma once

namespace S_Location {

    struct Rule {
        TESGlobal* global;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
        bool discover = true;
        bool clear = true;
    };

    std::vector<Rule> globals;

    void Process(bool bCleared) {
        auto playerLocation = PlayerCharacter::GetSingleton()->GetCurrentLocation();
        for (auto& item : globals) {
            if ((bCleared && !item.clear) || (!bCleared && !item.discover)) continue;
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(playerLocation, item.formFilter.value())) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<LocationDiscovery::Event>, public BSTEventSink<LocationCleared::Event> {
        BSEventNotifyControl ProcessEvent(const LocationDiscovery::Event*,
                                          BSTEventSource<LocationDiscovery::Event>*) {
            Process(false);
            return BSEventNotifyControl::kContinue;
        }

        BSEventNotifyControl ProcessEvent(const LocationCleared::Event*,
                                          BSTEventSource<LocationCleared::Event>*) {
            Process(true);
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("location")) return;
        auto& data = item.at("location");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("discover")) {
            rule.discover = data.at("discover").get<bool>();
        }
        if (data.contains("clear")) {
            rule.clear = data.at("clear").get<bool>();
        }

        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink theSink;
            LocationDiscovery::GetEventSource()->AddEventSink<LocationDiscovery::Event>(&theSink);
            LocationCleared::GetEventSource()->AddEventSink<LocationCleared::Event>(&theSink);
        }
    }
}
