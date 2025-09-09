#include "MacroRecorder.hpp"
#include "utils.hpp"
#include "UInputDevice.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <thread>
#include <cstring>

MacroRecorder::MacroRecorder(const std::string& mouse_device, const std::string& keyboard_device)
    : mouse_device(mouse_device), keyboard_device(keyboard_device),
      macros_dir("/macroses"), start_delay(3),
      recording(false), should_exit_flag(false) {}

MacroRecorder::~MacroRecorder() {
    stop_recording();
}

void MacroRecorder::start_recording(const std::string& macro_name) {
    if (recording) {
        std::cout << "Already recording!" << std::endl;
        return;
    }

    recording = true;
    events.clear();
    
    int mouse_fd = open(mouse_device.c_str(), O_RDONLY | O_NONBLOCK);
    int keyboard_fd = open(keyboard_device.c_str(), O_RDONLY | O_NONBLOCK);

    if (mouse_fd == -1 || keyboard_fd == -1) {
        perror("Error opening input devices");
        recording = false;
        return;
    }

    int start_x, start_y;
    utils::get_current_cursor_position(start_x, start_y);
    std::cout << "Starting cursor position: " << start_x << ", " << start_y << std::endl;

    timeval start_time;
    gettimeofday(&start_time, nullptr);
    std::cout << "Recording started... Press F9 to stop" << std::endl;

    while (recording && !should_exit_flag) {
        input_event ev;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mouse_fd, &fds);
        FD_SET(keyboard_fd, &fds);

        timeval timeout{0, 100000};
        int max_fd = std::max(mouse_fd, keyboard_fd) + 1;
        
        if (select(max_fd, &fds, nullptr, nullptr, &timeout) > 0) {
            if (FD_ISSET(mouse_fd, &fds) && read(mouse_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                timeval current_time, relative_time;
                gettimeofday(&current_time, nullptr);
                timersub(&current_time, &start_time, &relative_time);
                
                input_event timed_ev = ev;
                timed_ev.time = relative_time;
                events.push_back(timed_ev);
            }
            
            if (FD_ISSET(keyboard_fd, &fds) && read(keyboard_fd, &ev, sizeof(ev)) == sizeof(ev) && ev.type == EV_KEY) {
                timeval current_time, relative_time;
                gettimeofday(&current_time, nullptr);
                timersub(&current_time, &start_time, &relative_time);
                
                input_event timed_ev = ev;
                timed_ev.time = relative_time;
                events.push_back(timed_ev);
            }
        }
    }

    close(mouse_fd);
    close(keyboard_fd);

    MacroHeader header{start_x, start_y};
    
    fs::create_directories(macros_dir);
    std::string filename = macros_dir + "/" + macro_name + ".macro";
    
    if (utils::write_macro_file(filename, header, events)) {
        std::cout << "Macro saved: " << filename << " (" << events.size() << " events)" << std::endl;
    } else {
        std::cout << "Error saving macro" << std::endl;
    }
    
    recording = false;
}

void MacroRecorder::stop_recording() {
    recording = false;
}

void MacroRecorder::record_events() {
    int mouse_fd = open(mouse_device.c_str(), O_RDONLY | O_NONBLOCK);
    int keyboard_fd = open(keyboard_device.c_str(), O_RDONLY | O_NONBLOCK);

    if (mouse_fd == -1 || keyboard_fd == -1) {
        perror("Error opening input devices");
        recording = false;
        return;
    }

    gettimeofday(&start_time, nullptr);
    std::cout << "Recording started... Press F9 to stop" << std::endl;

    while (recording && !should_exit_flag) {
        input_event ev;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mouse_fd, &fds);
        FD_SET(keyboard_fd, &fds);

        timeval timeout{0, 100000};
        int max_fd = std::max(mouse_fd, keyboard_fd) + 1;
        
        if (select(max_fd, &fds, nullptr, nullptr, &timeout) > 0) {
            if (FD_ISSET(mouse_fd, &fds) && read(mouse_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                timeval current_time, relative_time;
                gettimeofday(&current_time, nullptr);
                timersub(&current_time, &start_time, &relative_time);
                
                input_event timed_ev = ev;
                timed_ev.time = relative_time;
                events.push_back(timed_ev);
            }
            
            if (FD_ISSET(keyboard_fd, &fds) && read(keyboard_fd, &ev, sizeof(ev)) == sizeof(ev) && ev.type == EV_KEY) {
                timeval current_time, relative_time;
                gettimeofday(&current_time, nullptr);
                timersub(&current_time, &start_time, &relative_time);
                
                input_event timed_ev = ev;
                timed_ev.time = relative_time;
                events.push_back(timed_ev);
            }
        }
    }

    close(mouse_fd);
    close(keyboard_fd);
    recording = false;
}

void MacroRecorder::play_macro(const std::string& macro_name, int loop_count) {
    std::string filename = macros_dir + "/" + macro_name + ".macro";
    MacroHeader header;
    std::vector<input_event> macro_events;
    
    if (!utils::read_macro_file(filename, header, macro_events)) {
        std::cout << "Error opening macro file: " << filename << std::endl;
        return;
    }

    std::cout << "Starting playback in " << start_delay << " seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(start_delay));

    std::thread([this, macro_events, header, loop_count]() {
        play_macro_loop(macro_events, header.start_x, header.start_y, loop_count);
    }).detach();
}

void MacroRecorder::play_macro_loop(const std::vector<input_event>& macro_events, int start_x, int start_y, int loop_count) {
    UInputDevice uinput;
    if (!uinput.initialize()) {
        std::cout << "Failed to create virtual input device" << std::endl;
        return;
    }

    int current_loop = 0;
    bool infinite = (loop_count == -1);

    while ((infinite || current_loop < loop_count) && !should_exit_flag) {
        current_loop++;
        
        utils::set_cursor_position(start_x, start_y);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        timeval start_time;
        gettimeofday(&start_time, nullptr);

        for (size_t i = 0; i < macro_events.size(); i++) {
            const auto& event = macro_events[i];

            if (i > 0) {
                timeval current_time, elapsed;
                gettimeofday(&current_time, nullptr);
                timersub(&current_time, &start_time, &elapsed);

                if (timercmp(&elapsed, &event.time, <)) {
                    timeval sleep_time;
                    timersub(&event.time, &elapsed, &sleep_time);
                    long sleep_us = sleep_time.tv_sec * 1000000 + sleep_time.tv_usec;
                    if (sleep_us > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
                    }
                }
            }

            uinput.emit_event(event);
            
            if (should_exit_flag) break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (should_exit_flag) break;
    }

    std::cout << "Playback completed" << std::endl;
}

void MacroRecorder::list_macros() const {
    auto macros = utils::list_macros(macros_dir);
    std::cout << "Available macros:" << std::endl;
    for (const auto& macro : macros) {
        std::cout << "  " << macro << std::endl;
    }
}

