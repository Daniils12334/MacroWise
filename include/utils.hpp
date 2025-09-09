#pragma once

#include <string>
#include <vector>
#include <linux/input.h>

// Filesystem compatibility
#if defined(USE_EXPERIMENTAL_FILESYSTEM)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

struct MacroHeader {
    int start_x;
    int start_y;
};

namespace utils {
    void print_devices();
    int get_current_cursor_position(int& x, int& y);
    void set_cursor_position(int x, int y);
    int get_loop_count_from_user();
    std::vector<std::string> list_macros(const std::string& macros_dir);
    bool read_macro_file(const std::string& filename, MacroHeader& header, std::vector<input_event>& events);
    bool write_macro_file(const std::string& filename, const MacroHeader& header, const std::vector<input_event>& events);
}