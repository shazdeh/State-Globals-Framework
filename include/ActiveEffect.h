#pragma once

namespace S_ActiveEffect {
    bool bQueued;
    std::unique_ptr<Ticker> ticker;

    enum Scope { Current = 0, Lifetime = 1 };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        std::optional<MagicSystem::SpellType> spellType;
        ValueMod mod{};
        bool unique = true;
        Scope scope = Scope::Current;
    };

    std::vector<Rule> globals;
    std::unordered_set<EffectSetting*> newEffects;

    bool ValidateSpell(Rule& rule, MagicItem* spell) {
        if (rule.spellType && spell->GetSpellType() != rule.spellType) return false;
        if (rule.formFilter.has_value() && !ValidateFormFilter(spell, rule.formFilter.value())) return false;
        return true;
    }

    void UpdateLifetimeGlobals() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        auto effects = mt->GetActiveEffectList();
        
        // compile a list of new spells applied to player from
        // the list of unordered_set<EffectSetting*> newEffects
        // this is not very accurate, if two spells share a ME and
        // both are applied, this can resolve the wrong spell, but
        // I'm not sure what to do about that. :|
        std::unordered_set<MagicItem*> spells;
        for (auto newEffect : newEffects) {
            for (auto* effect : *effects) {
                if (effect->GetBaseObject() == newEffect) {
                    spells.insert(effect->spell);
                    break;
                }
            }
        }
        newEffects.clear();

        for (auto spell : spells) {
            for (auto& item : globals) {
                if (item.scope == Scope::Current) continue;
                if (!ValidateSpell(item, spell)) continue;

                UpdateGlobalValue(item.global, item.mod);
            }
        }
    }

    void Process() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        auto effects = mt->GetActiveEffectList();
        for (auto& item : globals) {
            float value = 0.0f;
            if (item.scope == Scope::Lifetime) continue;
            std::unordered_set<MagicItem*> visited;
            for (auto* effect : *effects) {
                if (!effect || !effect->spell) continue;
                if (item.unique) {
                    if (visited.contains(effect->spell)) continue;
                    visited.insert(effect->spell);
                }
                if (!ValidateSpell(item, effect->spell)) continue;

                value += 1;
            }
            UpdateGlobalValue(item.global, item.mod, value, true);
        }
        bQueued = false;
    }

    void Tick() {
        if (Utils::IsPaused()) return;
        Process();
    }

    class EventSink : public BSTEventSink<TESMagicEffectApplyEvent> {
        BSEventNotifyControl ProcessEvent(const TESMagicEffectApplyEvent* event,
                                          BSTEventSource<TESMagicEffectApplyEvent>*) {
            if (!event || !event->target || !event->target->IsPlayerRef())
                return BSEventNotifyControl::kContinue;
            
            if (auto* form = TESForm::LookupByID<EffectSetting>(event->magicEffect); form) {
                newEffects.insert(form);
            }
            if (!bQueued) {
                SKSE::GetTaskInterface()->AddTask(Process);
                SKSE::GetTaskInterface()->AddTask(UpdateLifetimeGlobals);
                bQueued = true;
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("activeEffect")) return;
        auto& data = item.at("activeEffect");
        Rule rule;
        rule.mod = ParseValueMod(data);
        if (data.contains("spellType")) {
            rule.spellType = static_cast<MagicSystem::SpellType>(data.at("spellType").get<int>());
        }
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("scope")) {
            std::string_view scope = data.at("scope").get<std::string_view>();
            if (scope == "lifetime"sv) rule.scope = Scope::Lifetime;
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESMagicEffectApplyEvent>(&g_sink);
            // TESActiveEffectApplyRemoveEvent doesn't seem to properly trigger to detect
            // AME being removed, so we poll.
            if (!ticker) {
                ticker = std::make_unique<Ticker>(Tick, std::chrono::milliseconds(100));
                ticker->Start();
            }
        }
    }
}