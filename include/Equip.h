#pragma once

namespace S_Equip {
    bool bQueued;

    enum ValueType { Count = 0, Weight = 1, GoldValue = 2 };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        ValueType valueType = ValueType::Count;
        ValueMod mod{};
        std::optional<bool> isStolen;
        std::optional<bool> isEnchanted;
        bool weapon = true;
        bool apparel = true;
        // std::optional<bool> isPoisoned;
    };

    std::vector<Rule> globals;

    void Process() {
        auto inventory = player->GetInventory();

        for (auto& item : globals) {
            float value = 0;
            if (item.weapon) {
                bool states[] = {false, true};
                for (auto state : states) {
                    if (auto* entryData = player->GetEquippedEntryData(state); entryData) {
                        bool isStolen = entryData->GetOwner() ? true : false;
                        if (item.isStolen.has_value() && isStolen != item.isStolen.value()) continue;
                        if (item.isEnchanted.has_value() && entryData->IsEnchanted() != item.isEnchanted.value())
                            continue;
                        // if (item.isPoisoned.has_value() && entryData->IsPoisoned() != item.isPoisoned.value())
                        //     continue;
                        if (item.formFilter.has_value() &&
                            !ValidateFormFilter(entryData->GetObject(), item.formFilter.value()))
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
            }
            if (item.apparel) {
                for (auto& [inventoryItem, data] : inventory) {
                    if (!inventoryItem->Is(FormType::Armor) || !data.second->IsWorn()) continue;
                    if (item.isStolen.has_value() && !!data.second->GetOwner() != item.isStolen.value()) continue;
                    if (item.isEnchanted.has_value() && data.second->IsEnchanted() != item.isEnchanted.value())
                         continue;
                    if (item.formFilter.has_value() && !ValidateFormFilter(inventoryItem, item.formFilter.value())) continue;

                    if (item.valueType == ValueType::Count) {
                        value += 1;
                    } else if (item.valueType == ValueType::GoldValue) {
                        value += inventoryItem->GetGoldValue();
                    } else {
                        value += data.second->GetWeight();
                    }
                }
            }
            UpdateGlobalValue(item.global, item.mod, value, true);
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
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("stolen")) {
            rule.isStolen = data.at("stolen").get<bool>() ? 1 : 0;
        }
        if (data.contains("enchanted")) {
            rule.isEnchanted = data.at("enchanted").get<bool>() ? 1 : 0;
        }
        if (data.contains("weapon")) {
            rule.weapon = data.at("weapon").get<bool>() ? 1 : 0;
        }
        if (data.contains("apparel")) {
            rule.apparel = data.at("apparel").get<bool>() ? 1 : 0;
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