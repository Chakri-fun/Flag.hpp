#pragma once
/*
 * Copyright 2026 Chelluri Chakradhar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if __cplusplus < 202302L
    #error This Library uses C++23 features and requires at least a C++23 compliant compiler.
#endif

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <functional>
#include <stdexcept>
#include <vector>

// Min C++ 23
//
// Current support only for
// ./prog -flag 50
// ./prog --flag 50
// ./prog -flag=50
// ./prog --flag=50
// ./prog -flag  (For bool flags)
// ./prog -flag=false  (Only this no -flag false)
// ./prog -/flag 50  ] -> @Tsoding Style Comment
// ./prog --/flag 50 ]
// ./prog -flag 50,100 or ./prog -flag 50 -flag 100 (for Vector types)
// doesn't support -flag val1,val2 (for String Vector) (i.e. will return ['val1,val2',]) use -flag val1 -flag val2
// Known behaviour ./prog -flag=  or ./prog -flag="" are not supported use ./prog -flag "" instead.
namespace flag {
    template<typename T>
    concept FlagTypes = std::integral<T> || std::floating_point<T> || std::same_as<T, std::string> || std::same_as<T, bool>
        || std::same_as<T, std::vector<int>> || std::same_as<T, std::vector<float>> || std::same_as<T, std::vector<std::string>>;

    using int_ref = std::reference_wrapper<int>;
    using float_ref = std::reference_wrapper<float>;
    using string_ref = std::reference_wrapper<std::string>;
    using int_vec_ref = std::reference_wrapper<std::vector<int>>;
    using float_vec_ref = std::reference_wrapper<std::vector<float>>;
    using string_vec_ref = std::reference_wrapper<std::vector<std::string>>;
    using bool_ref = std::reference_wrapper<bool>;

    template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
    template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

    class CmdParser{
    private:
        std::vector<std::string_view> m_pos_args;
        std::unordered_map<std::string_view, std::variant<int_ref, int_vec_ref, float_ref, float_vec_ref, string_ref, string_vec_ref, bool_ref>>
            m_references;
        std::string m_help_string, m_prog_name = "default program";
        bool m_is_parse_called = false;

        template<FlagTypes T>
        constexpr std::string_view get_readable_typename() const noexcept{
            if constexpr (std::same_as<T, std::string>) return "String";
            else if constexpr(std::same_as<T, bool>) return "Boolean";
            else if constexpr(std::integral<T>) return "Integral";
            else if constexpr(std::floating_point<T>) return "Floating Point";
            else if constexpr(std::same_as<T, std::vector<int>>) return "Integral vector";
            else if constexpr(std::same_as<T, std::vector<float>>) return "Floating Point vector";
            else if constexpr(std::same_as<T, std::vector<std::string>>) return "String vector";
        }

        template<FlagTypes T>
        void print_to_help(std::string_view name, T default_value, std::string_view description) noexcept{
            m_help_string += std::format("  -{} {}\n", name, get_readable_typename<T>());
            m_help_string += std::format("\t{} (Default Value: {})\n", description, default_value);
        }

        void print_help() const noexcept{
            std::println("Usage of {}:", m_prog_name);
            std::print("{}", m_help_string);
            std::println("  -h");
            std::println("\tPrints this help message.");
        }

    public:
        CmdParser(){}

        std::vector<std::string_view> get_pos_args() const{
            if(!m_is_parse_called) throw std::runtime_error("Parse method must be called before you can access positional arguments");
            return m_pos_args;
        }

        template<FlagTypes T>
        void register_variable(T& ref, std::string_view name, T default_value, std::string_view usage){
            print_to_help(name, default_value, usage);
            if(m_references.count(name) || name == "h" || name == "help") throw std::runtime_error("Already Registered Variable");
            m_references.emplace(name, ref);
            ref = default_value;
        }

        void parse(int argc, const char* argv[]){
            m_is_parse_called = true;
            m_prog_name = argv[0];
            std::unordered_set<std::string_view> seen_flags;
            int idx;
            for(idx = 1; idx < argc; idx ++){
                std::string_view sv = argv[idx];
                size_t dash_count = sv.find_first_not_of('-');
                if (dash_count == std::string_view::npos) {
                    println("Got invalid flag: {}", sv);
                    exit(1);
                }
                if(dash_count == 0) break;
                sv.remove_prefix(std::min(dash_count, 2uz));
                bool is_commented = (sv.front() == '/'); // Tsoding's Comment style cmd line parsing
                if(is_commented) sv.remove_prefix(1);
                std::string_view sv_next;
                auto eq_pos = sv.find('=');
                if(eq_pos != std::string_view::npos){
                    sv_next = sv.substr(eq_pos + 1);
                    sv = sv.substr(0, eq_pos);
                    if(sv_next.empty()) throw std::runtime_error(std::format("Expected value after '{}='", sv));
                }
                auto dict_entry = m_references.find(sv);
                if(sv == "h" || sv == "help"){
                    print_help();
                    exit(0);
                }
                if(dict_entry == m_references.end()){
                    std::println("Got unexpected flag: {}", sv);
                    std::println("Use -h for help");
                    exit(1);
                }
                bool is_first_time = seen_flags.insert(sv).second;
                bool is_bool = holds_alternative<bool_ref>(dict_entry->second);
                if (!is_bool && sv_next.empty()) {
                    if (idx + 1 >= argc) {
                        throw std::runtime_error(std::format("No value received for flag '{}'", sv));
                    }
                    sv_next = argv[++idx];
                }
                visit( overloads{
                    [&](int_ref val){
                        int temp;
                        auto sv_next_end = sv_next.data() + sv_next.size();
                        auto [ptr, ec] = std::from_chars(sv_next.data(), sv_next_end, temp);
                        if(ptr != sv_next_end) throw std::runtime_error(std::format("Received '{}' for {} Flag '{}'", sv_next, get_readable_typename<int>(), sv));
                        if(ec == std::errc::result_out_of_range) throw std::runtime_error(std::format("Received Out of Range Value '{}' for {} Flag '{}'", sv_next, get_readable_typename<int>(), sv));
                        if(!is_commented) val.get() = temp;
                    },
                    [&](int_vec_ref val){
                        if(is_first_time && !is_commented) val.get().clear();
                        while(!sv_next.empty()){
                            int temp;
                            auto sv_next_end = sv_next.data() + sv_next.size();
                            auto [ptr, ec] = std::from_chars(sv_next.data(), sv_next_end, temp);
                            auto ptr_idx = static_cast<size_t>(ptr - sv_next.data());
                            if(ptr != sv_next_end){
                                if (sv_next[ptr_idx] == ',') sv_next.remove_prefix(ptr_idx + 1);
                                else throw std::runtime_error(std::format("Received '{}' for {} Flag '{}'", sv_next, get_readable_typename<std::vector<int>>(), sv));
                            }else sv_next = {};
                            if(ec == std::errc::result_out_of_range) throw std::runtime_error(std::format("Received Out of Range Value '{}' for {} Flag '{}'", sv_next, get_readable_typename<std::vector<int>>(), sv));
                            if(!is_commented) val.get().push_back(temp);
                        }
                    },
                    [&](float_ref val){
                        float temp;
                        auto sv_next_end = sv_next.data() + sv_next.size();
                        auto [ptr, ec] = std::from_chars(sv_next.data(), sv_next_end, temp);
                        if(ptr != sv_next_end) throw std::runtime_error(std::format("Received '{}' for {} Flag '{}'", sv_next, get_readable_typename<float>(), sv));
                        if(ec == std::errc::result_out_of_range) throw std::runtime_error(std::format("Received Out of Range Value '{}' for {} Flag '{}'", sv_next, get_readable_typename<float>(), sv));
                        if(!is_commented) val.get() = temp;
                    },
                    [&](float_vec_ref val){
                        if(is_first_time && !is_commented) val.get().clear();
                        while(!sv_next.empty()){
                            float temp;
                            auto sv_next_end = sv_next.data() + sv_next.size();
                            auto [ptr, ec] = std::from_chars(sv_next.data(), sv_next_end, temp);
                            auto ptr_idx = static_cast<size_t>(ptr - sv_next.data());
                            if(ptr != sv_next_end){
                                if(sv_next[ptr_idx] == ',') sv_next.remove_prefix(ptr_idx + 1);
                                else throw std::runtime_error(std::format("Received '{}' for {} Flag '{}'", sv_next, get_readable_typename<std::vector<float>>(), sv));
                            }else sv_next = {};
                            if(ec == std::errc::result_out_of_range) throw std::runtime_error(std::format("Received Out of Range Value '{}' for {} Flag '{}'", sv_next, get_readable_typename<std::vector<float>>(), sv));
                            if(!is_commented) val.get().push_back(temp);
                        }
                    },
                    [&](string_ref val){
                        if(!is_commented) val.get() = std::string(sv_next.begin(), sv_next.end());
                    },
                    [&](string_vec_ref val){
                        if(is_first_time && !is_commented) val.get().clear();
                        if(!is_commented) val.get().push_back(std::string(sv_next.begin(), sv_next.end()));
                    },
                    [&](bool_ref val){
                        if(!sv_next.empty()){
                            if(sv_next != "false" && sv_next != "true" && sv_next != "1" && sv_next != "0")
                                throw std::runtime_error(std::format("Invalid boolean value '{}' for flag '{}'", sv_next, sv));
                            if(!is_commented){
                                if (sv_next == "false" || sv_next == "0") val.get() = false;
                                else if (sv_next == "true" || sv_next == "1") val.get() = true;
                            }
                        }else{
                            if(!is_commented) val.get() = true;
                        }
                    }
                }, dict_entry->second);
            }
            while (idx < argc) {
                m_pos_args.emplace_back(argv[idx]);
                idx ++;
            }
        }
    };
}
