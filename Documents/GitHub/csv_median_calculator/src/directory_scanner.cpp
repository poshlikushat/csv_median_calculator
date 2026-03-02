/**
 * \file directory_scanner.cpp
 * \author github:poshlikushat
 * \brief Реализация модуля для сканирования файловой системы и поиска CSV-файлов
 * \date 2026-02-27
 * \version 1.1
 */

#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <spdlog/spdlog.h>

#include "directory_scanner.hpp"

namespace app::io {

directory_scanner::directory_scanner(
    fs::path                  input_dir_,
    const std::vector<std::string>& masks_) noexcept
    : _input_dir{std::move(input_dir_)},
      _masks{masks_} {}

std::tuple<std::vector<fs::path>, std::error_code> directory_scanner::scan() const noexcept {
    std::error_code ec;

    if (!fs::exists(_input_dir, ec) || !fs::is_directory(_input_dir, ec)) {
        return {{}, std::make_error_code(std::errc::no_such_file_or_directory)};
    }

    // std::set гарантирует уникальность: файл, совпадающий с несколькими масками,
    // не попадёт в результат дважды
    std::set<fs::path> found_set;

    for (const auto& entry : fs::directory_iterator(_input_dir, ec)) {
        if (ec) [[unlikely]] {
            spdlog::warn("Ошибка при доступе к элементу директории: {}", ec.message());
            ec.clear(); // Один сломанный элемент не должен прерывать весь обход
            continue;
        }

        if (!entry.is_regular_file()) {
            continue;
        }

        const auto& path = entry.path();

        if (path.extension() != ".csv") {
            continue;
        }

        const std::string filename = path.filename().string();

        // Пустой список масок = принять все CSV
        if (_masks.empty()) {
            found_set.emplace(path);
            continue;
        }

        for (const auto& mask : _masks) {
            if (filename.contains(mask)) {
                found_set.emplace(path);
                break; // Достаточно одного совпадения
            }
        }
    }

    return {std::vector<fs::path>(found_set.begin(), found_set.end()), {}};
}

} // namespace app::io