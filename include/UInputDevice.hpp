#pragma once

#include <linux/input.h>
#include <linux/uinput.h>

class UInputDevice {
public:
    UInputDevice();
    ~UInputDevice();
    
    bool initialize();
    void emit_event(const input_event& ev);
    void destroy();
    
    bool is_initialized() const { return initialized; }
    
private:
    int fd;
    bool initialized;
};