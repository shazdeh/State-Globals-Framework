#pragma once

namespace S_Kills {
    enum ValueType { Counter = 0, TargetLevel = 1, TargetLevelDiff = 2 };

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        std::optional<bool> isCommanded;
        ValueType type = ValueType::Counter;
        ValueMod mod{};
    };

    std::vector<Rule> globals;

    void Process(Actor* victim) {
        for (auto& item : globals) {
            if (item.isCommanded.has_value() && victim->IsCommandedActor() != item.isCommanded) continue;
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(victim, item.formFilter.value())) continue;

            switch (item.type) {
                case ValueType::Counter:
                    UpdateGlobalValue(item.global, item.mod);
                    break;
                case ValueType::TargetLevel:
                    item.global->value = victim->GetLevel();
                    break;
                case ValueType::TargetLevelDiff:
                    item.global->value = static_cast<float>(player->GetLevel() - victim->GetLevel());
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
        rule.mod = ParseValueMod(data);
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("condition")) {
            rule.condition = ParseConditionFilter(data.at("condition"));
            if (rule.condition == std::nullopt) return;
        }
        if (data.contains("commanded")) {
            rule.isCommanded = data.at("commanded").get<bool>();
        }
        if (data.contains("value")) {
            auto value = data.at("value").get<std::string>();
            if (value == "TargetLevel") {
                rule.type = ValueType::TargetLevel;
            } else if (value == "TargetLevelDiff") {
                rule.type = ValueType::TargetLevelDiff;
            }
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