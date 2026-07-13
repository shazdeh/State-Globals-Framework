#pragma once

namespace S_SpellLearn {
    struct Rule {
        TESGlobal* global = nullptr;
        TESForm* formFilter = nullptr;
        bool keywordMatchAll = false;
        std::optional<int> skillLevel;
        std::string skillComp = "=";
        ActorValue av = ActorValue::kNone;
        float mod = 1.0f;
    };

    std::vector<Rule> globals;

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
            if (item.skillLevel.has_value() &&
                !Utils::compare(skillLevel, item.skillLevel.value(), item.skillComp))
                continue;
            if (item.av != ActorValue::kNone && assocSkill != item.av) continue;
            if (item.formFilter && !Utils::ParseFormFilter(spell, item.formFilter)) continue;

            item.global->value += item.mod;
        }
    }

    void Process() {
        auto player = PlayerCharacter::GetSingleton();
        const auto& all = TESDataHandler::GetSingleton()->GetFormArray<TESObjectBOOK>();

        for (auto& item : globals) {
            item.global->value = 0;
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

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("learnspell")) return;
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
        if (data.contains("formFilter")) {
            rule.formFilter = Utils::GetForm<TESForm>(data.at("formFilter").get<std::string>());
            if (!rule.formFilter) return;
        }
        rule.global = global;
        globals.push_back(rule);
    };

    void SetupEvents() {
        if (!globals.empty()) {
            static EventSink spellSink;
            SpellsLearned::GetEventSource()->AddEventSink<SpellsLearned::Event>(&spellSink);
        }
    }
}