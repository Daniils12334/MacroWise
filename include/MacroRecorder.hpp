#pragma once

#include <atomic>
#include <vector>
#include <linux/input.h>
#include "utils.hpp"

class MacroRecorder {
public:
    MacroRecorder(const std::string& mouse_device, const std::string& keyboard_device);
    ~MacroRecorder();
    
    void start_recording(const std::string& macro_name);
    void stop_recording();
    void play_macro(const std::string& macro_name, int loop_count = 1);
    void list_macros() const;
    
    bool is_recording() const { return recording; }
    bool should_exit() const { return should_exit_flag; }
    
    void set_macros_directory(const std::string& dir) { macros_dir = dir; }
    void set_start_delay(int delay) { start_delay = delay; }
    
private:
    std::string mouse_device;
    std::string keyboard_device;
    std::string macros_dir;
    int start_delay;
    
    std::atomic<bool> recording;
    std::atomic<bool> should_exit_flag;
    std::vector<input_event> events;
    timeval start_time;
    
    void record_events();
    void play_macro_loop(const std::vector<input_event>& macro_events, int start_x, int start_y, int loop_count);
};