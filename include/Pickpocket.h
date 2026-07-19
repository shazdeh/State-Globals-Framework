#pragma once

namespace S_Pickpocket {
    FormID playerID = 0x14;
    Actor* currentTarget = nullptr;
    bool bPickpocketing = false;
    bool bQueued = false;

    struct Rule {
        TESGlobal* global;
        TESForm* conditionForm = nullptr; 
        std::optional<FormFilter> formFilter;
        std::optional<FormFilter> targetFilter;
        bool unique = false;
        bool reverse = false;
        ValueMod mod{};
    };

    std::vector<Rule> globals;
    std::unordered_map<TESForm*, std::pair<int, bool>> moveMap;

    Actor* GetTargetActor() {
        auto menu = UI::GetSingleton()->GetMenu<ContainerMenu>();
        TESObjectREFRPtr ptr;
        if (RE::LookupReferenceByHandle(menu->GetTargetRefHandle(), ptr)) {
            auto ref = ptr.get();
            if (ref && ref->IsActor()) {
                return ref->As<Actor>();
            }
        }
        return {};
    }

    void Process() {
        for (auto& moveEvent : moveMap) {
            for (auto& item : globals) {
                if (item.reverse != moveEvent.second.second) continue;
                if (item.conditionForm && !ValidateConditionForm(item.conditionForm)) continue;
                if (item.formFilter.has_value() && !ValidateFormFilter(moveEvent.first, item.formFilter.value())) continue;
                if (item.targetFilter.has_value() && !ValidateFormFilter(currentTarget, item.targetFilter.value())) continue;

                UpdateGlobalValue(item.global, item.mod, item.unique ? 1.0f : moveEvent.second.first);
            }
        }
        moveMap.clear();
        bQueued = false;
    }

    class ContainerSink : public BSTEventSink<TESContainerChangedEvent> {
        BSEventNotifyControl ProcessEvent(const TESContainerChangedEvent* event,
                                          BSTEventSource<TESContainerChangedEvent>*) {
            if (!event) return BSEventNotifyControl::kContinue;
            if (event->oldContainer == playerID || event->newContainer == playerID) {
                // with Poisoned perk this event fires multiple times if you pickpocket a poison
                // this is why we map the items to process the globals in one go.
                if (TESForm* form = TESForm::LookupByID(event->baseObj); form) {
                    moveMap[form].first += event->itemCount;
                    moveMap[form].second = event->oldContainer == playerID;
                }
                if (!bQueued) {
                    SKSE::GetTaskInterface()->AddTask(Process);
                    bQueued = true;
                }
            }
            return BSEventNotifyControl::kContinue;
        }
    };
    static ContainerSink containerSink;

    class UISink : public BSTEventSink<MenuOpenCloseEvent> {
        BSEventNotifyControl ProcessEvent(const MenuOpenCloseEvent* event, BSTEventSource<MenuOpenCloseEvent>*) {
            if (event->menuName == ContainerMenu::MENU_NAME) {
                if (event->opening && UI::GetSingleton()->GetMenu<ContainerMenu>()->GetContainerMode() == ContainerMenu::ContainerMode::kPickpocket) {
                    bPickpocketing = true;
                    currentTarget = GetTargetActor();
                    if (currentTarget) {
                        ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESContainerChangedEvent>(&containerSink);
                    }
                } else if(bPickpocketing) {
                    ScriptEventSourceHolder::GetSingleton()->RemoveEventSink<TESContainerChangedEvent>(&containerSink);
                    currentTarget = nullptr;
                    bPickpocketing = false;
                }
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("pickpocket")) return;
        auto& data = item.at("pickpocket");
        Rule rule;
        if (data.contains("mod")) {
            rule.mod = ParseValueMod(item);
        }
        if (data.contains("conditionForm")) {
            rule.conditionForm = Utils::GetForm<TESForm>(data.at("conditionForm").get<std::string>());
            if (!rule.conditionForm) return;
        }
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("targetFilter")) {
            rule.targetFilter = ParseFormFilter(data.at("targetFilter"));
            if (rule.targetFilter == std::nullopt) return;
        }
        if (data.contains("unique")) {
            rule.unique = data.at("unique").get<bool>();
        }
        if (data.contains("reverse")) {
            rule.reverse = data.at("reverse").get<bool>();
        }

        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static UISink uiSink;
            UI::GetSingleton()->AddEventSink<MenuOpenCloseEvent>(&uiSink);
        }
    }
}