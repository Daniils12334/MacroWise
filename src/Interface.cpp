#include "Interface.hpp"
#include <iostream>
#include <thread>
#include <chrono>

Interface::Interface() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    init_colors();
    
    // Create main window
    main_win = newwin(LINES - 3, COLS, 0, 0);
    status_win = newwin(3, COLS, LINES - 3, 0);
    
    draw_border();
    refresh();
}

Interface::~Interface() {
    delwin(main_win);
    delwin(status_win);
    endwin();
}

void Interface::init_colors() {
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
    }
}

void Interface::draw_border() {
    if (has_colors()) {
        wattron(status_win, COLOR_PAIR(1));
    }
    box(status_win, 0, 0);
    if (has_colors()) {
        wattroff(status_win, COLOR_PAIR(1));
    }
    wrefresh(status_win);
}

void Interface::clear_status() {
    wclear(status_win);
    draw_border();
    wrefresh(status_win);
}

void Interface::update_status(const std::string& message) {
    clear_status();
    mvwprintw(status_win, 1, 2, "%s", message.c_str());
    wrefresh(status_win);
}

std::string Interface::get_input(const std::string& prompt) {
    echo();
    curs_set(1);
    
    WINDOW* input_win = newwin(3, COLS - 4, LINES / 2 - 1, 2);
    box(input_win, 0, 0);
    mvwprintw(input_win, 1, 2, "%s", prompt.c_str());
    wrefresh(input_win);
    
    char input[256];
    mvwgetnstr(input_win, 1, prompt.length() + 3, input, sizeof(input) - 1);
    
    delwin(input_win);
    noecho();
    curs_set(0);
    
    return std::string(input);
}

int Interface::get_number_input(const std::string& prompt) {
    std::string input = get_input(prompt);
    try {
        return std::stoi(input);
    } catch (...) {
        return 1;
    }
}

void Interface::run() {
    int choice;
    bool running = true;
    
    while (running) {
        show_menu();
        choice = getch();
        
        switch (choice) {
            case '1': {
                std::string name = get_input("Enter macro name: ");
                if (!name.empty()) {
                    show_recording_screen(name);
                    if (recording_callback) {
                        recording_callback(name);
                    }
                    
                    // Wait for recording to complete
                    timeout(100);
                    while (recording_status_callback && recording_status_callback()) {
                        int ch = getch();
                        if (ch != ERR) {
                            // Handle any key press if needed
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    timeout(-1);
                    
                    show_message("Recording finished");
                }
                break;
            }
            case '2': {
                std::string name = get_input("Enter macro name to play: ");
                if (!name.empty()) {
                    int loops = get_number_input("Enter number of loops (0 for infinite): ");
                    show_playback_screen(name, loops);
                    if (playback_callback) {
                        playback_callback(name, loops);
                    }
                }
                break;
            }
            case '3': {
                if (list_callback) {
                    std::vector<std::string> macros = list_callback();
                    show_macros_list(macros);
                }
                break;
            }
            case '4':
                running = false;
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
        }
    }
}


void Interface::show_menu() {
    wclear(main_win);
    
    if (has_colors()) {
        wattron(main_win, COLOR_PAIR(5));
    }
    
    mvwprintw(main_win, 2, COLS/2 - 10, "MACRO RECORDER");
    
    if (has_colors()) {
        wattroff(main_win, COLOR_PAIR(5));
        wattron(main_win, COLOR_PAIR(3));
    }
    
    mvwprintw(main_win, 5, 5, "1. Record new macro");
    mvwprintw(main_win, 6, 5, "2. Play macro");
    mvwprintw(main_win, 7, 5, "3. List macros");
    mvwprintw(main_win, 8, 5, "4. Exit");
    mvwprintw(main_win, 10, 5, "Note: Use terminal for F9 to stop recording");
    
    if (has_colors()) {
        wattroff(main_win, COLOR_PAIR(3));
    }
    
    wrefresh(main_win);
    update_status("Use 1-4 to select option");
}

void Interface::show_recording_screen(const std::string& macro_name) {
    wclear(main_win);
    
    if (has_colors()) {
        wattron(main_win, COLOR_PAIR(2));
    }
    
    mvwprintw(main_win, 2, COLS/2 - 8, "RECORDING");
    mvwprintw(main_win, 4, 5, "Macro: %s", macro_name.c_str());
    mvwprintw(main_win, 6, 5, "Recording started...");
    mvwprintw(main_win, 8, 5, "Press F9 in terminal to stop");
    
    if (has_colors()) {
        wattroff(main_win, COLOR_PAIR(2));
    }
    
    wrefresh(main_win);
    update_status("Recording in progress... Press F9 in terminal to stop");
}

void Interface::show_playback_screen(const std::string& macro_name, int loop_count) {
    wclear(main_win);
    
    if (has_colors()) {
        wattron(main_win, COLOR_PAIR(3));
    }
    
    mvwprintw(main_win, 2, COLS/2 - 8, "PLAYBACK");
    mvwprintw(main_win, 4, 5, "Macro: %s", macro_name.c_str());
    mvwprintw(main_win, 5, 5, "Loops: %s", 
              loop_count == 0 ? "Infinite" : std::to_string(loop_count).c_str());
    mvwprintw(main_win, 7, 5, "Playback started...");
    
    if (has_colors()) {
        wattroff(main_win, COLOR_PAIR(3));
    }
    
    wrefresh(main_win);
    update_status("Playback in progress...");
}

void Interface::show_macros_list(const std::vector<std::string>& macros) {
    wclear(main_win);
    
    if (has_colors()) {
        wattron(main_win, COLOR_PAIR(4));
    }
    
    mvwprintw(main_win, 2, COLS/2 - 10, "AVAILABLE MACROS");
    
    if (has_colors()) {
        wattroff(main_win, COLOR_PAIR(4));
    }
    
    if (macros.empty()) {
        mvwprintw(main_win, 4, 5, "No macros found");
    } else {
        for (size_t i = 0; i < macros.size() && i < LINES - 8; i++) {
            mvwprintw(main_win, 4 + i, 5, "%zu. %s", i + 1, macros[i].c_str());
        }
    }
    
    mvwprintw(main_win, LINES - 6, 5, "Press any key to continue");
    wrefresh(main_win);
    update_status("Found " + std::to_string(macros.size()) + " macros");
    getch();
}

void Interface::show_message(const std::string& message) {
    update_status(message);
}

void Interface::show_error(const std::string& error) {
    if (has_colors()) {
        wattron(status_win, COLOR_PAIR(2));
    }
    update_status("ERROR: " + error);
    if (has_colors()) {
        wattroff(status_win, COLOR_PAIR(2));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void Interface::set_recording_callback(std::function<void(const std::string&)> callback) {
    recording_callback = callback;
}

void Interface::set_playback_callback(std::function<void(const std::string&, int)> callback) {
    playback_callback = callback;
}

void Interface::set_list_callback(std::function<std::vector<std::string>()> callback) {
    list_callback = callback;
}

void Interface::set_stop_recording_callback(std::function<void()> callback) {
    stop_recording_callback = callback;
}

void Interface::set_recording_status_callback(std::function<bool()> callback) {
    recording_status_callback = callback;
}