#pragma once

namespace S_Proximity {
    std::unique_ptr<Ticker> ticker;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        float distance = 0;
        std::optional<bool> hostile;
    };

    std::vector<Rule> globals;

    void Tick() {
        if (const auto& tes = TES::GetSingleton(); tes) {
            Actor* player = PlayerCharacter::GetSingleton();
            for (auto& item : globals) {
                item.global->value = 0.0f;
                tes->ForEachReferenceInRange(player, item.distance, [&item, player](TESObjectREFR* ref) {
                    if (!ref || ref->IsDisabled() || ref->IsDeleted() || ref->IsPlayerRef())
                        return BSContainer::ForEachResult::kContinue;
                    auto base = ref->GetBaseObject();
                    if (ref->IsActor()) {
                        Actor* actor = ref->As<Actor>();
                        if (item.hostile.has_value() && actor->IsHostileToActor(player) != item.hostile.value())
                            return BSContainer::ForEachResult::kContinue;
                    }
                    if (item.formFilter.has_value() && !ValidateFormFilter(base, item.formFilter.value())) return BSContainer::ForEachResult::kContinue;

                    item.global->value += 1;
                    return BSContainer::ForEachResult::kContinue;
                });
            }
        }
    }

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("proximity")) return;
        auto& data = item.at("proximity");
        Rule rule;
        if (data.contains("formFilter")) {
            rule.formFilter = ParseFormFilter(data.at("formFilter"));
            if (rule.formFilter == std::nullopt) return;
        }
        if (data.contains("distance")) {
            rule.distance = data.at("distance").get<float>();
        }
        if (data.contains("hostile")) {
            rule.hostile = data.at("hostile").get<bool>();
        }
        rule.global = global;
        globals.push_back(rule);
    }

    void SetupEvents() {
        if (!globals.empty()) {
            if (!ticker) {
                ticker = std::make_unique<Ticker>(Tick, std::chrono::milliseconds(100));
                ticker->Start();
            }
        }
    }
}