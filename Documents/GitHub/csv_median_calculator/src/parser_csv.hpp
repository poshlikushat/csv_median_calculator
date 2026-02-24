/**
* \file parser_csv.hpp
 * \author github:poshlikushat
 * \brief Парсер CSV файлов для обработки биржевых данных
 * \date 2026-02-24
 * \version 1.0
 */

#pragma once

#include <string>
#include <vector>

namespace exchange_data {

	/**
	 * \brief Структура для хранения данных стакана (level.csv)
	 */
	struct level_data {
		long long receive_ts;
		long long exchange_ts;
		double price;
		double quantity;
		std::string side;
		int rebuild;
	};

	/**
	 * \brief Структура для хранения данных сделок (trade.csv)
	 */
	struct trade_data {
		long long receive_ts;
		long long exchange_ts;
		double price;
		double quantity;
		std::string side;
	};

	/**
	 * \brief Класс для чтения и парсинга CSV файлов
	 */
	class csv_parser {
	public:
		/**
		 * \brief Читает файл стакана заявок и возвращает вектор структур
		 * \param filepath_ Путь к CSV файлу
		 * \param delimiter_ Символ-разделитель (например, ";")
		 * \return std::vector<level_data> Список записей стакана
		 */
		static std::vector<level_data> parse_levels(
				const std::string& filepath_,
				const std::string& delimiter_ ) noexcept(false);

		/**
		 * \brief Читает файл совершенных сделок и возвращает вектор структур
		 * \param filepath_ Путь к CSV файлу
		 * \param delimiter_ Символ-разделитель
		 * \return std::vector<trade_data> Список сделок
		 */
		static std::vector<trade_data> parse_trades(
				const std::string& filepath_,
				const std::string& delimiter_ ) noexcept(false);
	};

} // namespace exchange_data