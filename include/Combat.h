#pragma once

namespace S_Combat {
    Actor* player;
    std::unique_ptr<Ticker> ticker;
    bool inCombat;

    struct Rule {
        TESGlobal* global;
        ValueMod mod{};
    };

    std::vector<Rule> startGlobals;
    std::vector<Rule> endGlobals;

    void CombatStart() {
        for (auto& item : startGlobals) {
            UpdateGlobalValue(item.global, item.mod);
        }
    }

    void CombatEnd() {
        for (auto& item : endGlobals) {
            UpdateGlobalValue(item.global, item.mod);
        }
    }

    void Tick() {
        if (Utils::IsPaused()) return;
        if (inCombat && !player->IsInCombat()) {
            CombatEnd();
            S_Hits::CombatEnd();
            inCombat = false;
        } else if (!inCombat && player->IsInCombat()) {
            CombatStart();
            inCombat = true;
        }
    }

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        for (std::string_view key : {"combatstart"sv, "combatend"sv}) {
            if (item.contains(key)) {
                auto& data = item.at(key);
                Rule rule;
                rule.mod = ParseValueMod(data);
                rule.global = global;
                if (key == "combatstart"sv) {
                    startGlobals.push_back(rule);
                } else {
                    endGlobals.push_back(rule);
                }
            }
        }
    };

    void SetupEvents() {
        if (startGlobals.empty() && endGlobals.empty()) return;
        // TESCombatEvent does not work for player, and getting player.IsInCombat()
        // after it fires gives incorrect results. We poll.
        player = PlayerCharacter::GetSingleton();
        if (!ticker) {
            ticker = std::make_unique<Ticker>(Tick, std::chrono::milliseconds(100));
            ticker->Start();
        }
    }
}