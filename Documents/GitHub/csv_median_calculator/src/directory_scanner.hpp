/**
 * \file directory_scanner.hpp
 * \brief Модуль для сканирования файловой системы и поиска CSV-файлов
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <tuple>

namespace app::io {

namespace fs = std::filesystem;

/**
 * \brief Класс для поиска CSV-файлов в директории
 */
class directory_scanner {
		fs::path _input_dir;
		std::vector<std::string> _masks;
public:
    /**
     * \brief Конструктор сканера
     * \param input_dir_ Путь к директории для сканирования
     * \param masks_ Список масок для фильтрации имен файлов
     */
    directory_scanner(fs::path& input_dir_, const std::vector<std::string>& masks_);

    /**
     * \brief Выполняет поиск файлов
     * \return Кортеж из списка найденных путей и кода ошибки (если есть)
     */
    std::tuple<std::vector<fs::path>, std::error_code> scan() const;
};

} // namespace app::io
