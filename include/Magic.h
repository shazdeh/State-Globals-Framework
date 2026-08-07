#pragma once

namespace S_Magic {
    TESForm* lastCastedSpell = nullptr;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    void Process() {
        if (bLogIDs)
            ConsoleLog::GetSingleton()->Print(fmt::format("Spell Cast! ID: {:x}, Editor ID: {}",
                                                          lastCastedSpell->GetFormID(),
                                                          clib_util::editorID::get_editorID(lastCastedSpell))
                                                  .c_str());
        for (auto& item : globals) {
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(lastCastedSpell, item.formFilter.value())) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<TESSpellCastEvent> {
        BSEventNotifyControl ProcessEvent(const TESSpellCastEvent* event, BSTEventSource<TESSpellCastEvent>*) {
            if (!event->object || event->spell == 0) return BSEventNotifyControl::kContinue;
            auto ref = event->object.get();
            if (ref && ref->IsPlayerRef()) {
                lastCastedSpell = TESForm::LookupByID(event->spell);
                if (lastCastedSpell) Process();
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("spellcast")) return;
        auto& data = item.at("spellcast");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
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
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESSpellCastEvent>(&g_sink);
        }
    }

    TESForm* GetLastCastedSpell(StaticFunctionTag*) {
        return lastCastedSpell;
    }

    void OnLoadGame() { lastCastedSpell = nullptr;
    }
}