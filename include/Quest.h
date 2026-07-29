#pragma once

namespace S_Quest {
    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
    };

    std::vector<Rule> startGlobals;
    std::vector<Rule> failGlobals;
    std::vector<Rule> completeGlobals;
    std::vector<Rule> stageGlobals;

    void Process(TESQuest* a_quest, std::vector<Rule>& arr) {
        for (auto& item : arr) {
            if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) continue;
            if (item.formFilter.has_value() && !ValidateFormFilter(a_quest, item.formFilter.value())) continue;

            UpdateGlobalValue(item.global, item.mod);
        }
    }

    class EventSink : public BSTEventSink<TESQuestStartStopEvent> {
        BSEventNotifyControl ProcessEvent(const TESQuestStartStopEvent* event,
                                          BSTEventSource<TESQuestStartStopEvent>*) {
            if (!event) return BSEventNotifyControl::kContinue;
            if (auto form = TESForm::LookupByID<TESQuest>(event->formID); form) {
                if (event->started) {
                    Process(form, startGlobals);
                } else {
                    if (form->data.flags.all(QuestFlag::kFailed)) {
                        Process(form, failGlobals);
                    } else {
                        Process(form, completeGlobals);
                    }
                }
            }

            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        int i = -1;
        for (std::string_view key : {"questStart"sv, "questFail"sv, "questComplete"sv, "questStage"sv}) {
            i++;
            if (!item.contains(key)) continue;
            auto& data = item.at(key);
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
            rule.global = global;
            switch (i) {
                case 0:
                    startGlobals.push_back(rule);
                    break;
                case 1:
                    failGlobals.push_back(rule);
                    break;
                case 2:
                    completeGlobals.push_back(rule);
                    break;
                case 3:
                    stageGlobals.push_back(rule);
                    break;
            }
        }
    };

    void SetupEvents() {
        if (!startGlobals.empty() || !failGlobals.empty() || !completeGlobals.empty()) {
            static EventSink g_sink;
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESQuestStartStopEvent>(&g_sink);
        }
    }
}