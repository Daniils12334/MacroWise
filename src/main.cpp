#include "MacroRecorder.hpp"
#include "Interface.hpp"
#include "utils.hpp"
#include <iostream>
#include <atomic>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <linux/input.h>

std::atomic<bool> recording(false);
std::atomic<bool> should_exit(false);
std::atomic<bool> interface_active(true);

void keyboard_monitor(MacroRecorder& recorder, const std::string& keyboard_device) {
    int kbd_fd = open(keyboard_device.c_str(), O_RDONLY | O_NONBLOCK);
    if (kbd_fd == -1) {
        perror("Error opening keyboard device");
        return;
    }

    while (!should_exit) {
        input_event ev;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(kbd_fd, &fds);

        timeval timeout{0, 100000};
        
        if (select(kbd_fd + 1, &fds, nullptr, nullptr, &timeout) > 0 && FD_ISSET(kbd_fd, &fds)) {
            if (read(kbd_fd, &ev, sizeof(ev)) == sizeof(ev) && ev.type == EV_KEY && ev.value == 1) {
                
                // Handle F9 to stop recording (even when interface is active)
                if (ev.code == 67) { // F9
                    if (recording) {
                        recorder.stop_recording();
                        recording = false;
                        std::cout << "\nRecording stopped via F9" << std::endl;
                    }
                }
                
                // Handle Esc to exit
                if (ev.code == 1) { // Esc
                    should_exit = true;
                    recorder.stop_recording();
                    break;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    close(kbd_fd);
}

int main() {
    // Initialize devices
    std::string mouse_device = "/dev/input/event8";
    std::string keyboard_device = "/dev/input/event2";
    std::string macros_dir = "/macroses";
    
    fs::create_directories(macros_dir);
    
    MacroRecorder recorder(mouse_device, keyboard_device);
    recorder.set_macros_directory(macros_dir);
    
    // Start keyboard monitoring thread
    std::thread keyboard_thread(keyboard_monitor, std::ref(recorder), keyboard_device);
    
    // Initialize interface
    Interface interface;
    
    // Set up callbacks
    interface.set_recording_callback([&](const std::string& name) {
        recording = true;
        std::thread([&, name]() {
            recorder.start_recording(name);
            recording = false;
        }).detach();
    });
    
    interface.set_playback_callback([&](const std::string& name, int loops) {
        std::thread([&, name, loops]() {
            recorder.play_macro(name, loops);
        }).detach();
    });
    
    interface.set_list_callback([&]() {
        return utils::list_macros(macros_dir);
    });
    
    interface.set_stop_recording_callback([&]() {
        if (recording) {
            recorder.stop_recording();
            recording = false;
        }
    });
    
    interface.set_recording_status_callback([&]() {
        return recording.load();
    });
    
    // Run the interface
    interface.run();
    
    // Cleanup
    should_exit = true;
    if (keyboard_thread.joinable()) {
        keyboard_thread.join();
    }
    
    std::cout << "Program terminated" << std::endl;
    return 0;
}