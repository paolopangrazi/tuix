#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include "config/config.hpp"
#include "support/portable_env.hpp"

// Points XDG_CONFIG_HOME at a private temp root and writes/loads
// <root>/tuix/config.toml. `tag` should be unique per test file (e.g. "cfg",
// "save") so two files' env-var pokes don't share a directory.
namespace tuix::test {

class ConfigTestEnv {
public:
    explicit ConfigTestEnv(const std::string& tag)
        : m_dir(std::filesystem::temp_directory_path() /
                ("tuix_" + tag + "_test_" + unique_tag()) / "tuix") {
        std::filesystem::create_directories(m_dir);
    }

    std::filesystem::path file() const { return m_dir / "config.toml"; }

    // Writes `body` as config.toml and returns a freshly loaded Config.
    Config load(const std::string& body) {
        { std::ofstream(file()) << body; }
        set_env("XDG_CONFIG_HOME", m_dir.parent_path().string());
        return Config::load();
    }

private:
    std::filesystem::path m_dir;
};

}  // namespace tuix::test
