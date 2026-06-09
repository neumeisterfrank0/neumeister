#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Structure to hold CPU ticks
typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} CPUSnapshot;

// Function to read the current CPU ticks from /proc/stat
int get_cpu_snapshot(CPUSnapshot *snapshot) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        return 0;
    }

    char buffer[256];
    // Read the first line which contains the aggregate "cpu" metrics
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Parse the fields out of the line
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &snapshot->user, &snapshot->nice, &snapshot->system, &snapshot->idle,
               &snapshot->iowait, &snapshot->irq, &snapshot->softirq, &snapshot->steal);
    }

    fclose(fp);
    return 1;
}

// Function to calculate the CPU load percentage based on two snapshots
double calculate_cpu_load(CPUSnapshot *prev, CPUSnapshot *curr) {
    // Total idle time includes normal idle and time waiting for I/O
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long curr_idle = curr->idle + curr->iowait;

    // Total non-idle time (active CPU time)
    unsigned long long prev_active = prev->user + prev->nice + prev->system + prev->irq + prev->softirq + prev->steal;
    unsigned long long curr_active = curr->user + curr->nice + curr->system + curr->irq + curr->softirq + curr->steal;

    unsigned long long prev_total = prev_idle + prev_active;
    unsigned long long curr_total = curr_idle + curr_active;

    // Differentiate the chunks
    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;

    if (total_diff == 0) return 0.0;

    // CPU Usage % = ((Total_diff - Idle_diff) / Total_diff) * 100
    return ((double)(total_diff - idle_diff) / total_diff) * 100.0;
}

int main() {
    CPUSnapshot prev, curr;

    printf("Monitoring CPU Load (Press Ctrl+C to exit)...\n");

    // Get initial baseline snapshot
    if (!get_cpu_snapshot(&prev)) {
        return EXIT_FAILURE;
    }

    while (1) {
        // Sleep for 2 seconds

        // Get the new snapshot
        if (!get_cpu_snapshot(&curr)) {
            return EXIT_FAILURE;
        }

        // Calculate and print the load
        double load = calculate_cpu_load(&prev, &curr);
        printf("C.P.U. Load: %.2f%%\n", load);
        fflush(stdout);

        // Move current snapshot to previous for the next iteration
        prev = curr;
        sleep(1);
    }

    return EXIT_SUCCESS;
}
