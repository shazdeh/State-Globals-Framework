#pragma once

namespace S_Equip {
    bool bQueued;

    enum ValueType { Count = 0, Weight = 1, GoldValue = 2 };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        std::optional<SpellFilter> enchantFilter;
        std::optional<SpellFilter> spellFilter;
        ValueType valueType = ValueType::Count;
        ValueMod mod{};
        std::optional<bool> isStolen;
        std::optional<bool> isEnchanted;
        bool weapon = false;
        bool apparel = false;
        bool spell = false;
        bool scroll = false;
        bool left = true;
        bool right = true;
        // std::optional<bool> isPoisoned;
    };

    std::vector<Rule> globals;

    void Process() {
        auto inventory = player->GetInventory();
        auto leftObject = player->GetEquippedObject(true);
        auto rightObject = player->GetEquippedObject(false);
        auto leftEntry = player->GetEquippedEntryData(true);
        auto rightEntry = player->GetEquippedEntryData(false);

        for (auto& item : globals) {
            float value = 0;
            bool states[] = {false, true};
            for (auto state : states) {
                if ((state == false && !item.right) || (state && !item.left)) continue;
                TESForm* object = state == false ? rightObject : leftObject;
                if (!object) continue;
                if ((item.weapon && object->Is(FormType::Weapon)) ||
                    (item.scroll && object->Is(FormType::Scroll))) {
                    InventoryEntryData* entryData = state == false ? rightEntry : leftEntry;
                    if (!entryData) continue;
                    bool isStolen = entryData->GetOwner() ? true : false;
                    if (item.isStolen.has_value() && isStolen != item.isStolen.value()) continue;
                    if (item.isEnchanted.has_value() && entryData->IsEnchanted() != item.isEnchanted.value()) continue;
                    // if (item.isPoisoned.has_value() && entryData->IsPoisoned() != item.isPoisoned.value())
                    //     continue;
                    if (item.formFilter.has_value() && !ValidateFormFilter(object, item.formFilter.value())) continue;

                    if (item.enchantFilter.has_value()) {
                        if (!entryData->IsEnchanted()) continue;
                        auto enchant = entryData->GetEnchantment();
                        if (!enchant || !ValidateSpellFilter(item.enchantFilter.value(), enchant)) continue;
                    }

                    if (item.valueType == ValueType::Count) {
                        value += 1;
                    } else if (item.valueType == ValueType::GoldValue) {
                        value += object->GetGoldValue();
                    } else {
                        value += entryData->GetWeight();
                    }
                }
                if (item.spell && object->Is(FormType::Spell)) {
                    if (item.formFilter.has_value() && !ValidateFormFilter(object, item.formFilter.value())) continue;
                    if (item.spellFilter.has_value() &&
                        !ValidateSpellFilter(item.spellFilter.value(), object->As<SpellItem>()))
                        continue;

                    value += 1;
                }
            } // left & right
            if (item.apparel) {
                for (auto& [inventoryItem, data] : inventory) {
                    if (inventoryItem->Is(RE::FormType::LeveledItem)) continue;
                    auto* inv = data.second.get();
                    if (data.first == 0 || !inv || !inventoryItem->GetPlayable()) continue;
                    if (!inventoryItem->Is(FormType::Armor) || !inv->IsWorn()) continue;
                    if (item.isStolen.has_value() && !!inv->GetOwner() != item.isStolen.value()) continue;
                    if (item.isEnchanted.has_value() && inv->IsEnchanted() != item.isEnchanted.value())
                         continue;
                    if (item.formFilter.has_value() && !ValidateFormFilter(inventoryItem, item.formFilter.value())) continue;

                    if (item.enchantFilter.has_value()) {
                        if (!inv->IsEnchanted()) continue;
                        auto enchant = inv->GetEnchantment();
                        if (!enchant || !ValidateSpellFilter(item.enchantFilter.value(), enchant)) continue;
                    }

                    if (item.valueType == ValueType::Count) {
                        value += 1;
                    } else if (item.valueType == ValueType::GoldValue) {
                        value += inventoryItem->GetGoldValue();
                    } else {
                        value += inv->GetWeight();
                    }
                }
            }
            UpdateGlobalValue(item.global, item.mod, value, true);
        } // globals loop
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
        if (data.contains("enchantFilter")) {
            rule.enchantFilter = ParseSpellFilter(data.at("enchantFilter"));
            if (rule.enchantFilter == std::nullopt) return;
        }
        if (data.contains("spellFilter")) {
            rule.spellFilter = ParseSpellFilter(data.at("spellFilter"));
            if (rule.spellFilter == std::nullopt) return;
        }
        if (data.contains("stolen")) {
            rule.isStolen = data.at("stolen").get<bool>();
        }
        if (data.contains("enchanted")) {
            rule.isEnchanted = data.at("enchanted").get<bool>();
        }
        if (data.contains("weapon")) {
            rule.weapon = data.at("weapon").get<bool>();
        }
        if (data.contains("scroll")) {
            rule.scroll = data.at("scroll").get<bool>();
        }
        if (data.contains("spell")) {
            rule.spell = data.at("spell").get<bool>();
        }
        if (data.contains("left")) {
            rule.left = data.at("left").get<bool>();
        }
        if (data.contains("right")) {
            rule.right = data.at("right").get<bool>();
        }
        if (data.contains("apparel")) {
            rule.apparel = data.at("apparel").get<bool>();
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

    void OnLoadGame() { Process();
    }
}