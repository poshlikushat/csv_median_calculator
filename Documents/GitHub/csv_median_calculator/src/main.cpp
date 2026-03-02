/**
 * \file main.cpp
 * \author github:poshlikushat
 * \brief Точка входа приложения csv_median_calculator
 * \date 2026-02-27
 * \version 1.1
 */

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

#include "directory_scanner.hpp"
#include "median_calculator.hpp"

namespace fs = std::filesystem;
namespace po = boost::program_options;

// Константы

static constexpr std::string_view k_app_version{"1.1.0"};
static constexpr std::string_view k_default_config{"config.toml"};
static constexpr std::string_view k_default_output_dir{"output"};
static constexpr std::string_view k_output_filename{"median_result.csv"};

// Вспомогательные структуры и функции

struct app_config {
    fs::path                 input_dir;
    fs::path                 output_dir;
    std::vector<std::string> masks;
};

/**
 * \brief Парсит аргументы командной строки и возвращает путь к конфигу
 * \param argc_ Количество аргументов
 * \param argv_ Массив аргументов
 * \return Путь к файлу конфигурации
 * \throws std::exception при ошибке разбора аргументов
 */
[[nodiscard]] std::string parse_args(int argc_, char* argv_[]) noexcept(false) {
    po::options_description desc("Параметры");
    desc.add_options()
        ("help,h",  "Показать справку")
        ("config",  po::value<std::string>(), "Путь к конфигурационному файлу")
        ("cfg",     po::value<std::string>(), "Путь к конфигурационному файлу (псевдоним)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc_, argv_, desc), vm);
    po::notify(vm);

    if (vm.contains("help")) {
        std::cout << desc << '\n';
        std::exit(0);
    }

    if (vm.contains("config")) {
        return vm["config"].as<std::string>();
    }

    if (vm.contains("cfg")) {
        return vm["cfg"].as<std::string>();
    }

    // Конфиг не указан — ищем рядом с исполняемым файлом
    return std::string{k_default_config};
}

/**
 * \brief Читает и валидирует конфигурационный TOML-файл
 * \param config_path_ Путь к файлу конфигурации
 * \return Заполненная структура app_config
 * \throws std::runtime_error если обязательные поля отсутствуют или путь не найден
 */
[[nodiscard]] app_config load_config(const fs::path& config_path_) noexcept(false) {
    if (!fs::exists(config_path_)) {
        throw std::runtime_error(
            std::format("Конфигурационный файл не найден: {}", config_path_.string()));
    }

    const auto toml = toml::parse_file(config_path_.string());
    app_config cfg;

    const auto input = toml["main"]["input"].value<std::string>();
    if (!input) {
        throw std::runtime_error("Отсутствует обязательный параметр: main.input");
    }
    cfg.input_dir = *input;

    // output опционален — по умолчанию ./output
    const auto output = toml["main"]["output"].value<std::string>();
    cfg.output_dir = output ? fs::path{*output} : fs::path{k_default_output_dir};

    // masks опциональны — пустой список означает "все CSV"
    if (const auto* masks_arr = toml["main"]["filename_mask"].as_array()) {
        for (const auto& mask : *masks_arr) {
            if (const auto val = mask.value<std::string>()) {
                cfg.masks.push_back(*val);
            }
        }
    }

    return cfg;
}

int main(int argc, char* argv[]) {
    spdlog::info("Запуск приложения csv_median_calculator v{}", k_app_version);

    // 1. Парсим аргументы командной строки
    std::string config_path;
    try {
        config_path = parse_args(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error("Ошибка разбора аргументов: {}", e.what());
        return 1;
    }

    // 2. Читаем конфигурацию
    app_config cfg;
    try {
        cfg = load_config(config_path);
    } catch (const std::exception& e) {
        spdlog::error("Ошибка конфигурации: {}", e.what());
        return 1;
    }

    spdlog::info(std::format("Конфигурация: input='{}', output='{}'",
        cfg.input_dir.string(), cfg.output_dir.string()));

    // 3. Сканируем директорию
    app::io::directory_scanner scanner(cfg.input_dir, cfg.masks);
    auto [files, ec] = scanner.scan();

    if (ec) {
        spdlog::error("Ошибка сканирования директории: {}", ec.message());
        return 1;
    }

    if (files.empty()) {
        spdlog::warn("Не найдено подходящих CSV-файлов в: {}", cfg.input_dir.string());
        return 0;
    }

    spdlog::info("Найдено файлов: {}", files.size());
    for (const auto& f : files) {
        spdlog::info("  - {}", f.filename().string());
    }

    // 4. Запускаем расчёт медианы (параллельное чтение внутри)
    const fs::path output_path = cfg.output_dir / k_output_filename;
    app::median_calculator calculator(std::move(files), output_path);

    if (!calculator.run()) {
        spdlog::error("Обработка завершилась с ошибкой");
        return 1;
    }

    spdlog::info("Завершение работы");
    return 0;
}