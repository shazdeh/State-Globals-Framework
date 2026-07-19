#pragma once

namespace S_Barter {
    FormID playerID = 0x14;
    FormID goldID = 0xF;
    Actor* currentVendor = nullptr;

    struct Rule {
        TESGlobal* global;
        TESForm* conditionForm = nullptr;
        std::optional<FormFilter> formFilter;
        std::optional<FormFilter> vendorFilter;
        bool unique = false;
        float mod = 1.0f;
        std::optional<float> min;
        std::optional<float> max;
    };

    std::vector<Rule> buyGlobals;
    std::vector<Rule> sellGlobals;

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

    void Process(std::vector<Rule>&arr, FormID itemID, int32_t count) {
        for (auto& item : arr) {
            if (TESForm* form = TESForm::LookupByID(itemID); form) {
                if (item.formFilter.has_value() && !ValidateFormFilter(form, item.formFilter.value()))
                    continue;
                if (item.vendorFilter.has_value() && !ValidateFormFilter(currentVendor, item.vendorFilter.value()))
                    continue;
                if (item.conditionForm && !ValidateConditionForm(item.conditionForm)) continue;

                if (item.mod == 0.0f) {
                    item.global->value = 0;
                } else {
                    if (item.unique) count = 1;
                    float newValue = item.global->value + (count * item.mod);
                    if (item.max.has_value() && newValue > item.max.value())
                        newValue = item.max.value();
                    else if (item.min.has_value() && newValue < item.min.value())
                        newValue = item.min.value();
                    item.global->value = newValue;
                }
            }
        }
    }

    class ContainerSink : public BSTEventSink<TESContainerChangedEvent> {
        BSEventNotifyControl ProcessEvent(const TESContainerChangedEvent* event,
                                          BSTEventSource<TESContainerChangedEvent>*) {
            if (!event || event->baseObj == goldID) return BSEventNotifyControl::kContinue;
            if (event->oldContainer == playerID || event->newContainer == playerID) {
                Process(event->newContainer == playerID ? buyGlobals : sellGlobals, event->baseObj, event->itemCount);
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
        for (std::string_view key : {"buy"sv, "sell"sv}) {
            if (!item.contains(key)) continue;
            auto& data = item.at(key);
            Rule rule;
            if (data.contains("formFilter")) {
                rule.formFilter = ParseFormFilter(data.at("formFilter"));
                if (rule.formFilter == std::nullopt) return;
            }
            if (data.contains("vendorFilter")) {
                rule.vendorFilter = ParseFormFilter(data.at("vendorFilter"));
                if (rule.vendorFilter == std::nullopt) return;
            }
            if (data.contains("conditionForm")) {
                rule.conditionForm = Utils::GetForm<TESForm>(data.at("conditionForm").get<std::string>());
                if (!rule.conditionForm) return;
            }
            if (data.contains("unique")) {
                rule.unique = data.at("unique").get<bool>();
            }
            if (data.contains("mod")) {
                rule.mod = data.at("mod").get<float>();
            }
            if (item.contains("min")) {
                rule.min = item.at("min").get<float>();
            }
            if (item.contains("max")) {
                rule.max = item.at("max").get<float>();
            }

            rule.global = global;
            (key == "buy"sv ? buyGlobals : sellGlobals).push_back(rule);
        }
    }

    void SetupEvents() {
        if (!buyGlobals.empty() || !sellGlobals.empty()) {
            static UISink uiSink;
            UI::GetSingleton()->AddEventSink<MenuOpenCloseEvent>(&uiSink);
        }
    }
}