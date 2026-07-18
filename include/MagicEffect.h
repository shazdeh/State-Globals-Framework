#pragma once

namespace S_MagicEffect {
    bool bQueued;

    enum Scope { Current = 0, Lifetime = 1 };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        std::optional<MagicSystem::SpellType> spellType;
        bool unique = true;
    };

    std::vector<Rule> globals;

    void Process() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        float value = 0.0f;
        auto effects = mt->GetActiveEffectList();
        for (auto& item : globals) {
            std::unordered_set<MagicItem*> visited;
            for (auto* effect : *effects) {
                if (!effect || !effect->spell) continue;
                if (item.spellType && effect->spell->GetSpellType() != item.spellType) continue;
                if (item.formFilter.has_value() && !ValidateFormFilter(effect->spell, item.formFilter.value())) continue;
                if (item.unique) {
                    if (visited.contains(effect->spell)) continue;
                    visited.insert(effect->spell);
                }

                value += 1;
            }
            item.global->value = value;
        }
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESMagicEffectApplyEvent> {
        BSEventNotifyControl ProcessEvent(const TESMagicEffectApplyEvent* event,
                                          BSTEventSource<TESMagicEffectApplyEvent>*) {
            if (bQueued || !event || !event->target || !event->target->IsPlayerRef())
                return BSEventNotifyControl::kContinue;
            SKSE::GetTaskInterface()->AddTask(Process);
            bQueued = true;
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("magiceffect")) return;
        auto& data = item.at("magiceffect");
        Rule rule;
        if (data.contains("spellType")) {
            rule.spellType = static_cast<MagicSystem::SpellType>(data.at("spellType").get<int>());
        }
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESMagicEffectApplyEvent>(&g_sink);
        }
    }
}