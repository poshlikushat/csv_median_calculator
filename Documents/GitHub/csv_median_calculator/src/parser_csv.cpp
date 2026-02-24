/**
 * \file parser_csv.cpp
 * \author github:poshlikushat
 * \brief Реализация методов парсера CSV файлов
 * \date 2026-02-24
 * \version 1.0
 */
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "parser_csv.hpp"

namespace exchange_data {

std::vector<level_data> csv_parser::parse_levels(
    const std::string& filepath_,
    const std::string& delimiter_) noexcept(false) {

    std::vector<level_data> data;
    std::ifstream file(filepath_);

    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filepath_);
    }

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(delimiter_));

        if (6 != tokens.size()) {
            std::cerr << "Пропущена некорректная строка: " << line << "\n";
            continue;
        }

        try {
            level_data row;
            row.receive_ts  = std::stoll(tokens[0]);
            row.exchange_ts = std::stoll(tokens[1]);
            row.price       = std::stod(tokens[2]);
            row.quantity    = std::stod(tokens[3]);
            row.side        = tokens[4];
            row.rebuild     = std::stoi(tokens[5]);

            data.push_back(row);
        } catch (const std::exception& e) {
            std::cerr << "Ошибка конвертации: " << line << "\n";
        }
    }

    return data;
}

std::vector<trade_data> csv_parser::parse_trades(
    const std::string& filepath_,
    const std::string& delimiter_) noexcept(false) {

    std::vector<trade_data> data;
    std::ifstream file(filepath_);

    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filepath_);
    }

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(delimiter_));

        if (5 != tokens.size()) {
            std::cerr << "Пропущена некорректная строка: " << line << "\n";
            continue;
        }

        try {
            trade_data row;
            row.receive_ts  = std::stoll(tokens[0]);
            row.exchange_ts = std::stoll(tokens[1]);
            row.price       = std::stod(tokens[2]);
            row.quantity    = std::stod(tokens[3]);
            row.side        = tokens[4];

            data.push_back(row);
        } catch ( const std::exception& e ) {
            std::cerr << e.what() << "\n";
        }
    }

    return data;
}

} // namespace exchange_data