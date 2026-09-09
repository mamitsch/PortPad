#include "FileBrowser.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct Fixture {
    fs::path path;
    Fixture() {
        auto pattern = (fs::temp_directory_path() / "portpad-test-XXXXXX").string();
        const auto directory = mkdtemp(pattern.data());
        if (!directory) throw std::runtime_error("Cannot create test directory");
        path = directory;
    }
    ~Fixture() {
        std::error_code error;
        fs::permissions(path / "locked", fs::perms::owner_all, error);
        fs::remove_all(path, error);
    }
};
int main() {
    using namespace portpad;
    try {
        Fixture fixture;
        fs::create_directory(fixture.path / "z-dir");
        fs::create_directory(fixture.path / "a-dir");
        std::ofstream(fixture.path / "z.txt") << "unchanged";
        std::ofstream(fixture.path / "a.txt") << "hello";
        std::ofstream(fixture.path / ".hidden") << "hidden";
        const auto listing = listDirectory(fixture.path);
        check(listing.error.empty() && listing.entries.size() == 5, "all entries listed");
        check(listing.entries[0].path.filename() == "a-dir" && listing.entries[1].path.filename() == "z-dir", "directories first and sorted");
        check(listing.entries[2].path.filename() == ".hidden" && listing.entries[3].path.filename() == "a.txt" && listing.entries[4].path.filename() == "z.txt", "files sorted including hidden files");
        check(listDirectory(fixture.path / "a-dir").entries.empty(), "empty directory");
        check(!listDirectory(fixture.path / "missing").error.empty(), "missing directory error");
        check(!listDirectory(fixture.path / "a.txt").error.empty(), "file is not a directory");
        FileBrowser browser;
        check(browser.open(fixture.path), "open initial directory");
        browser.handle(InputAction::Up);
        check(browser.selected() == 5, "up wraps");
        browser.handle(InputAction::Confirm);
        check(!browser.message().empty() && browser.path() == fixture.path, "file confirm stays in directory");
        browser.handle(InputAction::Down);
        browser.handle(InputAction::Down);
        browser.handle(InputAction::Confirm);
        check(browser.path() == fixture.path / "a-dir" && browser.entries().size() == 1 && browser.entries()[0].path.filename() == "..", "empty directory has parent entry");
        browser.handle(InputAction::Up);
        browser.handle(InputAction::Down);
        check(browser.selected() == 0, "empty directory navigation safe");
        browser.handle(InputAction::Back);
        check(browser.path() == fixture.path / "a-dir", "Back does not navigate parent");
        browser.handle(InputAction::Confirm);
        check(browser.path() == fixture.path && browser.selected() == 1, "parent restores child selection");
        fs::remove(fixture.path / "a-dir");
        browser.handle(InputAction::Confirm);
        check(browser.path() == fixture.path && !browser.message().empty(), "failed navigation preserves current listing");
        check(browser.open(fixture.path / "z-dir") && browser.message().empty(), "successful navigation clears error");
        fs::create_directory(fixture.path / "locked");
        fs::permissions(fixture.path / "locked", fs::perms::none);
        const auto locked = listDirectory(fixture.path / "locked");
        if (locked.error.empty()) std::cout << "Permission-denied check skipped: process can access mode-000 directory\n";
        else check(locked.entries.empty(), "inaccessible directory returns error without partial listing");
        check(browser.open(fixture.path.root_path()), "open filesystem root");
        browser.handle(InputAction::Back);
        check(browser.path() == fixture.path.root_path(), "back at root stays at root");
        std::ifstream file(fixture.path / "z.txt");
        std::string content;
        file >> content;
        check(content == "unchanged", "browser does not change file contents");
        for (int i = 0; i < 20; ++i) std::ofstream(fixture.path / ("page-" + std::to_string(i)));
        check(browser.open(fixture.path), "open paging fixture");
        browser.handle(InputAction::PageDown);
        check(browser.selected() == FileBrowser::pageSize, "browser page down");
        for (int i = 0; i < 20; ++i) browser.handle(InputAction::PageDown);
        check(browser.selected() == browser.entries().size() - 1, "page down clamps at end");
        for (int i = 0; i < 20; ++i) browser.handle(InputAction::PageUp);
        check(browser.selected() == 0, "page up clamps at start");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
