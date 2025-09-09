#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <cstdio>
#include <sstream>

namespace utils {

void print_devices() {
    std::cout << "Available devices:" << std::endl;
    for (int i = 0; i < 32; i++) {
        std::string path = "/dev/input/event" + std::to_string(i);
        if (fs::exists(path)) {
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd != -1) {
                char name[256] = "Unknown";
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                std::cout << "event" << i << ": " << name;
                
                // Проверка доступности для чтения
                if (access(path.c_str(), R_OK) == 0) {
                    std::cout << " (readable)";
                } else {
                    std::cout << " (NO READ ACCESS)";
                }
                std::cout << std::endl;
                
                close(fd);
            }
        }
    }
}

int get_current_cursor_position(int& x, int& y) {
    // Check if xdotool is available
    if (system("which xdotool > /dev/null 2>&1") != 0) {
        x = 960;
        y = 540;
        return -1;
    }

    FILE* fp = popen("xdotool getmouselocation", "r");
    if (fp) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), fp)) {
            if (sscanf(buffer, "x:%d y:%d", &x, &y) == 2) {
                pclose(fp);
                return 0;
            }
        }
        pclose(fp);
    }

    x = 960;
    y = 540;
    return -1;
}

void set_cursor_position(int x, int y) {
    // Check if xdotool is available
    if (system("which xdotool > /dev/null 2>&1") != 0) {
        std::cout << "Warning: xdotool not available, cannot set cursor position" << std::endl;
        return;
    }

    char command[128];
    snprintf(command, sizeof(command), "xdotool mousemove %d %d", x, y);
    system(command);
}

int get_loop_count_from_user() {
    int loop_count = 1;
    std::string input;

    std::cout << "Enter number of loops (0 for infinite, 1 for single, >1 for multiple): ";
    std::getline(std::cin, input);

    if (input.empty()) {
        return 1;
    }

    if (input == "0" || input == "inf" || input == "infinite") {
        return -1;
    }

    try {
        loop_count = std::stoi(input);
        if (loop_count < 1) {
            loop_count = -1;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid input, using single loop" << std::endl;
        loop_count = 1;
    }

    return loop_count;
}

std::vector<std::string> list_macros(const std::string& macros_dir) {
    std::vector<std::string> macros;
    try {
        for (const auto& entry : fs::directory_iterator(macros_dir)) {
            if (entry.path().extension() == ".macro") {
                macros.push_back(entry.path().stem().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << "Error reading macros directory: " << e.what() << std::endl;
    }
    return macros;
}

bool read_macro_file(const std::string& filename, MacroHeader& header, std::vector<input_event>& events) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        return false;
    }

    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    input_event ev;
    while (in.read(reinterpret_cast<char*>(&ev), sizeof(ev))) {
        events.push_back(ev);
    }
    
    return true;
}

bool write_macro_file(const std::string& filename, const MacroHeader& header, const std::vector<input_event>& events) {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(events.data()), events.size() * sizeof(input_event));
    
    return true;
}

} // namespace utils