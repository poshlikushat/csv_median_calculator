/**
 * \file test_parser_csv.cpp
 * \author github:poshlikushat
 * \brief Unit-тесты для парсера CSV с использованием Google Test
 * \date 2026-02-24
 * \version 1.0
 */

#include <cstdio>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/parser_csv.hpp"

namespace exchange_data::tests {

/**
 * \brief Вспомогательная функция для создания временного тестового файла
 * \param filepath_ Путь к создаваемому файлу
 * \param content_ Содержимое файла
 */
void create_temp_file(
    const std::string& filepath_,
    const std::string& content_) noexcept(false) {

    std::ofstream file(filepath_);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось создать тестовый файл");
    }
    file << content_;
}

// Тест 1: Проверка успешного парсинга стакана (level)
TEST(csv_parser_tests, parse_level_success) {
    const std::string filepath = "test_level.csv";
    const std::string content =
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.00000000;10.10900000;bid;1\n"
        "1716810808593628;1716810808574000;68479.90000000;0.00400000;bid;0\n";

    create_temp_file(filepath, content);

    const auto data = csv_parser::parse_levels(filepath, ";");

    // Константы слева согласно стилю (Yoda conditions)
    ASSERT_EQ(2, data.size());
    EXPECT_EQ(1716810808593627, data[0].receive_ts);
    EXPECT_DOUBLE_EQ(68480.0, data[0].price);
    EXPECT_EQ("bid", data[0].side);
    EXPECT_EQ(1, data[0].rebuild);

    // Убираем за собой
    std::remove(filepath.c_str());
}

// Тест 2: Проверка обработки битых строк (недостаточно колонок)
TEST(csv_parser_tests, skips_invalid_rows) {
    const std::string filepath = "test_invalid.csv";
    // Вторая строка сломана (нет разделителей)
    const std::string content =
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.0;10.1;bid;1\n"
        "BROKEN_ROW_WITHOUT_SEMICOLONS\n";

    create_temp_file(filepath, content);

    const auto data = csv_parser::parse_levels(filepath, ";");

    // Должна распарситься только одна валидная строка
    ASSERT_EQ(1, data.size());

    std::remove(filepath.c_str());
}

// Тест 3: Проверка выброса исключения при отсутствии файла
TEST(csv_parser_tests, throws_on_missing_file) {
    EXPECT_THROW(
        { auto data = csv_parser::parse_levels("non_existent_file_123.csv", ";"); },
        std::runtime_error
    );
}

    // Тест 4: Пустой файл (только заголовок)
TEST(csv_parser_tests, empty_file_returns_empty_vector) {
    const std::string filepath = "test_empty.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    EXPECT_EQ(0, data.size());
    std::remove(filepath.c_str());
}

// Тест 5: Только заголовок без переноса строки
TEST(csv_parser_tests, header_only_no_newline) {
    const std::string filepath = "test_header_only.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild");

    const auto data = csv_parser::parse_levels(filepath, ";");

    EXPECT_EQ(0, data.size());
    std::remove(filepath.c_str());
}

// Тест 6: Корректный парсинг стороны "ask"
TEST(csv_parser_tests, parse_ask_side) {
    const std::string filepath = "test_ask.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68500.00000000;5.00000000;ask;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(1, data.size());
    EXPECT_EQ("ask", data[0].side);
    EXPECT_EQ(0, data[0].rebuild);
    std::remove(filepath.c_str());
}

// Тест 7: Rebuild = 1 и rebuild = 0 парсятся корректно
TEST(csv_parser_tests, rebuild_flag_values) {
    const std::string filepath = "test_rebuild.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.0;10.1;bid;1\n"
        "1716810808593628;1716810808574000;68479.0;5.5;ask;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(2, data.size());
    EXPECT_EQ(1, data[0].rebuild);
    EXPECT_EQ(0, data[1].rebuild);
    std::remove(filepath.c_str());
}

// Тест 8: Цена с большим количеством знаков после запятой
TEST(csv_parser_tests, high_precision_price) {
    const std::string filepath = "test_precision.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.12345678;0.00000001;bid;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(1, data.size());
    EXPECT_DOUBLE_EQ(68480.12345678, data[0].price);
    EXPECT_DOUBLE_EQ(0.00000001, data[0].quantity);
    std::remove(filepath.c_str());
}

// Тест 9: Строка с лишними колонками пропускается
TEST(csv_parser_tests, skips_row_with_extra_columns) {
    const std::string filepath = "test_extra_cols.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.0;10.1;bid;1;EXTRA_COLUMN\n"
        "1716810808593628;1716810808574000;68479.0;5.5;ask;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(1, data.size());
    EXPECT_EQ(1716810808593628, data[0].receive_ts);
    std::remove(filepath.c_str());
}

