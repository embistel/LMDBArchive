#include <catch2/catch_test_macros.hpp>
#include "core/PathUtil.h"

TEST_CASE("PathUtil::normalizeArchivePath", "[path]") {
    SECTION("basic normalization") {
        REQUIRE(PathUtil::normalizeArchivePath("File.jpg") == "File.jpg");
        REQUIRE(PathUtil::normalizeArchivePath("Sub/File.jpg") == "Sub/File.jpg");
        REQUIRE(PathUtil::normalizeArchivePath("A/B/C.jpg") == "A/B/C.jpg");
    }

    SECTION("backslash to slash") {
        REQUIRE(PathUtil::normalizeArchivePath("Sub\\File.jpg") == "Sub/File.jpg");
        REQUIRE(PathUtil::normalizeArchivePath("A\\B\\C.jpg") == "A/B/C.jpg");
    }

    SECTION("remove leading/trailing slashes") {
        REQUIRE(PathUtil::normalizeArchivePath("/File.jpg") == "File.jpg");
        REQUIRE(PathUtil::normalizeArchivePath("Sub/") == "Sub");
        REQUIRE(PathUtil::normalizeArchivePath("/Sub/File.jpg/") == "Sub/File.jpg");
    }

    SECTION("collapse dot and dotdot") {
        REQUIRE(PathUtil::normalizeArchivePath("./File.jpg") == "File.jpg");
        REQUIRE(PathUtil::normalizeArchivePath("Sub/../File.jpg") == "File.jpg");
        REQUIRE(PathUtil::normalizeArchivePath("A/B/./C.jpg") == "A/B/C.jpg");
    }

    SECTION("empty input") {
        REQUIRE(PathUtil::normalizeArchivePath("").isEmpty());
        REQUIRE(PathUtil::normalizeArchivePath("/").isEmpty());
    }
}

TEST_CASE("PathUtil::splitArchivePath", "[path]") {
    REQUIRE(PathUtil::splitArchivePath("A/B/C.jpg").size() == 3);
    REQUIRE(PathUtil::splitArchivePath("File.jpg").size() == 1);
    REQUIRE(PathUtil::splitArchivePath("").isEmpty());
}

TEST_CASE("PathUtil::joinArchivePath", "[path]") {
    QStringList parts = {"A", "B", "C.jpg"};
    REQUIRE(PathUtil::joinArchivePath(parts) == "A/B/C.jpg");
}

TEST_CASE("PathUtil::isValidArchivePath", "[path]") {
    REQUIRE(PathUtil::isValidArchivePath("File.jpg") == true);
    REQUIRE(PathUtil::isValidArchivePath("Sub/File.jpg") == true);
    REQUIRE(PathUtil::isValidArchivePath("") == false);
    REQUIRE(PathUtil::isValidArchivePath("/etc/passwd") == false);
    REQUIRE(PathUtil::isValidArchivePath("../escape") == false);
}

TEST_CASE("PathUtil::parentPath", "[path]") {
    REQUIRE(PathUtil::parentPath("Sub/File.jpg") == "Sub");
    REQUIRE(PathUtil::parentPath("A/B/C.jpg") == "A/B");
    REQUIRE(PathUtil::parentPath("File.jpg").isEmpty());
}

TEST_CASE("PathUtil::fileName", "[path]") {
    REQUIRE(PathUtil::fileName("Sub/File.jpg") == "File.jpg");
    REQUIRE(PathUtil::fileName("File.jpg") == "File.jpg");
}

TEST_CASE("PathUtil::dirPath", "[path]") {
    REQUIRE(PathUtil::dirPath("Sub/File.jpg") == "Sub");
    REQUIRE(PathUtil::dirPath("A/B/C.jpg") == "A/B");
    REQUIRE(PathUtil::dirPath("File.jpg").isEmpty());
}

TEST_CASE("PathUtil::isUnderPrefix", "[path]") {
    REQUIRE(PathUtil::isUnderPrefix("A/B/C.jpg", "A/B") == true);
    REQUIRE(PathUtil::isUnderPrefix("A/B/C.jpg", "A") == true);
    REQUIRE(PathUtil::isUnderPrefix("A/B/C.jpg", "D") == false);
    REQUIRE(PathUtil::isUnderPrefix("A/B/C.jpg", "") == true);
}
