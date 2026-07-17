struct ValueMod {
    float value = 1.0;
    std::optional<float> min;
    std::optional<float> max;
    bool resetOnMin = false;
    bool resetOnMax = false;
};

ValueMod ParseValueMod(const nlohmann::json_abi_v3_12_0::json& item) {
    ValueMod mod;
    if (item.contains("mod")) {
        auto& data = item.at("mod");
        if (data.contains("value")) mod.value = data.at("value").get<float>();
        if (data.contains("min")) mod.min = data.at("min").get<float>();
        if (data.contains("max")) mod.max = data.at("max").get<float>();
        if (data.contains("resetOnMin")) mod.resetOnMin = data.at("resetOnMin").get<bool>();
        if (data.contains("resetOnMax")) mod.resetOnMax = data.at("resetOnMax").get<bool>();
    }
    return mod;
}

void UpdateGlobalValue(TESGlobal* global, ValueMod& mod, float fMult = 1.0f) {
    // ConsoleLog::GetSingleton()->Print(fmt::format("global: {}, modvalue: {} with mult: {}", clib_util::editorID::get_editorID(global), mod.value, fMult).c_str());
    if (mod.value == 0.0f || fMult == 0.0f) {
        global->value = 0;
    } else {
        float baseValue = mod.value * fMult;
        float newValue = global->value += baseValue;
        if (mod.max.has_value() && newValue >= mod.max.value()) {
            if (mod.resetOnMax)
                newValue = 0;
            else
                newValue = mod.max.value();
        } else if (mod.min.has_value() && newValue <= mod.min.value()) {
            if (mod.resetOnMin)
                newValue = 0;
            else
                newValue = mod.min.value();
        }
        global->value = newValue;
    }
}