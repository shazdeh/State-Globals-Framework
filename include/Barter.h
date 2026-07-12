#pragma once

namespace S_Barter {
    FormID playerID = 0x14;
    FormID goldID = 0xF;

    struct Rule {
        TESGlobal* global;
        TESForm* formFilter = nullptr;
        TESForm* vendorFilter = nullptr;
        std::unordered_set<int> formTypes;
        bool unique = false;
        bool buy = true;
        bool sell = true;
    };

    std::vector<Rule> globals;

    Actor* GetBarteringActor() {
        auto menu = UI::GetSingleton()->GetMenu<BarterMenu>();
        TESObjectREFRPtr ptr;
        if (RE::LookupReferenceByHandle(menu->GetTargetRefHandle(), ptr)) {
            auto ref = ptr.get();
            if (ref && ref->IsActor()) {
                return ref->As<Actor>();
            }
        }
        return {};
    }

    void Process(bool buy, FormID itemID, int32_t count) {
        Actor* vendor = GetBarteringActor();
        if (!vendor) return;
        for (auto& item : globals) {
            if ((buy && !item.buy) || (!buy && !item.sell)) continue;
            if (TESForm* form = TESForm::LookupByID(itemID); form) {
                if (!empty(item.formTypes) && !item.formTypes.contains(std::to_underlying(form->GetFormType())))
                    continue;
                if (item.formFilter && !Utils::ParseFormFilter(form, item.formFilter)) continue;
                if (item.vendorFilter && !Utils::ParseActorFilter(vendor, item.vendorFilter)) continue;

                if (item.unique) count = 1;
                item.global->value += count;
            }
        }
    }

    class ContainerSink : public BSTEventSink<TESContainerChangedEvent> {
        BSEventNotifyControl ProcessEvent(const TESContainerChangedEvent* event,
                                          BSTEventSource<TESContainerChangedEvent>*) {
            if (!event || event->baseObj == goldID) return BSEventNotifyControl::kContinue;
            if (event->oldContainer == playerID || event->newContainer == playerID) {
                Process(event->newContainer == playerID, event->baseObj, event->itemCount);
            }
            return BSEventNotifyControl::kContinue;
        }
    };
    static ContainerSink containerSink;

    class UISink : public BSTEventSink<MenuOpenCloseEvent> {
        BSEventNotifyControl ProcessEvent(const MenuOpenCloseEvent* event, BSTEventSource<MenuOpenCloseEvent>*) {
            if (event->menuName == BarterMenu::MENU_NAME) {
                if (event->opening) {
                    ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESContainerChangedEvent>(&containerSink);
                } else {
                    ScriptEventSourceHolder::GetSingleton()->RemoveEventSink<TESContainerChangedEvent>(&containerSink);
                }
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("barter")) return;
        auto& data = item.at("barter");
        Rule rule;
        if (data.contains("formType")) {
            Utils::FillSet<int>(data.at("formType"), rule.formTypes);
        }
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
        }
        if (data.contains("vendorFilter")) {
            rule.vendorFilter = Utils::GetForm<TESForm>(data.at("vendorFilter").get<std::string>());
            if (!rule.vendorFilter) return;
        }
        if (data.contains("unique")) {
            rule.unique = data.at("unique").get<bool>();
        }
        if (data.contains("buy")) {
            rule.buy = data.at("buy").get<bool>();
        }
        if (data.contains("sell")) {
            rule.sell = data.at("sell").get<bool>();
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