#pragma once

#include <string>
#include <vector>
#include <functional>
#include <ncurses.h>

class Interface {
public:
    Interface();
    ~Interface();
    
    void run();
    void show_menu();
    void show_recording_screen(const std::string& macro_name);
    void show_playback_screen(const std::string& macro_name, int loop_count);
    void show_macros_list(const std::vector<std::string>& macros);
    void show_message(const std::string& message);
    void show_error(const std::string& error);
    
    void set_recording_callback(std::function<void(const std::string&)> callback);
    void set_playback_callback(std::function<void(const std::string&, int)> callback);
    void set_list_callback(std::function<std::vector<std::string>()> callback);
    void set_stop_recording_callback(std::function<void()> callback);
    void set_recording_status_callback(std::function<bool()> callback);
    
private:
    WINDOW* main_win;
    WINDOW* status_win;
    
    std::function<void(const std::string&)> recording_callback;
    std::function<void(const std::string&, int)> playback_callback;
    std::function<std::vector<std::string>()> list_callback;
    std::function<void()> stop_recording_callback;
    std::function<bool()> recording_status_callback;
    
    void init_colors();
    void draw_border();
    void clear_status();
    void update_status(const std::string& message);
    std::string get_input(const std::string& prompt);
    int get_number_input(const std::string& prompt);
};