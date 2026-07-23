#pragma once

namespace S_Hits {
    enum ValueType { Counter = 0, TargetLevel = 1, TargetLevelDiff = 2 };

    struct HitData {
        bool process = false;
        TESObjectREFR* target;
        TESForm* source = nullptr;
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
        std::optional<ConditionFilter> condition;
        TESForm* sourceFilter = nullptr;
        std::optional<bool> sameTarget;
        ValueMod mod{};
        std::optional<bool> sneakAttack;
        std::optional<bool> powerAttack;
        std::optional<bool> bashAttack;
        std::optional<bool> blocked;
        ValueType valueType = ValueType::Counter;
        bool resetOnMismatchHit = false;
        bool actorsOnly = true;
        bool ignoreDead = true;
    };

    std::vector<Rule> hitGlobals;
    std::vector<Rule> hitTakenGlobals;

    TESForm* GetSourceForm(FormID sourceID) {
        auto form = TESForm::LookupByID(sourceID);
        if (form->Is(FormType::Enchantment)) {
            // we need to find the weapon
        }
        return form;
    }

    void _Process(std::vector<Rule>& arr, HitData& hit, TESObjectREFR* lastTarget) {
        for (auto& item : arr) {
            switch (item.valueType) {
                case ValueType::TargetLevel:
                    if (hit.target->IsActor()) item.global->value = hit.target->As<Actor>()->GetLevel();
                    break;

                case ValueType::TargetLevelDiff:
                    if (hit.target->IsActor())
                        item.global->value =
                            static_cast<float>(player->GetLevel() - hit.target->As<Actor>()->GetLevel());
                    break;

                default:
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

                    if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) {
                        matches = false;
                    }
                    if (item.sourceFilter && !Utils::ParseFormFilter(hit.source, item.sourceFilter)) {
                        matches = false;
                        if (item.resetOnMismatchHit) reset = true;
                    }

                    if (item.sameTarget.has_value()) {
                        if (lastTarget && (hit.target == lastTarget) != *item.sameTarget) {
                            matches = false;
                            reset = true;
                        }
                    }

                    if (reset)
                        UpdateGlobalValue(item.global, item.mod, 0.0f);
                    else if (matches)
                        UpdateGlobalValue(item.global, item.mod);
            }
        }
    }

    void Process() {
        if (hitCache.process) {
            _Process(hitGlobals, hitCache, lastHitTarget);
            if (lastHitTarget && lastHitTarget->IsActor()) {
                lastHitTarget->As<Actor>()->RemoveSpell(LastHitSpell);
            }
            hitCache.process = false;
            lastHitTarget = hitCache.target;
            if (lastHitTarget->IsActor()) {
                lastHitTarget->As<Actor>()->AddSpell(LastHitSpell);
            }
        }
        if (hitTakenCache.process) {
            _Process(hitTakenGlobals, hitTakenCache, lastHitTakenTarget);
            hitTakenCache.process = false;
            lastHitTakenTarget = hitTakenCache.target;
        }
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESHitEvent>,
                      public BSTEventSink<CriticalHit::Event>,
                      public BSTEventSink < TESCellAttachDetachEvent> {
        BSEventNotifyControl ProcessEvent(const CriticalHit::Event* event, BSTEventSource<CriticalHit::Event>*) {
            ConsoleLog::GetSingleton()->Print(fmt::format("Crit hit, sneak: {}", event->sneakHit).c_str());
            return BSEventNotifyControl::kContinue;
        }

        BSEventNotifyControl ProcessEvent(const TESHitEvent* event, BSTEventSource<TESHitEvent>*) {
            auto cause = event->cause.get();
            auto target = event->target.get();
            if (cause && target) {
                if (cause->IsPlayerRef()) {
                    // auto av = target->As<Actor>()->AsActorValueOwner();
                    // if (av) ConsoleLog::GetSingleton()->Print(fmt::format("Health: {}", av->GetActorValue(ActorValue::kHealth)).c_str());
                    hitCache.target = target;
                    hitCache.source = GetSourceForm(event->source);
                    hitCache.projectile = event->projectile;
                    hitCache.flags = event->flags;
                    hitCache.process = true;
                } else if (target->IsPlayerRef()) {
                    hitTakenCache.target = cause;
                    hitTakenCache.source = GetSourceForm(event->source);
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

        BSEventNotifyControl ProcessEvent(const TESCellAttachDetachEvent* event,
                                          BSTEventSource<TESCellAttachDetachEvent>*) {
            if (!event || !event->reference) return BSEventNotifyControl::kContinue;
            auto ref = event->reference.get();
            if (ref->IsActor() && ref->As<Actor>()->HasSpell(LastHitSpell)) {
                ref->As<Actor>()->RemoveSpell(LastHitSpell);
            }
            return BSEventNotifyControl::kContinue;
        }
    };

    void parseJSON(const nlohmann::json_abi_v3_12_0::json& item, TESGlobal* global) {
        for (std::string_view key : {"hit"sv, "hitTaken"sv}) {
            if (!item.contains(key)) continue;
            auto& data = item.at(key);
            Rule rule;
            if (data.contains("sourceFilter")) {
                rule.sourceFilter = Utils::GetForm<TESForm>(data.at("sourceFilter").get<std::string>());
                if (!rule.sourceFilter) continue;
            }
            if (data.contains("condition")) {
                rule.condition = ParseConditionFilter(data.at("condition"));
                if (rule.condition == std::nullopt) return;
            }
            if (data.contains("sameTarget")) {
                rule.sameTarget = data.at("sameTarget").get<bool>();
            }
            rule.mod = ParseValueMod(data);
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
            if (data.contains("valueType")) {
                auto type = data.at("valueType").get<std::string>();
                if (type == "TargetLevel") {
                    rule.valueType = ValueType::TargetLevel;
                } else if (type == "TargetLevelDiff") {
                    rule.valueType = ValueType::TargetLevelDiff;
                }
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
            ScriptEventSourceHolder::GetSingleton()->AddEventSink<TESCellAttachDetachEvent>(&g_sink);
        }
    }
}