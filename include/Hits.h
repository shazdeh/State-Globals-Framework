#pragma once

namespace S_Hits {
    struct HitData {
        bool process = false;
        TESObjectREFR* target;
        FormID source;
        FormID projectile;
        REX::EnumSet<TESHitEvent::Flag, std::uint8_t> flags;
    };
    
    bool bQueued = false;
    TESObjectREFR* lastHitTarget = nullptr;
    TESObjectREFR* lastHitTakenTarget = nullptr;
    HitData hitCache;
    HitData hitTakenCache;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<bool> sameTarget;
        bool actorsOnly = true;
        bool ignoreDead = true;
        std::optional<bool> sneakAttack;
        std::optional<bool> powerAttack;
        std::optional<bool> bashAttack;
        std::optional<bool> blocked;
        bool resetOnMismatchHit = false;
        float mod = 1.0f;
    };

    std::vector<Rule> hitGlobals;
    std::vector<Rule> hitTakenGlobals;

    void _Process(std::vector<Rule>& arr, HitData& hit, TESObjectREFR* lastTarget) {
        for (auto& item : arr) {
            if (hit.target->IsActor()) {
                if (hit.target->IsDead() && item.ignoreDead) continue;
            } else if (item.actorsOnly) {
                continue;
            }

            bool reset = false;
            bool matches = true;

            auto checkFlag = [&](const std::optional<bool>& expected, TESHitEvent::Flag flag) {
                if (!expected) return;

                if (hit.flags.all(flag) != *expected) {
                    matches = false;
                    if (item.resetOnMismatchHit) reset = true;
                }
            };

            checkFlag(item.sneakAttack, TESHitEvent::Flag::kSneakAttack);
            checkFlag(item.powerAttack, TESHitEvent::Flag::kPowerAttack);
            checkFlag(item.bashAttack, TESHitEvent::Flag::kBashAttack);
            checkFlag(item.blocked, TESHitEvent::Flag::kHitBlocked);

            if (item.sameTarget.has_value()) {
                if (lastTarget && (hit.target == lastTarget) != *item.sameTarget) {
                    matches = false;
                    reset = true;
                }
            }

            if (reset) item.global->value = 0;

            if (!matches) continue;

            if (item.mod == 0)
                item.global->value = 0;
            else
                item.global->value += item.mod;
        }
    }

    void Process() {
        if (hitCache.process) {
            _Process(hitGlobals, hitCache, lastHitTarget);
            hitCache.process = false;
            lastHitTarget = hitCache.target;
        }
        if (hitTakenCache.process) {
            _Process(hitTakenGlobals, hitTakenCache, lastHitTakenTarget);
            hitTakenCache.process = false;
            lastHitTakenTarget = hitTakenCache.target;
        }
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESHitEvent>, public BSTEventSink<CriticalHit::Event> {
        BSEventNotifyControl ProcessEvent(const CriticalHit::Event* event, BSTEventSource<CriticalHit::Event>*) {
            ConsoleLog::GetSingleton()->Print(fmt::format("Crit hit, sneak: {}", event->sneakHit).c_str());
            return BSEventNotifyControl::kContinue;
        }

        BSEventNotifyControl ProcessEvent(const TESHitEvent* event, BSTEventSource<TESHitEvent>*) {
            auto cause = event->cause.get();
            auto target = event->target.get();
            if (cause && target) {
                if (cause->IsPlayerRef()) {
                    hitCache.target = target;
                    hitCache.source = event->source;
                    hitCache.projectile = event->projectile;
                    hitCache.flags = event->flags;
                    hitCache.process = true;
                } else if (target->IsPlayerRef()) {
                    hitTakenCache.target = cause;
                    hitTakenCache.source = event->source;
                    hitTakenCache.projectile = event->projectile;
                    hitTakenCache.flags = event->flags;
                    hitTakenCache.process = true;
                }
            }
            if (!bQueued) {
                SKSE::GetTaskInterface()->AddTask(Process);
                bQueued = true;
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        for (std::string_view key : {"hit"sv, "hitTaken"sv}) {
            if (!item.contains(key)) continue;
            auto& data = item.at(key);
            Rule rule;
            if (data.contains("sameTarget")) {
                rule.sameTarget = data.at("sameTarget").get<bool>();
            }
            if (data.contains("mod")) {
                rule.mod = data.at("mod").get<float>();
            }
            if (data.contains("actorsOnly")) {
                rule.actorsOnly = data.at("actorsOnly").get<bool>();
            }
            if (data.contains("ignoreDead")) {
                rule.ignoreDead = data.at("ignoreDead").get<bool>();
            }
            if (data.contains("resetOnMismatchHit")) {
                rule.resetOnMismatchHit = data.at("resetOnMismatchHit").get<bool>();
            }
            if (data.contains("powerAttack")) {
                rule.powerAttack = data.at("powerAttack").get<bool>();
            }
            if (data.contains("sneakAttack")) {
                rule.sneakAttack = data.at("sneakAttack").get<bool>();
            }
            if (data.contains("bashAttack")) {
                rule.bashAttack = data.at("bashAttack").get<bool>();
            }
            if (data.contains("blocked")) {
                rule.blocked = data.at("blocked").get<bool>();
            }
            rule.global = global;
            (key == "hit" ? hitGlobals : hitTakenGlobals).push_back(rule);
        }
    }

    void SetupEvents() {
        if (!hitGlobals.empty() || !hitTakenGlobals.empty()) {
            static EventSink g_sink;
            CriticalHit::GetEventSource()->AddEventSink(&g_sink);
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESHitEvent>(&g_sink);
        }
    }
}