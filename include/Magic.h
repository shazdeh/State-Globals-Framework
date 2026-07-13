#pragma once

namespace S_Magic {
    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* formFilter = nullptr;
        std::unordered_set<int> formTypes;
        std::vector<BGSKeyword*> magicEffectKeywords;
        std::unordered_set<EffectSetting*> magicEffects;
        float mod = 1.0f;
    };

    std::vector<Rule> globals;

    void Process(FormID spellID) {
        TESForm* spell = TESForm::LookupByID(spellID);
        for (auto& item : globals) {
            if (!item.formTypes.empty() &&
                !item.formTypes.contains(std::to_underlying(spell->GetFormType())))
                continue;
            if (item.formFilter && !Utils::ParseFormFilter(spell, item.formFilter)) continue;
            if (!item.magicEffects.empty() && !Utils::FormHasAnyMagicEffect(spell, item.magicEffects)) continue;
            if (!item.magicEffectKeywords.empty() &&
                !Utils::FormHasMagicEffectKeyword(spell, item.magicEffectKeywords))
                continue;

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
        if (data.contains("formType")) {
            Utils::FillSet<int>(data.at("formType"), rule.formTypes);
        }
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
        }
        if (data.contains("magicEffect")) {
            Utils::FillFormsSet(data.at("magicEffect"), rule.magicEffects);
        }
        if (data.contains("magicEffectKeyword")) {
            if (!Utils::fillFormsArray(data.at("magicEffectKeyword"), rule.magicEffectKeywords)) return;
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