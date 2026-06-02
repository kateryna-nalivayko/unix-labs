#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int fd;

static void emit(int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof ev);
    ev.type = type;
    ev.code = (unsigned short)code;
    ev.value = value;
    if (write(fd, &ev, sizeof ev) < 0) { }
}

static void press(int code) {
    emit(EV_KEY, code, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    emit(EV_KEY, code, 0);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(80000);
}

int main(void) {
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open(/dev/uinput)");
        fprintf(stderr, "Підказка: sudo ./inject  або  sudo chmod 666 /dev/uinput\n");
        return 1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int c = 0; c < 256; ++c)
        ioctl(fd, UI_SET_KEYBIT, c);

    struct uinput_setup us;
    memset(&us, 0, sizeof us);
    us.id.bustype = BUS_USB;
    us.id.vendor  = 0x0001;
    us.id.product = 0x0001;
    strcpy(us.name, "lab5-virtual-kbd");
    ioctl(fd, UI_DEV_SETUP, &us);
    ioctl(fd, UI_DEV_CREATE);

    printf("Віртуальну клавіатуру 'lab5-virtual-kbd' створено.\n");
    printf("За 6 секунд почну набирати 'hello lab5'. Запусти keyhook на ній.\n");
    sleep(6);

    int seq[] = { KEY_H, KEY_E, KEY_L, KEY_L, KEY_O, KEY_SPACE,
                  KEY_L, KEY_A, KEY_B, KEY_5, KEY_ENTER };
    for (size_t i = 0; i < sizeof seq / sizeof *seq; ++i)
        press(seq[i]);

    printf("Готово. Видаляю віртуальну клавіатуру.\n");
    sleep(1);
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
