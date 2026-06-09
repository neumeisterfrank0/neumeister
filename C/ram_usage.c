#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void get_exact_ram(unsigned long *total, unsigned long *used, double *percent) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo");
        return;
    }

    char label[32];
    unsigned long value;
    
    unsigned long mem_total = 0;
    unsigned long mem_free = 0;
    unsigned long buffers = 0;
    unsigned long cached = 0;
    unsigned long sreclaimable = 0; // Kernel slab cache that can be freed

    // Parse lines like: "MemTotal:       16343532 kB"
    while (fscanf(fp, "%s %lu kB", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) {
            mem_total = value;
        } else if (strcmp(label, "MemFree:") == 0) {
            mem_free = value;
        } else if (strcmp(label, "Buffers:") == 0) {
            buffers = value;
        } else if (strcmp(label, "Cached:") == 0) {
            cached = value;
        } else if (strcmp(label, "SReclaimable:") == 0) {
            sreclaimable = value;
        }
    }
    fclose(fp);

    // This is the exact calculation the modern 'free' command uses:
    // Available/Actual Free = MemFree + Buffers + Cached + SReclaimable
    unsigned long actual_free_kb = mem_free + buffers + cached + sreclaimable;
    unsigned long used_kb = mem_total - actual_free_kb;

    // Convert Kilobytes to Megabytes
    *total = mem_total / 1024;
    *used = used_kb / 1024;
    
    if (*total > 0) {
        *percent = ((double)*used / *total) * 100;
    } else {
        *percent = 0.0;
    }
}

int main() {
    unsigned long total_mb = 0;
    unsigned long used_mb = 0;
    double percent_used = 0.0;

    while (1) {
        get_exact_ram(&total_mb, &used_mb, &percent_used);

        // \r rewrites the line seamlessly 
        printf("RAM Used: %lu MB / %lu MB (%.2f%%)\n", used_mb, total_mb, percent_used);
        fflush(stdout);

        sleep(1);
    }

    return 0;
}
