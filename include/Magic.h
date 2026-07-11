#pragma once

namespace S_SpellCast {
    struct Rule {
        std::unordered_set<int> formTypes;
        std::unordered_set<TESForm*> forms;
        std::vector<BGSKeyword*> keywords;
        std::vector<BGSKeyword*> magicEffectKeywords;
        std::unordered_set<EffectSetting*> magicEffects;
        bool keywordMatchAll = false;
        float mod = 1.0f;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void Process(FormID spellID) {
        TESForm* spell = TESForm::LookupByID(spellID);
        for (auto& item : globals) {
            if (!item.second.formTypes.empty() &&
                !item.second.formTypes.contains(std::to_underlying(spell->GetFormType())))
                continue;
            if (!item.second.keywords.empty() &&
                !spell->HasKeywordInArray(item.second.keywords, item.second.keywordMatchAll))
                continue;
            if (!item.second.forms.empty() && !item.second.forms.contains(spell)) continue;
            if (!item.second.magicEffects.empty() && !Utils::FormHasAnyMagicEffect(spell, item.second.magicEffects)) continue;
            if (!item.second.magicEffectKeywords.empty() &&
                !Utils::FormHasMagicEffectKeyword(spell, item.second.magicEffectKeywords))
                continue;

            if (item.second.mod == 0.0f) {
                item.first->value = 0;
            } else {
                item.first->value += item.second.mod;
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

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
        auto& data = item.at("spellcast");
        Rule rule;
        if (data.contains("formType")) {
            Utils::FillSet<int>(data.at("formType"), rule.formTypes);
        }
        if (data.contains("form")) {
            Utils::FillFormsSet(data.at("form"), rule.forms);
        }
        if (data.contains("magicEffect")) {
            Utils::FillFormsSet(data.at("magicEffect"), rule.magicEffects);
        }
        if (data.contains("keyword")) {
            if (!Utils::fillFormsArray(data.at("keyword"), rule.keywords)) return {};
            if (data.contains("keywordMatchAll")) {
                rule.keywordMatchAll = data.at("keywordMatchAll").get<bool>();
            }
        }
        if (data.contains("magicEffectKeyword")) {
            if (!Utils::fillFormsArray(data.at("magicEffectKeyword"), rule.magicEffectKeywords)) return {};
        }
        if (data.contains("mod")) {
            rule.mod = data.at("mod").get<float>();
        }
        return rule;
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESSpellCastEvent>(&g_sink);
        }
    }
}