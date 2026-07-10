#pragma once

namespace S_SpellLearn {
    struct Rule {
        std::vector<BGSKeyword*> keywords;
        bool keywordMatchAll = false;
        std::optional<int> skillLevel;
        std::string skillComp = "=";
        ActorValue av = ActorValue::kNone;
        float mod = 1.0f;
    };

    std::unordered_map<TESGlobal*, Rule> globals;

    int GetSpellMinimumSkillLevel(SpellItem* a_spell) {
        if (!a_spell) return 0;
        int min = 0;
        auto effect = a_spell->GetCostliestEffectItem();
        if (effect) min = effect->baseEffect->GetMinimumSkillLevel();

        return min;
    }

    void UpdateGlobals(SpellItem* spell) {
        auto assocSkill = spell->GetAssociatedSkill();
        int skillLevel = GetSpellMinimumSkillLevel(spell);

        for (auto& item : globals) {
            if (item.second.skillLevel.has_value() &&
                !Utils::compare(skillLevel, item.second.skillLevel.value(), item.second.skillComp))
                continue;
            if (item.second.av != ActorValue::kNone && assocSkill != item.second.av) continue;
            if (!item.second.keywords.empty() &&
                !spell->HasKeywordInArray(item.second.keywords, item.second.keywordMatchAll))
                continue;

            item.first->value += item.second.mod;
        }
    }

    void Process() {
        auto player = PlayerCharacter::GetSingleton();
        const auto& all = TESDataHandler::GetSingleton()->GetFormArray<TESObjectBOOK>();

        for (auto& item : globals) {
            item.first->value = 0;
        }

        for (auto book : all) {
            if (!book->TeachesSpell()) continue;
            auto spell = book->GetSpell();
            if (!spell || !player->HasSpell(spell)) continue;
            UpdateGlobals(spell);
        }
    }

    class EventSink : public BSTEventSink<SpellsLearned::Event> {
        BSEventNotifyControl ProcessEvent(const SpellsLearned::Event* event, BSTEventSource<SpellsLearned::Event>*) override {
            Process();
            // the SpellsLearned::Event is buggy and event.spell contains invalid data
            // so this is not used atm and instead we loop all spells
            // UpdateGlobals(event->spell);
            return BSEventNotifyControl::kContinue;
        }
    };

    static std::optional<Rule> parseJSON(const nlohmann::json_abi_v3_12_0::json& item) {
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
            rule.av = static_cast<ActorValue>(skill);
        }
        if (data.contains("keyword")) {
            if (!Utils::fillFormsArray(data.at("keyword"), rule.keywords)) return {};
            if (data.contains("keywordMatchAll")) {
                rule.keywordMatchAll = data.at("keywordMatchAll").get<bool>();
            }
        }
        return rule;
    };

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink spellSink;
            SpellsLearned::GetEventSource()->AddEventSink<SpellsLearned::Event>(&spellSink);
        }
    }
}