// Тест 10: Строка с нечисловым значением в цене пропускается
TEST(csv_parser_tests, skips_row_with_invalid_price) {
    const std::string filepath = "test_invalid_price.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;NOT_A_NUMBER;10.1;bid;1\n"
        "1716810808593628;1716810808574000;68479.0;5.5;ask;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(1, data.size());
    EXPECT_EQ(1716810808593628, data[0].receive_ts);
    std::remove(filepath.c_str());
}

// Тест 11: Строка с нечисловым timestamp пропускается
TEST(csv_parser_tests, skips_row_with_invalid_timestamp) {
    const std::string filepath = "test_invalid_ts.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "INVALID_TS;1716810808574000;68480.0;10.1;bid;1\n"
        "1716810808593628;1716810808574000;68479.0;5.5;ask;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(1, data.size());
    std::remove(filepath.c_str());
}

// Тест 12: Пустые строки в середине файла пропускаются
TEST(csv_parser_tests, skips_empty_lines_in_middle) {
    const std::string filepath = "test_empty_lines.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.0;10.1;bid;1\n"
        "\n"
        "1716810808593628;1716810808574000;68479.0;5.5;ask;0\n"
        "\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(2, data.size());
    std::remove(filepath.c_str());
}

// Тест 13: Корректный парсинг большого количества строк
TEST(csv_parser_tests, parse_many_rows) {
    const std::string filepath = "test_many_rows.csv";
    std::string content = "receive_ts;exchange_ts;price;quantity;side;rebuild\n";
    const int row_count = 1000;
    for (int i = 0; i < row_count; ++i) {
        content += std::to_string(1716810808593627LL + i)
            + ";1716810808574000;68480.0;10.1;bid;0\n";
    }
    create_temp_file(filepath, content);

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(row_count, data.size());
    EXPECT_EQ(1716810808593627LL, data[0].receive_ts);
    EXPECT_EQ(1716810808593627LL + row_count - 1, data[row_count - 1].receive_ts);
    std::remove(filepath.c_str());
}

// Тест 14: parse_trades — успешный парсинг
TEST(csv_parser_tests, parse_trades_success) {
    const std::string filepath = "test_trades.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side\n"
        "1716810808593627;1716810808574000;68480.0;2.5;buy\n"
        "1716810808593628;1716810808574000;68479.0;1.0;sell\n");

    const auto data = csv_parser::parse_trades(filepath, ";");

    ASSERT_EQ(2, data.size());
    EXPECT_EQ(1716810808593627LL, data[0].receive_ts);
    EXPECT_DOUBLE_EQ(68480.0, data[0].price);
    EXPECT_EQ("buy", data[0].side);
    EXPECT_EQ("sell", data[1].side);
    std::remove(filepath.c_str());
}

// Тест 15: parse_trades — выброс исключения при отсутствии файла
TEST(csv_parser_tests, parse_trades_throws_on_missing_file) {
    EXPECT_THROW(
        { const auto data = csv_parser::parse_trades("non_existent_trades.csv", ";"); },
        std::runtime_error
    );
}

// Тест 16: parse_trades — битая строка пропускается
TEST(csv_parser_tests, parse_trades_skips_invalid_rows) {
    const std::string filepath = "test_trades_invalid.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side\n"
        "1716810808593627;1716810808574000;68480.0;2.5;buy\n"
        "BROKEN_ROW\n");

    const auto data = csv_parser::parse_trades(filepath, ";");

    ASSERT_EQ(1, data.size());
    std::remove(filepath.c_str());
}

// Тест 17: parse_trades — нечисловая цена пропускается
TEST(csv_parser_tests, parse_trades_skips_invalid_price) {
    const std::string filepath = "test_trades_bad_price.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side\n"
        "1716810808593627;1716810808574000;BAD_PRICE;2.5;buy\n"
        "1716810808593628;1716810808574000;68479.0;1.0;sell\n");

    const auto data = csv_parser::parse_trades(filepath, ";");

    ASSERT_EQ(1, data.size());
    EXPECT_EQ("sell", data[0].side);
    std::remove(filepath.c_str());
}

// Тест 18: Нулевая цена и нулевое количество парсятся корректно
TEST(csv_parser_tests, zero_price_and_quantity) {
    const std::string filepath = "test_zero_values.csv";
    create_temp_file(filepath,
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;0.0;0.0;bid;0\n");

    const auto data = csv_parser::parse_levels(filepath, ";");

    ASSERT_EQ(1, data.size());
    EXPECT_DOUBLE_EQ(0.0, data[0].price);
    EXPECT_DOUBLE_EQ(0.0, data[0].quantity);
    std::remove(filepath.c_str());
}

} // namespace exchange_data::tests