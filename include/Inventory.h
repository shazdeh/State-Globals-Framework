#pragma once

namespace S_Inventory {
    bool bQueued;
    FormID playerID = 0x14;

    enum ValueType {
        Count = 0,
        Weight = 1
    };

    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* formFilter = nullptr;
        TESForm* formExcludeFilter = nullptr;
        std::unordered_set<int> formTypes;
        std::optional<bool> isEnchanted;
        std::optional<bool> isStolen;
        ValueType valueType = ValueType::Count;
        bool unique = false;
    };

    std::vector<Rule> globals;

    void Process() {
        auto inventory = PlayerCharacter::GetSingleton()->GetInventory();
        for (auto& item : globals) {
            float value = 0.0f;
            for (auto& [inventoryItem, data] : inventory) {
                if (!item.formTypes.empty() &&
                    !item.formTypes.contains(std::to_underlying(inventoryItem->GetFormType())))
                    continue;
                if (item.formExcludeFilter && Utils::ParseFormFilter(inventoryItem, item.formExcludeFilter)) continue;
                if (item.formFilter && !Utils::ParseFormFilter(inventoryItem, item.formFilter)) continue;
                if (item.isEnchanted.has_value() &&
                    data.second->IsEnchanted() != item.isEnchanted.value())
                    continue;
                if (item.isStolen.has_value() && !!data.second->GetOwner() != item.isStolen.value())
                    continue;

                int count = item.unique ? 1 : data.first;
                if (item.valueType == ValueType::Weight) {
                    value += data.second.get()->GetWeight() * count;
                } else {
                    value += count;
                }
            }
            item.global->value = value;
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
        if (data.contains("formType")) {
            Utils::FillSet<int>(data.at("formType"), rule.formTypes);
        }
        if (data.contains("unique")) {
            rule.unique = data.at("unique").get<bool>();
        }
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
        }
        if (data.contains("formExcludeFilter")) {
            rule.formExcludeFilter = Utils::GetForm<TESForm>(data.at("formExcludeFilter").get<std::string>());
            if (!rule.formExcludeFilter) return;
        }
        if (data.contains("enchanted")) {
            rule.isEnchanted = data.at("enchanted").get<bool>();
        }
        if (data.contains("stolen")) {
            rule.isStolen = data.at("stolen").get<bool>();
        }
        if (data.contains("valueType")) {
            std::string_view valueType = data.at("valueType").get<std::string_view>();
            if (valueType == "weight"sv) {
                rule.valueType = ValueType::Weight;
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
}