#include <iostream>
#include <spdlog/spdlog.h>
#include <toml++/toml.h>
#include <boost/program_options.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>

namespace po = boost::program_options;
namespace acc = boost::accumulators;

/**
 * \brief Главная функция программы
 * \param argc_ Количество аргументов командной строки
 * \param argv_ Массив аргументов командной строки
 * \return Код завершения программы
 */
int main(int argc_, char* argv_[]) {
	try {
		// Настраиваем базовое логирование
		spdlog::set_level(spdlog::level::debug);
		spdlog::info("Приложение для расчета медианы запущено");

		// Тестируем Boost.Program_options
		po::options_description desc("Разрешенные опции");
		desc.add_options()
				("help,h", "Вывести справочное сообщение")
				("config,cfg", po::value<std::string>()->default_value("config.toml"), "Путь к конфигурационному файлу");

		po::variables_map vm;
		po::store(po::parse_command_line(argc_, argv_, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 0;
		}

		std::string config_path = vm["config"].as<std::string>();
		spdlog::info("Используется конфигурационный файл: {}", config_path);

		// Тестируем Boost.Accumulators (инкрементальная медиана)
		acc::accumulator_set<double, acc::stats<acc::tag::median>> median_acc;

		median_acc(100.0);
		median_acc(102.0);
		median_acc(99.0);

		spdlog::info("Текущая медиана (ожидается 100): {}", acc::median(median_acc));

	} catch (const std::exception& e) {
		spdlog::error("Критическая ошибка: {}", e.what());
		return 1;
	}

	return 0;
}