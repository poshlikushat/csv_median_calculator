/**
 * \file median_calculator.cpp
 * \author github:poshlikushat
 * \brief Реализация инкрементального расчёта медианы цен из CSV-файлов
 * \date 2026-02-27
 * \version 1.0
 */

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <queue>
#include <string>

#include <spdlog/spdlog.h>

#include "median_calculator.hpp"
#include "parser_csv.hpp"

namespace app {

// ─── Расчёт медианы ─────────────────────────────────────────────────────────
//
// По требованию ТЗ используется Boost.Accumulators (tag::median).
// ВАЖНО: tag::median реализует приближённый алгоритм p² (P-squared quantile
// estimator). Он эффективен на больших потоковых выборках (>1000 элементов),
// но даёт некорректные результаты на малых наборах данных — например,
// для двух элементов [100, 102] вернёт 100 вместо точного 101.
//
// Если требуется точная медиана (например, в unit-тестах на малых данных),
// следует использовать реализацию на двух кучах (median_heap ниже),
// которая гарантирует O(log n) на вставку и O(1) на запрос медианы.
//
// Точная реализация через две кучи (оставлена для справки):
//
// class median_heap {
// public:
//     void push(double value_) noexcept {
//         if (lower_.empty() || value_ <= lower_.top()) {
//             lower_.push(value_);
//         } else {
//             upper_.push(value_);
//         }
//         balance();
//     }
//
//     [[nodiscard]] double median() const noexcept {
//         if (lower_.size() > upper_.size()) return lower_.top();
//         if (upper_.size() > lower_.size()) return upper_.top();
//         return (lower_.top() + upper_.top()) / 2.0;
//     }
//
// private:
//     void balance() noexcept {
//         while (lower_.size() > upper_.size() + 1) {
//             upper_.push(lower_.top()); lower_.pop();
//         }
//         while (upper_.size() > lower_.size() + 1) {
//             lower_.push(upper_.top()); upper_.pop();
//         }
//     }
//     std::priority_queue<double> lower_;
//     std::priority_queue<double, std::vector<double>, std::greater<double>> upper_;
// };

using accumulator_t = acc::accumulator_set<
    double,
    acc::stats<acc::tag::median>>;

// ─── Публичный интерфейс ────────────────────────────────────────────────────

median_calculator::median_calculator(
    std::vector<fs::path> input_files_,
    fs::path              output_path_) noexcept
    : _input_files{std::move(input_files_)},
      _output_path{std::move(output_path_)} {}

bool median_calculator::run() noexcept {
    spdlog::info("Загрузка данных из {} файлов", _input_files.size());

    auto records = load_records();

    if (records.empty()) {
        spdlog::warn("Нет данных для обработки — выходной файл не будет создан");
        return false;
    }

    // Сортировка по временной метке — ключевое требование ТЗ для корректной
    // инкрементальной медианы
    std::sort(records.begin(), records.end(), [](const price_record& a_, const price_record& b_) {
        return a_.receive_ts < b_.receive_ts;
    });

    spdlog::info("Загружено и отсортировано {} записей", records.size());

    return calculate_and_write(records);
}

// ─── Приватные методы ───────────────────────────────────────────────────────

std::vector<price_record> median_calculator::load_records() const noexcept {
    std::vector<price_record> all_records;
    exchange_data::csv_parser parser;

    for (const auto& path : _input_files) {
        const std::string filepath  = path.string();
        const std::string file_type = detect_file_type(path);

        if (file_type.empty()) {
            spdlog::warn("Пропущен файл с неизвестным типом: {}", filepath);
            continue;
        }

        try {
            if ("level" == file_type) {
                const auto rows = parser.parse_levels(filepath, std::string{k_delimiter});

                // Резервируем заранее, чтобы избежать реаллокаций
                all_records.reserve(all_records.size() + rows.size());

                for (const auto& row : rows) {
                    all_records.emplace_back(row.receive_ts, row.price);
                }

                spdlog::info("Файл level '{}': прочитано {} записей",
                    path.filename().string(), rows.size());

            } else {
                const auto rows = parser.parse_trades(filepath, std::string{k_delimiter});

                all_records.reserve(all_records.size() + rows.size());

                for (const auto& row : rows) {
                    all_records.emplace_back(row.receive_ts, row.price);
                }

                spdlog::info("Файл trade '{}': прочитано {} записей",
                    path.filename().string(), rows.size());
            }

        } catch (const std::exception& e) {
            // Один сломанный файл не должен останавливать всю обработку
            spdlog::error("Ошибка чтения файла '{}': {}", filepath, e.what());
        }
    }

    return all_records;
}

bool median_calculator::calculate_and_write(
    const std::vector<price_record>& records_) const noexcept {

    // Создаём выходную директорию если её нет
    std::error_code ec;
    fs::create_directories(_output_path.parent_path(), ec);
    if (ec) {
        spdlog::error("Не удалось создать директорию '{}': {}",
            _output_path.parent_path().string(), ec.message());
        return false;
    }

    std::ofstream out(_output_path);
    if (!out.is_open()) {
        spdlog::error("Не удалось открыть выходной файл: {}", _output_path.string());
        return false;
    }

    out << "receive_ts;price_median\n";

    accumulator_t accumulator;

    double   last_median   = std::numeric_limits<double>::quiet_NaN();
    uint64_t written_count = 0;

    for (const auto& record : records_) {
        accumulator(record.price);

        const double current_median = acc::median(accumulator);

        // NaN-безопасное сравнение: при первой итерации last_median == NaN,
        // поэтому условие всегда истинно — первая медиана всегда записывается
        if (current_median != last_median) {
            out << record.receive_ts << ';'
                << std::fixed << std::setprecision(k_price_precision)
                << current_median << '\n';

            last_median = current_median;
            ++written_count;
        }
    }

    spdlog::info("Записано изменений медианы: {}", written_count);
    spdlog::info("Результат сохранён: {}", _output_path.string());

    return true;
}

std::string median_calculator::detect_file_type(const fs::path& path_) noexcept {
    const std::string filename = path_.filename().string();

    if (filename.contains("level")) {
        return "level";
    }
    if (filename.contains("trade")) {
        return "trade";
    }

    return {};
}

} // namespace app