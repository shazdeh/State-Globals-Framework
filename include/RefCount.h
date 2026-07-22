#pragma once

namespace S_RefCount {
    std::unique_ptr<Ticker> ticker;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<FormFilter> formFilter;
        ValueMod mod{};
        float distance = 0;
        std::optional<bool> hostile;
        std::string levelComp;
    };

    std::vector<Rule> globals;

    void Tick() {
        if (Utils::IsPaused()) return;
        if (const auto& tes = TES::GetSingleton(); tes) {
            for (auto& item : globals) {
                float value = 0.0f;
                tes->ForEachReferenceInRange(player, item.distance, [&item, &value](TESObjectREFR* ref) {
                    if (!ref || ref->IsDisabled() || ref->IsDeleted() || ref->IsPlayerRef())
                        return BSContainer::ForEachResult::kContinue;
                    auto base = ref->GetBaseObject();
                    if (ref->IsActor()) {
                        Actor* actor = ref->As<Actor>();
                        if (item.hostile.has_value() && actor->IsHostileToActor(player) != item.hostile.value())
                            return BSContainer::ForEachResult::kContinue;
                        if (!item.levelComp.empty() &&
                            !Utils::compare(player->GetLevel(), actor->GetLevel(), item.levelComp))
                            return BSContainer::ForEachResult::kContinue;
                    }
                    if (item.formFilter.has_value() && !ValidateFormFilter(base, item.formFilter.value()))
                        return BSContainer::ForEachResult::kContinue;

                    value += 1;
                    return BSContainer::ForEachResult::kContinue;
                });
                UpdateGlobalValue(item.global, item.mod);
            }
        }
    }

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        if (!item.contains("refCount")) return;
        auto& data = item.at("refCount");
        Rule rule;
        rule.mod = ParseValueMod(data);
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
        if (data.contains("level")) {
            rule.levelComp = data.at("level").get<std::string>();
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