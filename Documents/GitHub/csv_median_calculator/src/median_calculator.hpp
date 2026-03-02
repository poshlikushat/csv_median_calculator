/**
 * \file median_calculator.hpp
 * \author github:poshlikushat
 * \brief Инкрементальный расчёт медианы цен из CSV-файлов биржевых торгов
 * \date 2026-02-27
 * \version 1.1
 */

#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/stats.hpp>

#include "parser_csv.hpp"

namespace app {

namespace fs  = std::filesystem;
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
 * Файлы загружаются параллельно через std::jthread — каждый файл в отдельном
 * потоке. После завершения всех потоков записи объединяются, сортируются по
 * receive_ts и последовательно подаются в аккумулятор Boost. При каждом
 * изменении медианы строка фиксируется в выходном CSV.
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
     * \brief Запускает полный цикл: параллельное чтение → сортировка → расчёт → запись
     * \return true если обработка прошла без критических ошибок
     */
    bool run() const noexcept;

private:
    /**
     * \brief Параллельно читает все входные файлы через std::jthread
     * \return Объединённый вектор price_record из всех файлов
     *
     * Каждый файл обрабатывается в отдельном jthread. Результаты защищены
     * мьютексом при записи в общий вектор. Некорректные файлы пропускаются
     * с предупреждением.
     */
    [[nodiscard]] std::vector<price_record> load_records_parallel() const noexcept;

    /**
		 * \brief Читает один файл и добавляет его записи в общий вектор
		 * \param path_        Путь к файлу
		 * \param out_mutex_   Мьютекс для защиты out_records_
		 * \param out_records_ Общий выходной вектор
		 */
		void load_one_file(
        const fs::path&            path_,
        std::mutex&                out_mutex_,
        std::vector<price_record>& out_records_) const noexcept;

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

    /// Разделитель CSV
    static constexpr std::string_view k_delimiter{";"};

    /// Количество знаков после запятой в выходном файле
    static constexpr int k_price_precision{8};
};

} // namespace app