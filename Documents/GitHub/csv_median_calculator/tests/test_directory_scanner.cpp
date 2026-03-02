/**
 * \file test_directory_scanner.cpp
 * \author github:poshlikushat
 * \brief Unit-тесты для directory_scanner
 * \date 2026-02-27
 * \version 1.1
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

void create_file(const fs::path& path_) {
    std::ofstream f(path_);
    f << "test";
}

class directory_scanner_tests : public ::testing::Test {
protected:
    fs::path test_dir;

    void SetUp() override {
        test_dir = fs::temp_directory_path()
            / ("scanner_test_" + std::to_string(
                std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);
    }

    directory_scanner make_scanner(const std::vector<std::string>& masks_) {
        return {test_dir, masks_};
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

    ASSERT_EQ(1u, files.size());
    EXPECT_FALSE(ec);
}

// Тест 4: Не-CSV файл игнорируется
TEST_F(directory_scanner_tests, ignores_non_csv) {
    create_file(test_dir / "levels_2024.txt");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 5: CSV без подходящей маски игнорируется
TEST_F(directory_scanner_tests, ignores_csv_without_mask_match) {
    create_file(test_dir / "trades_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 6: Несколько файлов — находятся только подходящие
TEST_F(directory_scanner_tests, finds_only_matching_files) {
    create_file(test_dir / "levels_2024.csv");
    create_file(test_dir / "trades_2024.csv");
    create_file(test_dir / "readme.txt");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1u, files.size());
}

// Тест 7: Несколько масок — находятся файлы под каждую
TEST_F(directory_scanner_tests, finds_files_for_multiple_masks) {
    create_file(test_dir / "levels_2024.csv");
    create_file(test_dir / "trades_2024.csv");
    auto scanner = make_scanner({"levels", "trades"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(2u, files.size());
}

// Тест 8: Нет ошибки при успешном сканировании непустой директории
TEST_F(directory_scanner_tests, no_error_code_on_success) {
    create_file(test_dir / "levels_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_FALSE(ec);
}

// Тест 9: Вложенные директории не сканируются (только top-level)
TEST_F(directory_scanner_tests, does_not_recurse_into_subdirs) {
    fs::create_directories(test_dir / "subdir");
    create_file(test_dir / "subdir" / "levels_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 10: Регистр расширения важен — ".CSV" игнорируется
TEST_F(directory_scanner_tests, uppercase_csv_extension_ignored) {
    create_file(test_dir / "levels_2024.CSV");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    EXPECT_TRUE(files.empty());
}

// Тест 11: Возвращаемые пути абсолютные
TEST_F(directory_scanner_tests, returned_paths_are_absolute) {
    create_file(test_dir / "levels_2024.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(1u, files.size());
    EXPECT_TRUE(files[0].is_absolute());
}

// Тест 12: КЛЮЧЕВОЙ — файл совпадает с несколькими масками, дублей нет
TEST_F(directory_scanner_tests, no_duplicates_when_file_matches_multiple_masks) {
    create_file(test_dir / "levels_trades_2024.csv");
    auto scanner = make_scanner({"levels", "trades"});
    auto [files, ec] = scanner.scan();

    // Файл содержит обе подстроки — должен быть в результате ровно один раз
    ASSERT_EQ(1u, files.size());
}

// Тест 13: Большое количество файлов — все подходящие находятся
TEST_F(directory_scanner_tests, finds_all_among_many_files) {
    for (int i = 0; i < 50; ++i) {
        create_file(test_dir / ("levels_" + std::to_string(i) + ".csv"));
        create_file(test_dir / ("other_"  + std::to_string(i) + ".csv"));
    }
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(50u, files.size());
}

// Тест 14: Пустой список масок — возвращаются все CSV-файлы
TEST_F(directory_scanner_tests, empty_masks_returns_all_csv) {
    create_file(test_dir / "levels_2024.csv");
    create_file(test_dir / "trades_2024.csv");
    create_file(test_dir / "readme.txt");
    auto scanner = make_scanner({});
    auto [files, ec] = scanner.scan();

    // Пустые маски = принять все CSV
    ASSERT_EQ(2u, files.size());
}

// Тест 15: Результаты отсортированы (детерминированный порядок)
TEST_F(directory_scanner_tests, results_are_sorted) {
    create_file(test_dir / "z_levels.csv");
    create_file(test_dir / "a_levels.csv");
    create_file(test_dir / "m_levels.csv");
    auto scanner = make_scanner({"levels"});
    auto [files, ec] = scanner.scan();

    ASSERT_EQ(3u, files.size());
    // std::set сортирует по пути — первый файл должен быть "a_levels.csv"
    EXPECT_EQ("a_levels.csv", files[0].filename().string());
    EXPECT_EQ("m_levels.csv", files[1].filename().string());
    EXPECT_EQ("z_levels.csv", files[2].filename().string());
}

} // namespace app::io::tests