/**
 * \file median_calculator.hpp
 * \author github:poshlikushat
 * \brief Инкрементальный расчёт медианы цен из CSV-файлов биржевых торгов
 * \date 2026-02-27
 * \version 1.0
 */
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/stats.hpp>

#include "parser_csv.hpp"

namespace app {

namespace fs = std::filesystem;

namespace acc = boost::accumulators;

/// Единая запись с временной меткой и ценой — общий знаменатель для level и trade
struct price_record {
    std::int64_t receive_ts;
    double       price;
};

/**
 * \brief Вычисляет инкрементальную медиану цен из CSV-файлов и записывает
 *        результат в выходной файл.
 *
 * Класс объединяет данные из level- и trade-файлов, сортирует их по receive_ts
 * и последовательно добавляет цены в аккумулятор Boost. При каждом изменении
 * медианы строка фиксируется в выходном CSV.
 */
class median_calculator {
public:
    /**
     * \brief Конструктор
     * \param input_files_  Список путей к входным CSV-файлам
     * \param output_path_  Путь к выходному CSV-файлу
     */
    median_calculator(
        std::vector<fs::path> input_files_,
        fs::path              output_path_) noexcept;

    /**
     * \brief Запускает полный цикл: чтение → сортировка → расчёт → запись
     * \return true если обработка прошла без критических ошибок
     */
    bool run() noexcept;

private:
    /**
     * \brief Читает все входные файлы и объединяет записи в единый вектор
     * \return Вектор price_record из всех файлов
     *
     * Определяет тип файла (level/trade) по имени, вызывает соответствующий
     * метод парсера. Некорректные файлы пропускаются с предупреждением.
     */
    [[nodiscard]] std::vector<price_record> load_records() const noexcept;

    /**
     * \brief Вычисляет медиану инкрементально и пишет изменения в файл
     * \param records_ Отсортированный по receive_ts вектор записей
     * \return true если запись в файл прошла успешно
     */
    [[nodiscard]] bool calculate_and_write(
        const std::vector<price_record>& records_) const noexcept;

    /**
     * \brief Определяет тип файла по имени
     * \param path_ Путь к файлу
     * \return "level", "trade" или "" если тип не определён
     */
    [[nodiscard]] static std::string detect_file_type(
        const fs::path& path_) noexcept;

    std::vector<fs::path> _input_files;
    fs::path              _output_path;

    /// Разделитель CSV согласно ТЗ
    static constexpr std::string_view k_delimiter{";"};

    /// Формат вещественного числа в выходном файле (8 знаков после запятой)
    static constexpr int k_price_precision{8};
};

} // namespace app