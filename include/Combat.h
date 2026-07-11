#pragma once

namespace S_Combat {
    Actor* player;
    std::unique_ptr<Ticker> ticker;
    bool inCombat;

    struct Rule {
        float mod = 1.0f;
    };

    std::unordered_map<TESGlobal*, Rule> startGlobals;
    std::unordered_map<TESGlobal*, Rule> endGlobals;

    void CombatStart() {
        for (auto& item : startGlobals) {
            if (item.second.mod == 0) {
                item.first->value = 0;
            } else {
                item.first->value += item.second.mod;
            }
        }
    }

    void CombatEnd() {
        for (auto& item : endGlobals) {
            if (item.second.mod == 0) {
                item.first->value = 0;
            } else {
                item.first->value += item.second.mod;
            }
        }
    }

    void Tick() {
        if (inCombat && !player->IsInCombat()) {
            CombatEnd();
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
                if (data.contains("mod")) {
                    rule.mod = data.at("mod").get<float>();
                }
                if (key == "combatstart"sv) {
                    startGlobals.insert({global, rule});
                } else {
                    endGlobals.insert({global, rule});
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