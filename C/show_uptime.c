#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

int main() {
    struct sysinfo info;

    while (1) {
        if (sysinfo(&info) != 0) {
            perror("sysinfo error");
            return EXIT_FAILURE;
        }

        long total_seconds = info.uptime;

        // Breakdown math for years down to seconds
        long years   = total_seconds / 31536000;
        long weeks   = (total_seconds % 31536000) / 604800;
        long days    = (total_seconds % 604800) / 86400;
        long hours   = (total_seconds % 86400) / 3600;
        long minutes = (total_seconds % 3600) / 60;
        long seconds = total_seconds % 60;

        // Dynamically format output string based on how long system has been up
        if (years > 0) {
            // E.g., "Up: 1y 12w 3d"
            printf("Up: %ldy %ldw %ldd\n", years, weeks, days);
        } else if (weeks > 0) {
            // E.g., "Up: 2w 4d 12h"
            printf("Up: %ldw %ldd %ldh\n", weeks, days, hours);
        } else if (days > 0) {
            // E.g., "Up: 5d 8h 23m"
            printf("Up: %ldd %ldh %ldm\n", days, hours, minutes);
        } else if (hours > 0) {
            // E.g., "Up: 4h 12m 5s"
            printf("Up: %ldh %ldm \n", hours, minutes);
        } else {
            // E.g., "Up: 2m 45s"
            printf("Up: %ldm \n", minutes);
        }

        fflush(stdout);

        // Sleep for 5 seconds to keep CPU usage at virtually zero
        sleep(5);
    }

    return EXIT_SUCCESS;
}
