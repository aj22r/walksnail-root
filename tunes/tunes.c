#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>

#define TIMER_FREQ 250000000LL // just a guess

struct note {
    int freq;
    float duration;
};

static const struct note notes[] = {
#include "badapple.h"
};

int pwm_fd = -1;
void *pwm_mem = NULL;

void pwm_set(uint32_t duty_cycle, uint32_t period, int enable) {
    *(uint32_t*)(pwm_mem + 0x8C) = period;
    *(uint32_t*)(pwm_mem + 0xCC) = duty_cycle;
    *(uint32_t*)(pwm_mem + 0x94) = enable ? 0x0B : 0x00;
}

uint32_t freq_to_period(int freq) {
    return TIMER_FREQ / freq;
}

void signal_handler(int) {
    if(pwm_mem)
        pwm_set(0, 0, 0);
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    pwm_fd = open("/dev/mem", O_RDWR);
    if(pwm_fd < 0) {
        perror("Failed to open /dev/mem");
        exit(1);
    }

    pwm_mem = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, pwm_fd, 0x40000000);
    if(pwm_mem == NULL) {
        perror("Failed to mmap /dev/mem");
        close(pwm_fd);
        exit(1);
    }

    struct timeval start, end;

    int i = 0;
    while(i < sizeof(notes)/sizeof(*notes)) {
        gettimeofday(&start, NULL);

        printf("note %d duration %.1f\n", notes[i].freq, notes[i].duration);
        fflush(stdout);

        int duration = (int)(notes[i].duration * 1000.f);
        if(notes[i].freq > 0) {
            uint32_t period = freq_to_period(notes[i].freq);
            pwm_set(period / 2, period, 1);
        } else {
            pwm_set(0, 0, 0);
        }

        gettimeofday(&end, NULL);
        long elapsed_microsecs = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        if(elapsed_microsecs < duration)
            usleep(duration - elapsed_microsecs);
        
        i++;
    }
    pwm_set(0, 0, 0);
    printf("done\n");

    close(pwm_fd);
    return 0;
}
