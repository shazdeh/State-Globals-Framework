#pragma once

#include "CLibUtilsQTR/FormReader.hpp"

namespace Utils {
    template <typename T>
    T* GetForm(const std::string& a_id) {
        auto id = FormReader::GetFormEditorIDFromString(a_id);
        if (id) {
            return TESForm::LookupByID<T>(id);
        }
        return nullptr;
    }

    template <typename T>
    bool compare(T a, T b, const std::string& op) {
        if (op == "=" || op == "==") return a == b;
        if (op == "<") return a < b;
        if (op == "<=") return a <= b;
        if (op == ">") return a > b;
        if (op == ">=") return a >= b;
        if (op == "!=") return a != b;
        return false;
    }

    template <typename T>
    bool fillFormsArray(const nlohmann::json_abi_v3_12_0::json& data, std::vector<T*>& arr) {
        if (data.is_array()) {
            for (auto& item : data) {
                T* form = GetForm<T>(item.get<std::string>());
                if (form) arr.push_back(form);
                else return false;
            }
        } else if (data.is_string()) {
            T* form = GetForm<T>(data.get<std::string>());
            if (form) arr.push_back(form);
            else return false;
        }

        return true;
    }
}