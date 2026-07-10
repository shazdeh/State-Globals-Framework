#pragma once

namespace S_MagicEffect {
    bool bQueued;

    enum Scope { Current = 0, Lifetime = 1 };

    struct Rule {
        std::optional<MagicSystem::SpellType> spellType;
        std::unordered_set<MagicItem*> spells;
        bool unique = true;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    void Process() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        float value = 0.0f;
        auto effects = mt->GetActiveEffectList();
        for (auto& item : globals) {
            std::unordered_set<MagicItem*> visited;
            for (auto* effect : *effects) {
                if (!effect || !effect->spell) continue;
                if (item.second.spellType && effect->spell->GetSpellType() != item.second.spellType) continue;
                if (!item.second.spells.empty() && !item.second.spells.contains(effect->spell)) continue;
                if (item.second.unique) {
                    if (visited.contains(effect->spell)) continue;
                    visited.insert(effect->spell);
                }

                value += 1;
            }
            item.first->value = value;
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

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
        auto& data = item.at("magiceffect");
        Rule rule;
        if (data.contains("spellType")) {
            rule.spellType = static_cast<MagicSystem::SpellType>(data.at("spellType").get<int>());
        }
        if (data.contains("spell")) {
            if (!Utils::FillFormsSet(data.at("spell"), rule.spells)) return {};
            ConsoleLog::GetSingleton()->Print(fmt::format("count of spells: {}", rule.spells.size()).c_str());
        }
        return rule;
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESMagicEffectApplyEvent>(&g_sink);
        }
    }
}