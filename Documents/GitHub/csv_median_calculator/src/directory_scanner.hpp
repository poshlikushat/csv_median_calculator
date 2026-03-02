/**
* \file directory_scanner.hpp
 * \author github:poshlikushat
 * \brief Модуль для сканирования файловой системы и поиска CSV-файлов
 * \date 2026-02-27
 * \version 1.1
 */

#pragma once

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace app::io {

	namespace fs = std::filesystem;

	/**
	 * \brief Класс для поиска CSV-файлов в директории по маскам имён
	 *
	 * Принимает путь к директории и список масок (подстрок имени файла).
	 * Пустой список масок означает "принять все CSV-файлы".
	 */
	class directory_scanner {
	public:
		/**
		 * \brief Конструктор сканера
		 * \param input_dir_  Путь к директории для сканирования (по значению — move)
		 * \param masks_      Список масок для фильтрации имён файлов
		 */
		directory_scanner(
		    fs::path                  input_dir_,
		    const std::vector<std::string>& masks_) noexcept;

		/**
		 * \brief Выполняет поиск CSV-файлов, соответствующих маскам
		 * \return Кортеж {список найденных путей, код ошибки}
		 *
		 * Файл включается в результат если его имя содержит хотя бы одну из масок.
		 * При нескольких совпадающих масках дублирования не происходит.
		 */
		[[nodiscard]] std::tuple<std::vector<fs::path>, std::error_code> scan() const noexcept;

	private:
		fs::path               _input_dir;
		std::vector<std::string> _masks;
	};

} // namespace app::io