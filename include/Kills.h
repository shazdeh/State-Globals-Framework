#pragma once

namespace Kills {
    enum ValueType { Counter = 0, TargetLevel = 1, TargetLevelDiff = 2 };

    struct Rule {
        std::vector<TESFaction*> factions;
        std::vector<TESRace*> races;
        BGSPerk* conditionPerk = nullptr;
        int isCommanded = -1;
        ValueType type = ValueType::Counter;
        float mod = 1.0f;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    bool IsInAnyFaction(Actor* a_actor, std::vector<TESFaction*>& a_factions) {
        for (auto* faction : a_factions) {
            if (a_actor->IsInFaction(faction)) return true;
        }
        return false;
    }

    bool IsAnyRace(Actor* a_actor, std::vector<TESRace*>& a_races) {
        auto victimRace = a_actor->GetRace();
        for (auto* race : a_races) {
            if (race == victimRace) return true;
        }
        return false;
    }

    void Process(Actor* victim) {
        for (auto& item : globals) {
            if (item.second.isCommanded != -1 && victim->IsCommandedActor() != item.second.isCommanded) continue;
            if (!item.second.factions.empty() && !IsInAnyFaction(victim, item.second.factions)) continue;
            if (!item.second.races.empty() && !IsAnyRace(victim, item.second.races)) continue;
            if (item.second.conditionPerk &&
                !item.second.conditionPerk->perkConditions.IsTrue(PlayerCharacter::GetSingleton(), victim))
                continue;

            switch (item.second.type) {
                case ValueType::Counter:
                    if (item.second.mod == 0) {
                        item.first->value = 0;
                    } else {
                        item.first->value += item.second.mod;
                    }
                    break;
                case ValueType::TargetLevel:
                    item.first->value = victim->GetLevel();
                    break;
                case ValueType::TargetLevelDiff:
                    item.first->value = PlayerCharacter::GetSingleton()->GetLevel() - victim->GetLevel();
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

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
        auto& data = item.at("kill");
        Rule rule;
        if (data.contains("faction")) {
            if (!Utils::fillFormsArray(data.at("faction"), rule.factions)) return {};
        }
        if (data.contains("race")) {
            if (!Utils::fillFormsArray(data.at("race"), rule.races)) return {};
        }
        if (data.contains("conditionPerk")) {
            rule.conditionPerk = Utils::GetForm<BGSPerk>(data.at("conditionPerk").get<std::string>());
            if (!rule.conditionPerk) {
                return {};
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
        return rule;
    }

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESDeathEvent>(&g_sink);
        }
    }
}