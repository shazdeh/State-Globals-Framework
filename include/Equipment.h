#pragma once

namespace S_Equip {
    bool bQueued;

    enum ValueType { Count = 0, Weight = 1, GoldValue = 2 };

    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* formFilter = nullptr;
        int formType = 0;
        std::optional<bool> isStolen;
        std::optional<bool> isEnchanted;
        std::optional<bool> isPoisoned;
        ValueType valueType = ValueType::Count;
    };

    std::vector<Rule> globals;

    void Process() {
        auto player = PlayerCharacter::GetSingleton();
        auto inventory = player->GetInventory();

        for (auto& item : globals) {
            float value = 0;
            if (item.formType == 41) {
                bool states[] = {false, true};
                for (auto state : states) {
                    if (auto* entryData = player->GetEquippedEntryData(state); entryData) {
                        int isStolen = entryData->GetOwner() ? 1 : 0;
                        if (item.isStolen.has_value() && isStolen != item.isStolen.value()) continue;
                        if (item.isEnchanted.has_value() && entryData->IsEnchanted() != item.isEnchanted.value())
                            continue;
                        if (item.isPoisoned.has_value() && entryData->IsPoisoned() != item.isPoisoned.value())
                            continue;
                        if (item.formFilter && !Utils::ParseFormFilter(entryData->GetObject(), item.formFilter))
                            continue;

                        if (item.valueType == ValueType::Count) {
                            value += 1;
                        } else if(item.valueType == ValueType::GoldValue) {
                            value += entryData->GetObject()->GetGoldValue();
                        } else {
                            value += entryData->GetWeight();
                        }
                    }
                }
            } else if (item.formType == 26) {
                for (auto& [inventoryItem, data] : inventory) {
                    if (!inventoryItem->Is(FormType::Armor) || !data.second->IsWorn()) continue;
                    if (item.isStolen.has_value() && !!data.second->GetOwner() != item.isStolen.value()) continue;
                    if (item.isEnchanted.has_value() && data.second->IsEnchanted() != item.isEnchanted.value())
                         continue;
                    if (item.formFilter && !Utils::ParseFormFilter(inventoryItem, item.formFilter)) continue;

                    if (item.valueType == ValueType::Count) {
                        value += 1;
                    } else if (item.valueType == ValueType::GoldValue) {
                        value += inventoryItem->GetGoldValue();
                    } else {
                        value += data.second->GetWeight();
                    }
                }
            }
            item.global->value = value;
        }
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESEquipEvent> {
        BSEventNotifyControl ProcessEvent(const TESEquipEvent* event, BSTEventSource<TESEquipEvent>*) {
            if (!event->actor || !event->baseObject || bQueued) return BSEventNotifyControl::kContinue;
            if (event->actor->IsPlayerRef()) {
                SKSE::GetTaskInterface()->AddTask(Process);
                bQueued = true;
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("equip")) return;
        auto& data = item.at("equip");
        if (!data.contains("formType")) return;
        Rule rule;
        rule.formType = data.at("formType").get<int>();
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
        }
        if (data.contains("stolen")) {
            rule.isStolen = data.at("stolen").get<bool>() ? 1 : 0;
        }
        if (data.contains("enchanted")) {
            rule.isEnchanted = data.at("enchanted").get<bool>() ? 1 : 0;
        }
        if (data.contains("valueType")) {
            auto valueType = data.at("valueType").get<std::string_view>();
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
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESEquipEvent>(&g_sink);
        }
    }
}