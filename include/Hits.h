#pragma once

namespace S_Hits {
    enum ValueType { Counter = 0, TargetLevel = 1, TargetLevelDiff = 2 };
    enum WeaponType {
        None,
        Melee,
        Ranged,
        Unarmed,
        OneHanded,
        TwoHanded,
        OneHandedSword,
        OneHandedDagger,
        OneHandedAxe,
        OneHandedMace,
        TwoHandedSword,
        TwoHandedAxe,
        Bow,
        Crossbow,
        Staff
    };

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
    bool lastHitWasCrit = false;
    bool lastHitWasSneakCrit = false;

    struct Rule {
        TESGlobal* global = nullptr;
        std::optional<ConditionFilter> condition;
        std::optional<FormFilter> sourceFilter;
        std::optional<bool> sameTarget;
        ValueMod mod{};
        std::optional<bool> sneakAttack;
        std::optional<bool> powerAttack;
        std::optional<bool> bashAttack;
        std::optional<bool> blocked;
        std::optional<bool> isBound;
        WeaponType weaponType = WeaponType::None;
        std::optional<bool> crit;
        std::optional<bool> sneakCrit;
        std::optional<ActorValue> weaponSkill; // @todo: this should be a vector
        ValueType valueType = ValueType::Counter;
        bool resetOnMismatchHit = false;
        bool actorsOnly = true;
        bool ignoreDead = true;
        bool ignoreTeammate = true;
    };

    struct ConcentrationThrottle {
        FormID spellID;
        FormID targetID;
        std::uint32_t expiresAtMS = 0;
    };

    std::vector<Rule> hitGlobals;
    std::vector<Rule> hitTakenGlobals;
    inline std::vector<ConcentrationThrottle> concentrationThrottle;
    inline float fThrottleTime = 1.0f;

    // Concentration spells spam the TESHitEvent
    // we limit the trigger to 1/s per target
    bool MustThrottleConcentration(HitData& hit) {
        auto* spell = hit.source->As<MagicItem>();
        if (!spell || spell->GetCastingType() != MagicSystem::CastingType::kConcentration) return false;
        const auto now = RE::BSTimer::GetSingleton()->runTimeMS;
        const auto duration = static_cast<std::uint32_t>(fThrottleTime * 1000.0f);
        auto it = std::find_if(concentrationThrottle.begin(), concentrationThrottle.end(),
                               [&](const ConcentrationThrottle& e) {
                                   return e.spellID == spell->GetFormID() && e.targetID == hit.target->GetFormID();
                               });
        if (it != concentrationThrottle.end()) {
            if (static_cast<std::int32_t>(it->expiresAtMS - now) > 0) {
                return true;
            }
            it->expiresAtMS = now + duration;
            return false;
        }
        concentrationThrottle.push_back({spell->GetFormID(), hit.target->GetFormID(), now + duration});

        return false;
    }

    bool MatchWeaponType(TESObjectWEAP* weapon, WeaponType type) {
        switch (type) {
            case WeaponType::Melee:
                return weapon->IsMelee();
            case WeaponType::Ranged:
                return weapon->IsRanged();
            case WeaponType::Unarmed:
                return weapon->IsHandToHandMelee();
            case WeaponType::OneHanded:
                return weapon->IsOneHandedAxe() || weapon->IsOneHandedDagger() || weapon->IsOneHandedMace() || weapon->IsOneHandedSword();
            case WeaponType::TwoHanded:
                return weapon->IsTwoHandedAxe() || weapon->IsTwoHandedSword(); // missing mace
            case WeaponType::OneHandedSword:
                return weapon->IsOneHandedSword();
            case WeaponType::OneHandedDagger:
                return weapon->IsOneHandedDagger();
            case WeaponType::OneHandedAxe:
                return weapon->IsOneHandedAxe();
            case WeaponType::OneHandedMace:
                return weapon->IsOneHandedMace();
            case WeaponType::TwoHandedSword:
                return weapon->IsTwoHandedSword();
            case WeaponType::TwoHandedAxe:
                return weapon->IsTwoHandedAxe();
            case WeaponType::Bow:
                return weapon->IsBow();
            case WeaponType::Crossbow:
                return weapon->IsCrossbow();
            case WeaponType::Staff:
                return weapon->IsStaff();
        }
        return false;
    }

