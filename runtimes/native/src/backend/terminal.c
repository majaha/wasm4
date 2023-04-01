#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "../terminal.h"
#include "../runtime.h"

bool TERMINAL = false;

const char* ENABLE_ALT_SCREEN = "\e[?1049h";
const char* DISABLE_ALT_SCREEN = "\e[?1049l";
const char* DISABLE_CURSOR = "\e[?25l";
const char* ENABLE_CURSOR = "\e[?25h";


struct termios original_termios;
void catch_int() {
    tcsetattr(fileno(stdin), TCSANOW, &original_termios);
    fputs(DISABLE_ALT_SCREEN, stdout);
    fputs(ENABLE_CURSOR, stdout);
    exit(0);
}

void w4_terminalBoot (const char* title) {

    tcgetattr(fileno(stdin), &original_termios);

    struct termios custom_termios;
    memcpy(&custom_termios, &original_termios, sizeof(struct termios));
    // cfmakeraw(&custom_termios);
    custom_termios.c_lflag &= ~(ECHO | ICANON);
    custom_termios.c_cc[VMIN] = 0;
    custom_termios.c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &custom_termios);

    fputs(ENABLE_ALT_SCREEN, stdout);
    fputs(DISABLE_CURSOR, stdout);

    signal(SIGINT, catch_int);

    for (;;) {
        uint8_t gamepad = 0;

        char buffer[16];
        int n = read(fileno(stdin), buffer, sizeof(buffer));

        for (int i=0; i<n; i++) {
            // Player 1
            if (buffer[i] == 'x') {
                gamepad |= W4_BUTTON_X;
            }
            if (buffer[i] == 'z') {
                gamepad |= W4_BUTTON_Z;
            }
            if (buffer[i] == 'j') {
                gamepad |= W4_BUTTON_LEFT;
            }
            if (buffer[i] == 'l') {
                gamepad |= W4_BUTTON_RIGHT;
            }
            if (buffer[i] == 'i') {
                gamepad |= W4_BUTTON_UP;
            }
            if (buffer[i] == 'k') {
                gamepad |= W4_BUTTON_DOWN;
            }
        }
        w4_runtimeSetGamepad(0, gamepad);

        w4_runtimeUpdate();
    }
}

char* w4_terminalSetForegroundColour(const uint32_t rgb, char* pointer){
    uint8_t red = (rgb >> 16) & 0xff;
    uint8_t green = (rgb >> 8) & 0xff;
    uint8_t blue = (rgb >> 0) & 0xff;
    pointer += sprintf(pointer, "\e[38;2;%hhu;%hhu;%hhum", red, green, blue);
    return pointer;
}

char* w4_terminalSetBackgroundColour(const uint32_t rgb, char* pointer){
    uint8_t red = (rgb >> 16) & 0xff;
    uint8_t green = (rgb >> 8) & 0xff;
    uint8_t blue = (rgb >> 0) & 0xff;
    pointer += sprintf(pointer, "\e[48;2;%hhu;%hhu;%hhum", red, green, blue);
    return pointer;
}

void w4_terminalComposite (const uint32_t* palette, const uint8_t* framebuffer) {
    char buffer[((13+4)*2*160+1)+10000];
    char* pointer = buffer;


    pointer += sprintf(pointer, "\e[H"); // put cursor to top left

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    int last_top_colour = -1;
    int last_bottom_colour = -1;

    // Convert indexed 2bpp framebuffer to XRGB output
    for (int y=0; y < 160; y+=2) {
        for (int x=0; x < 160; x+=4) {
            int n = (y*160 + x) / 4;

            uint8_t top_quartet = framebuffer[n];
            uint8_t bottom_quartet = framebuffer[n+160/4];
            for (int i=0; i<4; i++) {
                int top_colour = (top_quartet >> (i * 2)) & 0b11;
                int bottom_colour = (bottom_quartet >> (i * 2)) & 0b11;

                if (top_colour != last_top_colour) {
                    pointer = w4_terminalSetBackgroundColour(palette[top_colour], pointer);
                    last_top_colour = top_colour;
                }
                
                if (bottom_colour != last_bottom_colour) {
                    pointer = w4_terminalSetForegroundColour(palette[bottom_colour], pointer);
                    last_bottom_colour = bottom_colour;
                }
                
                pointer += sprintf(pointer, "▄");
            }
        }
        puts(buffer);
        pointer = buffer;
    }

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec = start.tv_sec;
    double milliseconds = seconds/1000 + nanoseconds*1e-6;
    // printf("\e[0m%.5fms\n", milliseconds);
}