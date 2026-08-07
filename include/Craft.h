#pragma once

namespace S_Craft {
    TESForm* lastCraftedObject = nullptr;
    TESObjectREFR* lastUsedWorkbench = nullptr;

    struct Rule {
        TESGlobal* global;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        std::optional<FormFilter> furnitureFilter;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    TESObjectREFR* GetCurrentFurniture() {
        auto player = PlayerCharacter::GetSingleton();
        auto handle = player->GetOccupiedFurniture();
        if (handle) {
            if (auto ref = handle.get().get(); ref) {
                return ref;
            }
        }
        return nullptr;
    }

    void Process() {
        for (auto& item : globals) {
            ConsoleLog::GetSingleton()->Print(fmt::format("Process: {}", clib_util::editorID::get_editorID(item.global)).c_str());
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(lastCraftedObject, item.formFilter.value())) continue;
            if (item.furnitureFilter.has_value() &&
                (!lastUsedWorkbench ||
                 !ValidateFormFilter(lastUsedWorkbench->GetBaseObject(), item.furnitureFilter.value())))
                continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<ItemCrafted::Event> {
        BSEventNotifyControl ProcessEvent(const ItemCrafted::Event* event, BSTEventSource<ItemCrafted::Event>*) {
            if (event->item) {
                lastCraftedObject = event->item;
                lastUsedWorkbench = GetCurrentFurniture();

                if (bLogIDs)
                    ConsoleLog::GetSingleton()->Print(
                        fmt::format("Crafted: {}", clib_util::editorID::get_editorID(event->item)).c_str());
                Process();
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("craft")) return;
        auto& data = item.at("craft");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            if (data.at("formFilter").is_string()) {
                ConsoleLog::GetSingleton()->Print(fmt::format("filter: {}", data.at("formFilter").get<std::string>()).c_str());
            }
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (data.at("formFilter").is_string()) {
                ConsoleLog::GetSingleton()->Print(fmt::format("filter: {}", data.at("formFilter").get<std::string>()).c_str());
            }
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("furnitureFilter")) {
            rule.furnitureFilter = ParseFormFilter(data.at("furnitureFilter"));
            if (rule.furnitureFilter == std::nullopt) return;
        }
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }

        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink craftSink;
            ItemCrafted::GetEventSource()->AddEventSink(&craftSink);
        }
    }

    TESForm* GetLastCraftedObject(StaticFunctionTag*) {
        return lastCraftedObject;
    }

    TESForm* GetLastUsedWorkbench(StaticFunctionTag*) {
        return lastUsedWorkbench;
    }

    void OnLoadGame() {
        lastCraftedObject = nullptr;
        lastUsedWorkbench = nullptr;
    }
}