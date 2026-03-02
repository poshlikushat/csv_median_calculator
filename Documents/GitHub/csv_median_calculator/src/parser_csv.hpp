/**
* \file parser_csv.hpp
 * \author github:poshlikushat
 * \brief Парсер CSV файлов для обработки биржевых данных
 * \date 2026-02-27
 * \version 1.1
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace exchange_data {

	/**
	 * \brief Данные одного уровня стакана заявок (level.csv)
	 */
	struct level_data {
		std::int64_t receive_ts;
		std::int64_t exchange_ts;
		double       price;
		double       quantity;
		std::string  side;
		std::int32_t rebuild;
	};

	/**
	 * \brief Данные одной совершённой сделки (trade.csv)
	 */
	struct trade_data {
		std::int64_t receive_ts;
		std::int64_t exchange_ts;
		double       price;
		double       quantity;
		std::string  side;
	};

	/**
	 * \brief Класс для чтения и парсинга CSV файлов биржевых торгов
	 */
	class csv_parser {
	public:
		/**
		 * \brief Читает файл стакана заявок и возвращает вектор структур
		 * \param filepath_   Путь к CSV файлу
		 * \param delimiter_  Символ-разделитель (например, ";")
		 * \return Список записей стакана
		 * \throws std::runtime_error если файл не удалось открыть
		 */
		[[nodiscard]] static std::vector<level_data> parse_levels(
		    const std::string& filepath_,
		    const std::string& delimiter_) noexcept(false);

		/**
		 * \brief Читает файл совершённых сделок и возвращает вектор структур
		 * \param filepath_   Путь к CSV файлу
		 * \param delimiter_  Символ-разделитель
		 * \return Список сделок
		 * \throws std::runtime_error если файл не удалось открыть
		 */
		[[nodiscard]] static std::vector<trade_data> parse_trades(
		    const std::string& filepath_,
		    const std::string& delimiter_) noexcept(false);
	};

} // namespace exchange_data