    void _Process(std::vector<Rule>& arr, HitData& hit, TESObjectREFR* lastTarget) {
        if (MustThrottleConcentration(hit)) return;

        if (bLogIDs)
            ConsoleLog::GetSingleton()->Print( fmt::format("Hit Event! Source: {:x} : {}, Projectile: {:x}", hit.source->GetFormID(), clib_util::editorID::get_editorID(hit.source), hit.projectile).c_str());

        Actor* targetActor = hit.target->As<Actor>();
        TESObjectWEAP* sourceWeapon = hit.source->As<TESObjectWEAP>();

        for (auto& item : arr) {
            switch (item.valueType) {
                case ValueType::TargetLevel:
                    if (targetActor) item.global->value = targetActor->GetLevel();
                    break;

                case ValueType::TargetLevelDiff:
                    if (targetActor)
                        item.global->value = static_cast<float>(player->GetLevel() - targetActor->GetLevel());
                    break;

                default:
                    if (targetActor) {
                        if (item.ignoreDead && targetActor->IsDead()) continue;
                        if (item.ignoreTeammate && targetActor->IsPlayerTeammate()) continue;
                    } else if (item.actorsOnly) {
                        continue;
                    }

                    bool reset = false;
                    bool matches = true;
                    float resetValue = 0.0f;

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

                    // crits
                    if (item.crit.has_value() && lastHitWasCrit != item.crit.value()) {
                        matches = false;
                        if (item.resetOnMismatchHit) reset = true;
                    }
                    if (item.sneakCrit.has_value() && lastHitWasSneakCrit != item.sneakCrit.value()) {
                        matches = false;
                        if (item.resetOnMismatchHit) reset = true;
                    }

                    if (item.condition.has_value() && !ValidateConditionForm(item.condition.value())) {
                        matches = false;
                    }
                    if (item.sourceFilter.has_value() && !ValidateFormFilter(hit.source, item.sourceFilter.value())) {
                        matches = false;
                        if (item.resetOnMismatchHit) reset = true;
                    }

                    // weapon flags
                    if (
                        (item.weaponType != WeaponType::None && (!sourceWeapon || !MatchWeaponType(sourceWeapon, item.weaponType))) ||
                        (item.isBound.has_value() &&
                         (!sourceWeapon || sourceWeapon->IsBound() != item.isBound.value()))
                    ) {
                        matches = false;
                        if (item.resetOnMismatchHit) reset = true;
                    }

                    // weapon skill
                    if (item.weaponSkill.has_value() &&
                        (!sourceWeapon || !sourceWeapon->weaponData.skill.all(item.weaponSkill.value()))) {
                        matches = false;
                        if (item.resetOnMismatchHit) reset = true;
                    }

                    if (item.sameTarget.has_value()) {
                        if (lastTarget && (hit.target == lastTarget) != *item.sameTarget) {
                            // messy but what it does: if every other condition was matched without an issue but it's not
                            // the same target, a hit has landed still, so the value should reset to 1 and not 0.
                            if (matches && !reset) {
                                resetValue = 1.0f;
                            }
                            matches = false;
                            reset = true;
                        }
                    }

                    if (reset)
                        UpdateGlobalValue(item.global, item.mod, resetValue, true);
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
        lastHitWasCrit = false;
        lastHitWasSneakCrit = false;
        bQueued = false;
    }

    class EventSink : public BSTEventSink<TESHitEvent>,
                      public BSTEventSink<CriticalHit::Event>,
                      public BSTEventSink < TESCellAttachDetachEvent> {
        BSEventNotifyControl ProcessEvent(const CriticalHit::Event* event, BSTEventSource<CriticalHit::Event>*) {
            if (event->aggressor->IsPlayerRef()) {
                if (bLogIDs) ConsoleLog::GetSingleton()->Print(fmt::format("Crit Hit, weapon: {}, sneak: {}", clib_util::editorID::get_editorID(event->weapon), event->sneakHit).c_str());
                lastHitWasCrit = true;
                if (event->sneakHit) lastHitWasSneakCrit = true;
            }
            return BSEventNotifyControl::kContinue;
        }

        // TESHitEvent is an inconsistent mess
        // 1. For enchanted/poisoned melee weapons it triggers multiple times, but the event.source is always
        //    the WEAP form. In contrast, for enchanted/poisoned bows event.source points to the actual source (ENCH, ALCH or WEAP form)
        // 2. On some stuff (like Calm spell) doesn't trigger, possibly the Harmful tag is required?
        // 3. For staves, event.source is Unarmed, and event.projectile is empty too
        // 4. event.projectile is not sent for unenchanted bows
        BSEventNotifyControl ProcessEvent(const TESHitEvent* event, BSTEventSource<TESHitEvent>*) {
            auto cause = event->cause.get();
            auto target = event->target.get();
            if (cause && target && (cause->IsPlayerRef() || target->IsPlayerRef())) {
                auto sourceForm = TESForm::LookupByID(event->source);
                if (!sourceForm) return BSEventNotifyControl::kContinue;

                HitData temp;
                temp.target = cause->IsPlayerRef() ? target : cause;
                temp.source = sourceForm;
                temp.flags = event->flags;
                temp.process = true;
                temp.projectile = event->projectile;
                (cause->IsPlayerRef() ? hitCache : hitTakenCache) = std::move(temp);
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
            if (ref && ref->IsActor() && ref->As<Actor>()->HasSpell(LastHitSpell)) {
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
                rule.sourceFilter = ParseFormFilter(data.at("sourceFilter"));
                if (rule.sourceFilter == std::nullopt) return;
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
            if (data.contains("ignoreTeammate")) {
                rule.ignoreTeammate = data.at("ignoreTeammate").get<bool>();
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
            if (data.contains("isBound")) {
                rule.isBound = data.at("isBound").get<bool>();
            }
            if (data.contains("weaponType")) {
                auto value = data.at("weaponType").get<std::string>();
                if (value == "melee") {
                    rule.weaponType = WeaponType::Melee;
                } else if (value == "ranged") {
                    rule.weaponType = WeaponType::Ranged;
                } else if (value == "unarmed") {
                    rule.weaponType = WeaponType::Unarmed;
                } else if (value == "oneHanded") {
                    rule.weaponType = WeaponType::OneHanded;
                } else if (value == "twoHanded") {
                    rule.weaponType = WeaponType::TwoHanded;
                } else if (value == "oneHandedSword") {
                    rule.weaponType = WeaponType::OneHandedSword;
                } else if (value == "oneHandedDagger") {
                    rule.weaponType = WeaponType::OneHandedDagger;
                } else if (value == "oneHandedAxe") {
                    rule.weaponType = WeaponType::OneHandedAxe;
                } else if (value == "oneHandedMace") {
                    rule.weaponType = WeaponType::OneHandedMace;
                } else if (value == "twoHandedSword") {
                    rule.weaponType = WeaponType::TwoHandedSword;
                } else if (value == "twoHandedAxe") {
                    rule.weaponType = WeaponType::TwoHandedAxe;
                } else if (value == "bow") {
                    rule.weaponType = WeaponType::Bow;
                } else if (value == "crossbow") {
                    rule.weaponType = WeaponType::Crossbow;
                } else if (value == "staff") {
                    rule.weaponType = WeaponType::Staff;
                }
            }
            if (data.contains("crit")) {
                rule.crit = data.at("crit").get<bool>();
            }
            if (data.contains("sneakCrit")) {
                rule.sneakCrit = data.at("sneakCrit").get<bool>();
            }
            if (data.contains("weaponSkill")) {
                rule.weaponSkill = static_cast<ActorValue>(data.at("weaponSkill").get<int>());
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

    // when combat ends, cleanup the concentrationThrottle
    void CombatEnd() {
        const auto now = RE::BSTimer::GetSingleton()->runTimeMS;
        std::erase_if(concentrationThrottle, [&](const ConcentrationThrottle& e) {
            return static_cast<std::int32_t>(now - e.expiresAtMS) >= 0;
        });
    }

    TESObjectREFR* GetLastHitTarget(StaticFunctionTag*) {
        return lastHitTarget;
    }

    TESObjectREFR* GetLastHitTakenTarget(StaticFunctionTag*) {
        return lastHitTarget;
    }

    void OnLoadGame() {
        lastHitTarget = nullptr;
        lastHitTakenTarget = nullptr;
    }
}