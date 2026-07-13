#pragma once

namespace S_Kills {
    enum ValueType { Counter = 0, TargetLevel = 1, TargetLevelDiff = 2 };

    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* formFilter = nullptr;
        BGSPerk* conditionPerk = nullptr;
        int isCommanded = -1;
        ValueType type = ValueType::Counter;
        float mod = 1.0f;
    };

    std::vector<Rule> globals;

    void Process(Actor* victim) {
        for (auto& item : globals) {
            if (item.isCommanded != -1 && victim->IsCommandedActor() != item.isCommanded) continue;
            if (item.formFilter && !Utils::ParseActorFilter(victim, item.formFilter)) continue;
            if (item.conditionPerk &&
                !item.conditionPerk->perkConditions.IsTrue(PlayerCharacter::GetSingleton(), victim))
                continue;

            switch (item.type) {
                case ValueType::Counter:
                    if (item.mod == 0) {
                        item.global->value = 0;
                    } else {
                        item.global->value += item.mod;
                    }
                    break;
                case ValueType::TargetLevel:
                    item.global->value = victim->GetLevel();
                    break;
                case ValueType::TargetLevelDiff:
                    item.global->value = PlayerCharacter::GetSingleton()->GetLevel() - victim->GetLevel();
                    break;
            }
        }
    }

    class EventSink : public BSTEventSink<TESDeathEvent> {
        BSEventNotifyControl ProcessEvent(const TESDeathEvent* event, BSTEventSource<TESDeathEvent>*) {
            if (!event || !event->actorKiller || !event->actorKiller->IsPlayerRef() ||
                event->dead  // this event fires twice, before and after their death
            )
                return BSEventNotifyControl::kContinue;
            if (auto victimRef = event->actorDying.get(); victimRef) {
                Actor* victim = victimRef->As<Actor>();
                if (victim) Process(victim);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("kill")) return;
        auto& data = item.at("kill");
        Rule rule;
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
        }
        if (data.contains("conditionPerk")) {
            rule.conditionPerk = Utils::GetForm<BGSPerk>(data.at("conditionPerk").get<std::string>());
            if (!rule.conditionPerk) {
                return;
            }
        }
        if (data.contains("commanded")) {
            rule.isCommanded = data.at("commanded").get<bool>() ? 1 : 0;
        }
        if (data.contains("value")) {
            auto value = data.at("value").get<std::string>();
            if (value == "TargetLevel") {
                rule.type = ValueType::TargetLevel;
            } else if (value == "TargetLevelDiff") {
                rule.type = ValueType::TargetLevelDiff;
            }
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
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESDeathEvent>(&g_sink);
        }
    }
}