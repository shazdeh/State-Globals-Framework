#pragma once

namespace SpellLearn {
    struct Rule {
        int skillLevel = -1;
        std::string skillComp = "=";
        ActorValue av = ActorValue::kNone;
        float mod = 1.0f;
    };

    class EventSink : public BSTEventSink<SpellsLearned::Event> {
        BSEventNotifyControl ProcessEvent(const SpellsLearned::Event* event, BSTEventSource<SpellsLearned::Event>*) {
            if (event && event->spell) {
                UpdateSpellLearnGlobs(event->spell);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    int GetSpellMinimumSkillLevel(SpellItem* a_spell) {
        if (!a_spell) return 0;
        int min = 0;
        for (auto effect : a_spell->effects) {
            auto minLevel = effect->baseEffect->GetMinimumSkillLevel();
            if (minLevel > min) {
                min = minLevel;
            }
        }

        return min;
    }

    struct LearnSpell {
        static Rule parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
            auto& data = item.at("learnspell");
            Rule rule;
            if (data.contains("skillLevel")) {
                rule.skillLevel = data.at("skillLevel").get<int>();
                if (data.contains("skillComp")) {
                    rule.skillComp = data.at("skillComp").get<std::string>();
                }
            }
            if (data.contains("skill")) {
                auto skill = data.at("skill").get<int>();
                switch (skill) {
                    case 18:
                        rule.av = ActorValue::kAlteration;
                        break;
                    case 19:
                        rule.av = ActorValue::kConjuration;
                        break;
                    case 20:
                        rule.av = ActorValue::kDestruction;
                        break;
                    case 21:
                        rule.av = ActorValue::kIllusion;
                        break;
                    case 22:
                        rule.av = ActorValue::kRestoration;
                        break;
                    default:
                        return {};
                }
            }
            return rule;
        }
    };

    void UpdateSpellLearnGlobs(SpellItem* spell) {
        // auto assocSkill = spell->GetAssociatedSkill();
        // int skillLevel = GetSpellMinimumSkillLevel(spell);

        // for (auto& item : spellLearnGlobals) {
        //  if (item.second.skillLevel != -1 && !compare(skillLevel, item.second.skillLevel, item.second.skillComp))
        //      continue;
        // if (item.second.av != ActorValue::kNone && assocSkill != item.second.av) continue;
        // item.first->value += item.second.mod;
        //}
    }

    void SetupEvents() {
        static EventSink spellSink;
        SpellsLearned::GetEventSource()->AddEventSink(&spellSink);
    }
}