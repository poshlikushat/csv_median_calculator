/**
 * \file median_calculator.cpp
 * \author github:poshlikushat
 * \brief Реализация инкрементального расчёта медианы цен из CSV-файлов
 * \date 2026-02-27
 * \version 1.1
 */

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include "median_calculator.hpp"
#include "parser_csv.hpp"

namespace app {

// Расчёт медианы
//
// Используется Boost.Accumulators (tag::median) — приближённый алгоритм p²
// (P-squared quantile estimator). Эффективен на больших потоковых выборках
// (>1000 элементов), но даёт погрешность на малых наборах данных.
//
// Точная альтернатива через две кучи (min-heap + max-heap) оставлена
// закомментированной — даёт O(log n) на вставку и O(1) на запрос:
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
//     [[nodiscard]] double median() const noexcept {
//         if (lower_.size() > upper_.size()) return lower_.top();
//         if (upper_.size() > lower_.size()) return upper_.top();
//         return (lower_.top() + upper_.top()) / 2.0;
//     }
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

// Публичный интерфейс

median_calculator::median_calculator(
    std::vector<fs::path> input_files_,
    fs::path              output_path_) noexcept
    : _input_files{std::move(input_files_)},
      _output_path{std::move(output_path_)} {}

bool median_calculator::run() const noexcept {
    spdlog::info("Параллельная загрузка данных из {} файлов", _input_files.size());

    auto records = load_records_parallel();

    if (records.empty()) {
        spdlog::warn("Нет данных для обработки — выходной файл не будет создан");
        return false;
    }

    // Каждая новая цена должна учитывать все предыдущие
    // в хронологическом порядке
    std::ranges::sort(records, [](const price_record& a_, const price_record& b_) {
        return a_.receive_ts < b_.receive_ts;
    });

    spdlog::info("Загружено и отсортировано {} записей", records.size());

    return calculate_and_write(records);
}

// Приватные методы

std::vector<price_record> median_calculator::load_records_parallel() const noexcept {
    std::vector<price_record> all_records;
    std::mutex                records_mutex;

    // Запускаем по одному jthread на каждый файл.
    std::vector<std::jthread> threads;
    threads.reserve(_input_files.size());

    for (const auto& path : _input_files) {
        threads.emplace_back([this, &path, &records_mutex, &all_records] {
            load_one_file(path, records_mutex, all_records);
        });
    }

    return all_records;
}

void median_calculator::load_one_file(
    const fs::path&            path_,
    std::mutex&                out_mutex_,
    std::vector<price_record>& out_records_) const noexcept {

    const std::string filepath  = path_.string();
    const std::string file_type = detect_file_type(path_);

    if (file_type.empty()) {
        spdlog::warn("Пропущен файл с неизвестным типом: {}", filepath);
        return;
    }

    try {
        exchange_data::csv_parser parser;
        std::vector<price_record> local_records;

        if ("level" == file_type) {
            const auto rows = parser.parse_levels(filepath, std::string{k_delimiter});
            local_records.reserve(rows.size());

            for (const auto& row : rows) {
                local_records.emplace_back(row.receive_ts, row.price);
            }

            spdlog::info(std::format("Файл level '{}': прочитано {} записей",
                path_.filename().string(), rows.size()));

        } else {
            const auto rows = parser.parse_trades(filepath, std::string{k_delimiter});
            local_records.reserve(rows.size());

            for (const auto& row : rows) {
                local_records.emplace_back(row.receive_ts, row.price);
            }

            spdlog::info(std::format("Файл trade '{}': прочитано {} записей",
                path_.filename().string(), rows.size()));
        }

        // Объединяем локальный результат в общий вектор
        const std::lock_guard<std::mutex> lock{out_mutex_};
        out_records_.reserve(out_records_.size() + local_records.size());
        out_records_.insert(
            out_records_.end(),
            std::make_move_iterator(local_records.begin()),
            std::make_move_iterator(local_records.end()));

    } catch (const std::exception& e) {
        // Один сломанный файл не должен останавливать всю обработку
        spdlog::error("Ошибка чтения файла '{}': {}", filepath, e.what());
    }
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

    // Буферизуем запись: для >100MB файлов сброс на каждой строке катастрофически
    // медленный — используем буфер 256KB для пакетной записи
    constexpr std::size_t k_buf_size = 256 * 1024;
    std::vector<char>     write_buf(k_buf_size);
    out.rdbuf()->pubsetbuf(write_buf.data(), static_cast<std::streamsize>(k_buf_size));

    out << "receive_ts;price_median\n";

    accumulator_t accumulator;

    double       last_median   = std::numeric_limits<double>::quiet_NaN();
    std::uint64_t written_count = 0;

    for (const auto &[receive_ts, price] : records_) {
        accumulator(price);

			// NaN-безопасное сравнение: при первой итерации last_median == NaN,
        // поэтому условие всегда истинно — первая медиана всегда записывается
        if (const double current_median = acc::median(accumulator); current_median != last_median) {
            out << receive_ts << ';'
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