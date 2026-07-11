#pragma once

namespace Equipment {
    bool bQueued;

    enum ValueType { Count = 0, Weight = 1 };

    struct Rule {
        int formType = 0;
        int isStolen = -1;
        int isEnchanted = -1;
        ValueType valueType = ValueType::Count;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void Process() {
        auto player = PlayerCharacter::GetSingleton();
        auto inventory = player->GetInventory();

        for (auto& item : globals) {
            float value = 0;
            if (item.second.formType == 41) {
                bool states[] = {false, true};
                for (auto state : states) {
                    if (auto* entryData = player->GetEquippedEntryData(state); entryData) {
                        int isStolen = entryData->GetOwner() ? 1 : 0;
                        if (item.second.isStolen != -1 && isStolen != item.second.isStolen) continue;
                        if (item.second.isEnchanted != -1 && entryData->IsEnchanted() != !!item.second.isEnchanted)
                            continue;

                        if (item.second.valueType == ValueType::Count) {
                            value += 1;
                        } else {
                            value += entryData->GetWeight();
                        }
                    }
                }
            } else if (item.second.formType == 26) {
                for (auto& [inventoryItem, data] : inventory) {
                    if (!inventoryItem->Is(FormType::Armor) || !data.second->IsWorn()) continue;
                    if (item.second.isStolen != -1 && !!data.second->GetOwner() != item.second.isStolen) continue;
                    if (item.second.isEnchanted != -1 && data.second->IsEnchanted() != !!item.second.isEnchanted)
                        continue;

                    if (item.second.valueType == ValueType::Count) {
                        value += 1;
                    } else {
                        value += data.second->GetWeight();
                    }
                }
            }
            item.first->value = value;
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

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
        auto& data = item.at("equip");
        if (!data.contains("formType")) return std::nullopt;
        Rule rule;
        rule.formType = data.at("formType").get<int>();
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
            }
        }
        return rule;
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESEquipEvent>(&g_sink);
        }
    }
}