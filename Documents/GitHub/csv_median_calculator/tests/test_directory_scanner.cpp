/**
 * \file test_directory_scanner.cpp
 * \author github:poshlikushat
 * \brief Unit-тесты для directory_scanner
 * \date 2026-02-24
 * \version 1.0
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../src/directory_scanner.hpp"

namespace app::io::tests {

namespace fs = std::filesystem;

// ─── Вспомогательные функции ────────────────────────────────────────────────

void create_file(const fs::path& path) {
    std::ofstream f(path);
    f << "test";
}

class directory_scanner_tests : public ::testing::Test {
protected:
    fs::path test_dir;

	void SetUp() override {
		test_dir = fs::temp_directory_path()
				/ ("scanner_test_" + std::to_string(
						std::chrono::high_resolution_clock::now().time_since_epoch().count()
				));
		fs::create_directories(test_dir);
	}

	void TearDown() override {
		fs::remove_all(test_dir);
	}

    directory_scanner make_scanner(const std::vector<std::string>& masks) {
        return {test_dir, masks};
    }
};

// ─── Тесты ──────────────────────────────────────────────────────────────────

// Тест 1: Несуществующая директория возвращает ошибку
TEST_F(directory_scanner_tests, nonexistent_dir_returns_error) {
    fs::path bad_dir = test_dir / "nonexistent";
    directory_scanner scanner(bad_dir, {});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
    EXPECT_EQ(ec, std::make_error_code(std::errc::no_such_file_or_directory));
}

// Тест 2: Пустая директория возвращает пустой список
TEST_F(directory_scanner_tests, empty_dir_returns_empty) {
    auto scanner = make_scanner({});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
    EXPECT_FALSE(ec);
}

// Тест 3: CSV файл с подходящей маской находится
TEST_F(directory_scanner_tests, finds_csv_matching_mask) {
    create_file(test_dir / "levels_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
    EXPECT_EQ("levels_2024.csv", files[0].filename().string());
    EXPECT_FALSE(ec);
}

// Тест 4: CSV файл без подходящей маски не находится
TEST_F(directory_scanner_tests, ignores_csv_not_matching_mask) {
    create_file(test_dir / "trades_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 5: Не-CSV файл игнорируется даже если совпадает с маской
TEST_F(directory_scanner_tests, ignores_non_csv_files) {
    create_file(test_dir / "levels_2024.txt");
    create_file(test_dir / "levels_2024.json");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 6: Несколько масок — файл находится если совпадает хотя бы одна
TEST_F(directory_scanner_tests, finds_file_matching_any_mask) {
    create_file(test_dir / "trades_2024.csv");
    auto scanner = make_scanner({"levels", "trades"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
    EXPECT_EQ("trades_2024.csv", files[0].filename().string());
}

// Тест 7: Несколько файлов — находятся все подходящие
TEST_F(directory_scanner_tests, finds_multiple_matching_files) {
    create_file(test_dir / "levels_btc.csv");
    create_file(test_dir / "levels_eth.csv");
    create_file(test_dir / "trades_btc.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(2, files.size());
}

// Тест 8: Смешанные файлы — находятся только подходящие
TEST_F(directory_scanner_tests, mixed_files_only_matching_returned) {
    create_file(test_dir / "levels_btc.csv");
    create_file(test_dir / "trades_btc.csv");
    create_file(test_dir / "readme.txt");
    create_file(test_dir / "config.toml");
    auto scanner = make_scanner({"levels", "trades"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(2, files.size());
}

// Тест 9: Пустой список масок — ни один CSV не находится
TEST_F(directory_scanner_tests, empty_masks_returns_nothing) {
    create_file(test_dir / "levels_2024.csv");
    create_file(test_dir / "trades_2024.csv");
    auto scanner = make_scanner({});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 10: Маска совпадает с частью имени файла (подстрока)
TEST_F(directory_scanner_tests, mask_matches_substring) {
    create_file(test_dir / "2024_01_01_levels_btc_usdt.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
}

// Тест 11: Регистр маски важен (case-sensitive)
TEST_F(directory_scanner_tests, mask_is_case_sensitive) {
    create_file(test_dir / "LEVELS_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 12: Файл с именем равным маске находится
TEST_F(directory_scanner_tests, filename_equals_mask) {
    create_file(test_dir / "levels.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
}

// Тест 13: Директория внутри не попадает в результат
TEST_F(directory_scanner_tests, subdirectory_is_ignored) {
    fs::create_directory(test_dir / "levels_subdir");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 14: Файл без расширения игнорируется
TEST_F(directory_scanner_tests, file_without_extension_ignored) {
    create_file(test_dir / "levels_2024");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 15: Файл с расширением .CSV (верхний регистр) игнорируется
TEST_F(directory_scanner_tests, uppercase_csv_extension_ignored) {
    create_file(test_dir / "levels_2024.CSV");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 16: Возвращаемый путь абсолютный
TEST_F(directory_scanner_tests, returned_paths_are_absolute) {
    create_file(test_dir / "levels_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
    EXPECT_TRUE(files[0].is_absolute());
}

// Тест 17: Несколько масок — нет дублирования если файл содержит обе
TEST_F(directory_scanner_tests, no_duplicates_when_file_matches_multiple_masks) {
    create_file(test_dir / "levels_trades_2024.csv");
    auto scanner = make_scanner({"levels", "trades"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
}

// Тест 18: Большое количество файлов — все подходящие находятся
TEST_F(directory_scanner_tests, finds_all_among_many_files) {
    for (int i = 0; i < 50; ++i) {
        create_file(test_dir / ("levels_" + std::to_string(i) + ".csv"));
        create_file(test_dir / ("other_" + std::to_string(i) + ".csv"));
    }
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(50, files.size());
}

// Тест 19: Путь к директории передаётся как rvalue (move)
TEST_F(directory_scanner_tests, accepts_moved_path) {
    create_file(test_dir / "levels_2024.csv");
    fs::path dir_copy = test_dir;
    directory_scanner scanner(dir_copy, {"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1, files.size());
}

// Тест 20: Нет ошибки при успешном сканировании непустой директории
TEST_F(directory_scanner_tests, no_error_code_on_success) {
    create_file(test_dir / "levels_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_FALSE(ec);
}

} // namespace app::io::tests