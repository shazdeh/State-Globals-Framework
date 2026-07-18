#pragma once

namespace S_Location {

    struct Rule {
        TESGlobal* global;
        std::optional<FormFilter> formFilter;
        bool discover = true;
        bool clear = true;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    void Process(bool bCleared) {
        auto playerLocation = PlayerCharacter::GetSingleton()->GetCurrentLocation();
        for (auto& item : globals) {
            if ((bCleared && !item.clear) || (!bCleared && !item.discover)) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(playerLocation, item.formFilter.value())) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<LocationDiscovery::Event>, public BSTEventSink<LocationCleared::Event> {
        BSEventNotifyControl ProcessEvent(const LocationDiscovery::Event* event,
                                          BSTEventSource<LocationDiscovery::Event>*) {
            Process(false);
            return BSEventNotifyControl::kContinue;
        }

        BSEventNotifyControl ProcessEvent(const LocationCleared::Event* event,
                                          BSTEventSource<LocationCleared::Event>*) {
            Process(true);
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("location")) return;
        auto& data = item.at("location");
        Rule rule;
        if (data.contains("mod")) {
            rule.mod = ParseValueMod(item);
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
