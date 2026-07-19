#pragma once

namespace S_Magic {
    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* conditionForm = nullptr;
        std::optional<FormFilter> formFilter;
        float mod = 1.0f;
    };

    std::vector<Rule> globals;

    void Process(FormID spellID) {
        TESForm* spell = TESForm::LookupByID(spellID);
        // ConsoleLog::GetSingleton()->Print(fmt::format("Spell ID: {:x}", spell->GetFormID()).c_str());
        for (auto& item : globals) {
            if (item.formFilter.has_value() && !ValidateFormFilter(spell, item.formFilter.value())) continue;
            if (item.conditionForm && !ValidateConditionForm(item.conditionForm)) continue;

            if (item.mod == 0.0f) { 
                item.global->value = 0;
            } else {
                item.global->value += item.mod;
            }
        }
    }

    class EventSink : public BSTEventSink<TESSpellCastEvent> {
        BSEventNotifyControl ProcessEvent(const TESSpellCastEvent* event, BSTEventSource<TESSpellCastEvent>*) {
            if (!event->object) return BSEventNotifyControl::kContinue;
            auto ref = event->object.get();
            if (ref && ref->IsPlayerRef()) {
                Process(event->spell);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("spellcast")) return;
        auto& data = item.at("spellcast");
        Rule rule;
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("conditionForm")) {
            rule.conditionForm = Utils::GetForm<TESForm>(data.at("conditionForm").get<std::string>());
            if (!rule.conditionForm) return;
        }
        if (data.contains("mod")) {
            rule.mod = data.at("mod").get<float>();
        }

        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESSpellCastEvent>(&g_sink);
        }
    }
}