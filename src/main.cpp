#include "MacroRecorder.hpp"
#include "utils.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <atomic>
#include <thread>
#include <termios.h>
#include <unistd.h>

std::atomic<bool> waiting_for_name(false);

void clear_input_buffer() {
    tcflush(STDIN_FILENO, TCIFLUSH);
}

std::string read_input_safely() {
    std::string input;
    clear_input_buffer();
    std::getline(std::cin, input);
    return input;
}

// Функция для получения устройства по номеру или полному пути
std::string get_device_path(const std::string& input, const std::string& default_device) {
    if (input.empty()) {
        return default_device;
    }
    
    // Если ввод состоит только из цифр, добавляем префикс
    if (input.find_first_not_of("0123456789") == std::string::npos) {
        return "/dev/input/event" + input;
    }
    
    // Если ввод уже содержит полный путь, возвращаем как есть
    if (input.find("/dev/input/") == 0) {
        return input;
    }
    
    // Добавляем префикс если нужно
    if (input.find("event") == 0) {
        return "/dev/input/" + input;
    }
    
    return input; // Возвращаем как есть (может быть другим путем)
}

void monitor_keyboard(MacroRecorder& recorder, const std::string& keyboard_device) {
    int kbd_fd = open(keyboard_device.c_str(), O_RDONLY | O_NONBLOCK);
    if (kbd_fd == -1) {
        perror(("Error opening keyboard device: " + keyboard_device).c_str());
        utils::print_devices();
        return;
    }

    std::cout << "\n=== Macro Recorder Controls ===" << std::endl;
    std::cout << "F8  - Start recording" << std::endl;
    std::cout << "F9  - Stop recording" << std::endl;
    std::cout << "F10 - Play macro" << std::endl;
    std::cout << "F11 - List macros" << std::endl;
    std::cout << "Esc - Exit" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << "Using keyboard: " << keyboard_device << std::endl;

    while (!recorder.should_exit()) {
        input_event ev;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(kbd_fd, &fds);

        timeval timeout{0, 100000};
        
        if (select(kbd_fd + 1, &fds, nullptr, nullptr, &timeout) > 0 && FD_ISSET(kbd_fd, &fds)) {
            if (read(kbd_fd, &ev, sizeof(ev)) == sizeof(ev) && ev.type == EV_KEY && ev.value == 1) {
                
                if (waiting_for_name) continue;

                switch (ev.code) {
                    case 66: // F8
                        if (!recorder.is_recording()) {
                            std::cout << "\n=== RECORDING ===" << std::endl;
                            std::cout << "Enter macro name: ";
                            std::cout.flush();
                            waiting_for_name = true;
                            
                            std::string name = read_input_safely();
                            waiting_for_name = false;

                            if (!name.empty()) {
                                std::cout << "Starting recording in 3 seconds..." << std::endl;
                                std::this_thread::sleep_for(std::chrono::seconds(3));
                                recorder.start_recording(name);
                            } else {
                                std::cout << "Recording cancelled" << std::endl;
                            }
                        }
                        break;
                        
                    case 67: // F9
                        if (recorder.is_recording()) {
                            recorder.stop_recording();
                            std::cout << "\nRecording stopped" << std::endl;
                        }
                        break;
                        
                    case 68: // F10
                        {
                            std::cout << "\n=== PLAYBACK ===" << std::endl;
                            std::cout << "Enter macro name to play: ";
                            std::cout.flush();
                            std::string name = read_input_safely();
                            
                            if (!name.empty()) {
                                int loop_count = utils::get_loop_count_from_user();
                                recorder.play_macro(name, loop_count);
                            }
                        }
                        break;
                        
                    case 87: // F11
                        std::cout << "\n=== MACRO LIST ===" << std::endl;
                        recorder.list_macros();
                        break;
                        
                    case 1: // Esc
                        std::cout << "\nExiting..." << std::endl;
                        recorder.stop_recording();
                        break;
                }
            }
        }
    }

    close(kbd_fd);
}

int main() {
    std::cout << "Macro Recorder with Loop Support started" << std::endl;

    // Покажем доступные устройства
    utils::print_devices();
    
    // Спросим у пользователя какое устройство использовать
    std::cout << "\nEnter keyboard device (e.g., event2 or /dev/input/event2): ";
    std::string keyboard_input = read_input_safely();
    std::string keyboard_device = get_device_path(keyboard_input, "/dev/input/event2");

    std::cout << "Enter mouse device (e.g., event8 or /dev/input/event8): ";
    std::string mouse_input = read_input_safely();
    std::string mouse_device = get_device_path(mouse_input, "/dev/input/event8");

    std::cout << "Using keyboard: " << keyboard_device << std::endl;
    std::cout << "Using mouse: " << mouse_device << std::endl;

    std::string macros_dir = "/macroses";

    // Create macros directory
    fs::create_directories(macros_dir);

    MacroRecorder recorder(mouse_device, keyboard_device);
    recorder.set_macros_directory(macros_dir);
    
    monitor_keyboard(recorder, keyboard_device);

    std::cout << "Program terminated" << std::endl;
    return 0;
}