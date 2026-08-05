#pragma once

namespace S_ActiveEffect {
    bool bQueued;
    std::unique_ptr<Ticker> ticker;

    enum Scope { Current = 0, Lifetime = 1 };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        std::optional<SpellFilter> spellFilter;
        ValueMod mod{};
        bool unique = true;
        Scope scope = Scope::Current;
    };

    std::vector<Rule> globals;
    std::unordered_set<EffectSetting*> newEffects;

    void UpdateLifetimeGlobals() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        auto effects = mt->GetActiveEffectList();
        if (!effects) return;
        
        // compile a list of new spells applied to player from
        // the list of unordered_set<EffectSetting*> newEffects
        // this is not very accurate, if two spells share a ME and
        // both are applied, this can resolve the wrong spell, but
        // I'm not sure what to do about that. :|
        std::unordered_set<MagicItem*> spells;
        for (auto newEffect : newEffects) {
            for (auto* effect : *effects) {
                if (auto base = effect ? effect->GetBaseObject() : nullptr; base) {
                    if (base == newEffect) {
                        spells.insert(effect->spell);
                        break;
                    }
                }
            }
        }
        newEffects.clear();

        for (auto spell : spells) {
            for (auto& item : globals) {
                if (item.scope == Scope::Current) continue;
                if (item.formFilter.has_value() && !ValidateFormFilter(spell, item.formFilter.value())) continue;
                if (item.spellFilter.has_value() && !ValidateSpellFilter(item.spellFilter.value(), spell)) continue;

                UpdateGlobalValue(item.global, item.mod);
            }
        }
    }

    void Process() {
        auto* mt = PlayerCharacter::GetSingleton()->AsMagicTarget();
        if (!mt) return;
        auto effects = mt->GetActiveEffectList();
        for (auto& item : globals) {
            if (item.scope == Scope::Lifetime) continue;
            float value = 0.0f;
            if (effects) {
                std::unordered_set<MagicItem*> visited;
                for (auto* effect : *effects) {
                    if (!effect || !effect->spell) continue;
                    auto spell = effect->spell;
                    if (item.unique) {
                        if (visited.contains(spell)) continue;
                        visited.insert(spell);
                    }
                    if (item.formFilter.has_value() && !ValidateFormFilter(spell, item.formFilter.value())) continue;
                    if (item.spellFilter.has_value() && !ValidateSpellFilter(item.spellFilter.value(), spell)) continue;

                    value += 1;
                }
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
        rule.spellFilter = ParseSpellFilter(data);
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