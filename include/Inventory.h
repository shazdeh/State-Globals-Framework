#pragma once

namespace S_Inventory {
    bool bQueued;
    FormID playerID = 0x14;

    enum ValueType {
        Count = 0,
        Weight = 1,
        GoldValue = 2
    };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        std::optional<bool> isEnchanted;
        std::optional<bool> isStolen;
        ValueType valueType = ValueType::Count;
        ValueMod mod{};
        bool unique = false;
        bool onlyPlayable = true;
    };

    std::vector<Rule> globals;

    void Process() {
        auto inventory = PlayerCharacter::GetSingleton()->GetInventory();
        for (auto& item : globals) {
            float value = 0.0f;
            for (auto& [inventoryItem, data] : inventory) {
                if (inventoryItem->Is(RE::FormType::LeveledItem)) continue;
                auto count = data.first;
                auto* inv = data.second.get();
                if (count == 0 || !inv) continue; // why does InventoryItemMap contain items with 0 count?
                if (item.onlyPlayable && !inventoryItem->GetPlayable()) continue;
                if (item.formFilter.has_value() && !ValidateFormFilter(inventoryItem, item.formFilter.value())) continue;
                if (item.isEnchanted.has_value() && inv->IsEnchanted() != item.isEnchanted.value())
                    continue;
                if (item.isStolen.has_value() && !!inv->GetOwner() != item.isStolen.value())
                    continue;

                count = item.unique ? 1 : count;
                if (item.valueType == ValueType::Weight) {
                    value += inv->GetWeight() * count;
                } else if (item.valueType == ValueType::GoldValue) {
                    value += inv->GetObject()->GetGoldValue() * count;
                } else {
                    value += count;
                }
            }
            UpdateGlobalValue(item.global, item.mod, value, true);
        }
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESContainerChangedEvent> {
        BSEventNotifyControl ProcessEvent(const TESContainerChangedEvent* event,
                                          BSTEventSource<TESContainerChangedEvent>*) {
            if (bQueued || !event || !(event->oldContainer == playerID || event->newContainer == playerID))
                return BSEventNotifyControl::kContinue;
            SKSE::GetTaskInterface()->AddTask(Process);
            bQueued = true;
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("inventory")) return;
        auto& data = item.at("inventory");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("unique")) {
            rule.unique = data.at("unique").get<bool>();
        }
        if (data.contains("enchanted")) {
            rule.isEnchanted = data.at("enchanted").get<bool>();
        }
        if (data.contains("stolen")) {
            rule.isStolen = data.at("stolen").get<bool>();
        }
        if (data.contains("onlyPlayable")) {
            rule.onlyPlayable = data.at("onlyPlayable").get<bool>();
        }
        if (data.contains("valueType")) {
            std::string_view valueType = data.at("valueType").get<std::string_view>();
            if (valueType == "weight"sv) {
                rule.valueType = ValueType::Weight;
            } else if (valueType == "value"sv) {
                rule.valueType = ValueType::GoldValue;
            }
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESContainerChangedEvent>(&g_sink);
        }
    }

    void OnLoadGame() { Process();
    }
}