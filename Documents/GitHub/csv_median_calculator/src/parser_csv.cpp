/**
 * \file parser_csv.cpp
 * \author github:poshlikushat
 * \brief Реализация методов парсера CSV файлов
 * \date 2026-02-27
 * \version 1.1
 */

#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <spdlog/spdlog.h>

#include "parser_csv.hpp"

namespace exchange_data {

std::vector<level_data> csv_parser::parse_levels(
    const std::string& filepath_,
    const std::string& delimiter_) noexcept(false) {

    std::ifstream file(filepath_);

    if (!file.is_open()) {
        throw std::runtime_error(
            std::format("Не удалось открыть файл: {}", filepath_));
    }

    std::vector<level_data> data;
    std::string line;
    std::getline(file, line); // Пропускаем заголовок

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        // Тримим \r для совместимости с CRLF (Windows-файлы)
        if ('\r' == line.back()) {
            line.pop_back();
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(delimiter_));

        if (6 != tokens.size()) {
            spdlog::warn("Пропущена строка с неверным числом колонок: {}", line);
            continue;
        }

        try {
            level_data row;
            row.receive_ts  = std::stoll(tokens[0]);
            row.exchange_ts = std::stoll(tokens[1]);
            row.price       = std::stod(tokens[2]);
            row.quantity    = std::stod(tokens[3]);
            row.side        = std::move(tokens[4]);
            row.rebuild     = std::stoi(tokens[5]);

            data.push_back(std::move(row));
        } catch (const std::exception& e) {
            spdlog::warn("Ошибка конвертации строки '{}': {}", line, e.what());
        }
    }

    return data;
}

std::vector<trade_data> csv_parser::parse_trades(
    const std::string& filepath_,
    const std::string& delimiter_) noexcept(false) {

    std::ifstream file(filepath_);

    if (!file.is_open()) {
        throw std::runtime_error(
            std::format("Не удалось открыть файл: {}", filepath_));
    }

    std::vector<trade_data> data;
    std::string line;
    std::getline(file, line); // Пропускаем заголовок

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        if ('\r' == line.back()) {
            line.pop_back();
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(delimiter_));

        if (5 != tokens.size()) {
            spdlog::warn("Пропущена строка с неверным числом колонок: {}", line);
            continue;
        }

        try {
            trade_data row;
            row.receive_ts  = std::stoll(tokens[0]);
            row.exchange_ts = std::stoll(tokens[1]);
            row.price       = std::stod(tokens[2]);
            row.quantity    = std::stod(tokens[3]);
            row.side        = std::move(tokens[4]);

            data.push_back(std::move(row));
        } catch (const std::exception& e) {
            spdlog::warn("Ошибка конвертации строки '{}': {}", line, e.what());
        }
    }

    return data;
}

} // namespace exchange_data