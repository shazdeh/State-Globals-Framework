#pragma once

namespace S_Furniture {
    TESObjectREFR* lastUsedFurniture = nullptr;

    struct Rule {
        TESGlobal* global;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
    };

    std::vector<Rule> enterGlobals;
    std::vector<Rule> exitGlobals;

    void Process(std::vector<Rule>& arr) {
        for (auto& item : arr) {
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() &&
                !ValidateFormFilter(lastUsedFurniture->GetBaseObject(), item.formFilter.value()))
                continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<TESFurnitureEvent> {
        BSEventNotifyControl ProcessEvent(const TESFurnitureEvent* event, BSTEventSource<TESFurnitureEvent>*) {
            if (!event->actor || !event->actor->IsPlayerRef() || !event->targetFurniture || !event->targetFurniture.get()) return BSEventNotifyControl::kContinue;
            lastUsedFurniture = event->targetFurniture.get();
            if (event->type == TESFurnitureEvent::FurnitureEventType::kEnter) {
                Process(enterGlobals);
            } else {
                Process(exitGlobals);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        for (std::string_view key : {"furnitureEnter"sv, "furnitureExit"sv}) {
            if (!item.contains(key)) return;
            auto& data = item.at(key);
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
            (key == "furnitureEnter"sv ? enterGlobals : exitGlobals).push_back(rule);
        }
    }

    void SetupEvents() {
        if (!enterGlobals.empty() || !exitGlobals.empty()) {
            static EventSink furnitureSink;
            ScriptEventSourceHolder().GetSingleton()->AddEventSink(&furnitureSink);
        }
    }

    TESForm* GetLastUsedFurniture(StaticFunctionTag*) { return lastUsedFurniture; }

    void OnLoadGame() { lastUsedFurniture = nullptr;
    }
}