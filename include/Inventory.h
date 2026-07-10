#pragma once

namespace S_Inventory {
    bool bQueued;
    FormID playerID = 0x14;

    struct Rule {
        std::unordered_set<TESBoundObject*> excludes;
        std::vector<BGSKeyword*> keywords;
        std::unordered_set<int> formTypes;
        bool keywordMatchAll = false;
        bool unique = false;
        std::optional<bool> isEnchanted;
        std::optional<bool> isStolen;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void Process() {
        auto inventory = PlayerCharacter::GetSingleton()->GetInventory();
        for (auto& item : globals) {
            float value = 0.0f;
            for (auto& [inventoryItem, data] : inventory) {
                if (!item.second.formTypes.empty() &&
                    !item.second.formTypes.contains(std::to_underlying(inventoryItem->GetFormType())))
                    continue;
                if (!item.second.excludes.empty() && item.second.excludes.contains(inventoryItem)) continue;
                if (!item.second.keywords.empty() &&
                    !inventoryItem->HasKeywordInArray(item.second.keywords, item.second.keywordMatchAll))
                    continue;
                if (item.second.isEnchanted.has_value() &&
                    data.second->IsEnchanted() != item.second.isEnchanted.value())
                    continue;
                if (item.second.isStolen.has_value() && !!data.second->GetOwner() != item.second.isStolen.value())
                    continue;

                if (item.second.unique) {
                    value += 1;
                } else {
                    value += data.first;
                }
            }
            item.first->value = value;
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

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
        auto& data = item.at("inventory");
        Rule rule;
        if (data.contains("formType")) {
            Utils::FillSet<int>(data.at("formType"), rule.formTypes);
        }
        if (data.contains("unique")) {
            rule.unique = data.at("unique").get<bool>();
        }
        if (data.contains("exclude")) {
            if (!Utils::FillFormsSet(data.at("exclude"), rule.excludes)) return {};
        }
        if (data.contains("keyword")) {
            if (!Utils::fillFormsArray(data.at("keyword"), rule.keywords)) return {};
            if (data.contains("keywordMatchAll")) {
                rule.keywordMatchAll = data.at("keywordMatchAll").get<bool>();
            }
        }
        if (data.contains("enchanted")) {
            rule.isEnchanted = data.at("enchanted").get<bool>();
        }
        if (data.contains("stolen")) {
            rule.isStolen = data.at("stolen").get<bool>();
        }
        return rule;
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESContainerChangedEvent>(&g_sink);
        }
    }
}