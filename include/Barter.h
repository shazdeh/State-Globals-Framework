#pragma once

namespace S_Barter {
    FormID playerID = 0x14;
    FormID goldID = 0xF;
    Actor* currentVendor = nullptr;

    struct Rule {
        TESGlobal* global;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        std::optional<FormFilter> vendorFilter;
        ValueMod mod{};
        float sellMult = 1.0f;
        float buyMult = 1.0f;
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

    void Process(bool isBuying, FormID itemID, int32_t count) {
        for (auto& item : globals) {
            if ((isBuying && !item.buy) || (!isBuying && !item.sell)) continue;
            if (TESForm* form = TESForm::LookupByID(itemID); form) {    
                if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
                if (item.formFilter.has_value() && !ValidateFormFilter(form, item.formFilter.value()))
                    continue;
                if (item.vendorFilter.has_value() && !ValidateFormFilter(currentVendor, item.vendorFilter.value()))
                    continue;

                if (item.unique) count = 1;
                UpdateGlobalValue(item.global, item.mod, (isBuying ? item.buyMult : item.sellMult) * static_cast<float>(count));
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
                    currentVendor = GetBarteringActor();
                    if (currentVendor) {
                        ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESContainerChangedEvent>(&containerSink);
                    }
                } else {
                    ScriptEventSourceHolder::GetSingleton()->RemoveEventSink<TESContainerChangedEvent>(&containerSink);
                    currentVendor = nullptr;
                }
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("barter")) return;
        auto& data = item.at("barter");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("vendorFilter")) {
            rule.vendorFilter = ParseFormFilter(data.at("vendorFilter"));
            if (rule.vendorFilter == std::nullopt) return;
        }
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
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
        if (data.contains("buyMult")) {
            rule.buyMult = data.at("buyMult").get<float>();
        }
        if (data.contains("sellMult")) {
            rule.sellMult = data.at("sellMult").get<float>();
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