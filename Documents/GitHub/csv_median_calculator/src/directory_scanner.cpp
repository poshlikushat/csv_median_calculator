/**
* \file parser_csv.cpp
 * \author github:poshlikushat
 * \brief Реализация модуля для сканирования файловой системы и поиска CSV-файлов
 * \date 2026-02-25
 * \version 1.0
 */

#include <filesystem>
#include <string>
#include <vector>
#include <system_error>
#include <tuple>
#include <spdlog/spdlog.h>

#include "directory_scanner.hpp"

namespace app::io {
 directory_scanner::directory_scanner(fs::path &input_dir_, const std::vector<std::string> &masks_)
	: _input_dir{std::move(input_dir_)},
   _masks{masks_} {}

	std::tuple<std::vector<fs::path>, std::error_code> directory_scanner::scan() const {
 		std::vector<fs::path> found_files;
 		std::error_code ec;
		if (!fs::exists(_input_dir, ec) || !fs::is_directory(_input_dir, ec)) {
			return {found_files, std::make_error_code(std::errc::no_such_file_or_directory)};
		}

 		for (const auto &file : fs::directory_iterator(_input_dir, ec)) {
 			if (ec) [[unlikely]] {
 				spdlog::warn("Ошибка при доступе к элементу директории: {}", ec.message());
 				continue;
 			}

 			if (!file.is_regular_file()) {
 				continue;
 			}
 			const auto& path = file.path();
 			if (path.extension() != ".csv") {
 				continue;
 			}

 			std::string filename = path.filename().string();
 			bool is_matched = false;

 			for (const auto &mask : _masks) {
 				if (filename.contains(mask)) {
 					is_matched = true;
 					break;
 				}
 			}

 			if (is_matched) {
 				found_files.emplace_back(path);
 			}
 		}
 	return {found_files, ec};
 }
}
