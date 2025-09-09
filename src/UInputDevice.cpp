#include "UInputDevice.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <thread>
#include <chrono>

UInputDevice::UInputDevice() : fd(-1), initialized(false) {}

UInputDevice::~UInputDevice() {
    destroy();
}

bool UInputDevice::initialize() {
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Error opening uinput device");
        return false;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < KEY_MAX; i++) {
        ioctl(fd, UI_SET_KEYBIT, i);
    }

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "virtual-macro-device");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    initialized = true;
    return true;
}

void UInputDevice::emit_event(const input_event& ev) {
    if (initialized) {
        write(fd, &ev, sizeof(ev));
    }
}

void UInputDevice::destroy() {
    if (initialized) {
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
        initialized = false;
    }
}