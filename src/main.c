#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include "vmi.h"
#include "spoof.h"
#include "mem.h"

static vmi_instance_t vmi;
static volatile sig_atomic_t running = 1;
static FILE *log_fp = NULL;

void log_message(int level, const char *fmt, ...) {
    if (!log_fp) return;
    
    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';

    va_list args;
    va_start(args, fmt);
    fprintf(log_fp, "[%s] ", timestamp);
    vfprintf(log_fp, fmt, args);
    fprintf(log_fp, "\n");
    fflush(log_fp);
    va_end(args);
}

void signal_handler(int signum) {
    log_message(1, "Received signal %d, shutting down", signum);
    running = 0;
}

void cleanup(void) {
    if (log_fp) {
        fclose(log_fp);
    }
}

spoof_config_t* load_config(const char *config_path) {
    spoof_config_t *config = malloc(sizeof(spoof_config_t));
    if (!config) return NULL;

    json_object *root, *j_target_offset, *j_randomize, *j_interval, *j_debug_level, *j_log_file;
    root = json_object_from_file(config_path);
    if (!root) {
        free(config);
        return NULL;
    }

    json_object_object_get_ex(root, "target_offset", &j_target_offset);
    json_object_object_get_ex(root, "randomize", &j_randomize);
    json_object_object_get_ex(root, "interval", &j_interval);
    json_object_object_get_ex(root, "debug_level", &j_debug_level);
    json_object_object_get_ex(root, "log_file", &j_log_file);

    config->target_offset = json_object_get_int64(j_target_offset);
    config->randomize = json_object_get_boolean(j_randomize);
    config->interval = json_object_get_int(j_interval);
    config->debug_level = json_object_get_int(j_debug_level);
    strncpy(config->log_file, json_object_get_string(j_log_file), sizeof(config->log_file) - 1);

    json_object_put(root);
    return config;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <vm_name> <config_path>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    atexit(cleanup);

    spoof_config_t *config = load_config(argv[2]);
    if (!config) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }

    log_fp = fopen(config->log_file, "a");
    if (!log_fp) {
        fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
        free(config);
        return 1;
    }

    if (!init_vmi(&vmi, argv[1])) {
        log_message(0, "Failed to init VMI: %s", vmi_get_last_error());
        free(config);
        return 1;
    }

    if (!init_spoofer(vmi, config)) {
        log_message(0, "Failed to init spoofer");
        cleanup_vmi(&vmi);
        free(config);
        return 1;
    }

    log_message(1, "KASLR Offset Spoofer initialized successfully");
    log_message(1, "Original KASLR Offset: 0x%lx", get_kaslr_offset(vmi));

    while (running) {
        if (config->randomize && config->interval > 0) {
            addr_t new_offset = generate_random_offset();
            if (!spoof_kaslr_offset(vmi, new_offset)) {
                log_message(0, "Failed to spoof KASLR offset");
                break;
            }
            log_message(2, "New KASLR Offset: 0x%lx", get_current_spoofed_offset());
            usleep(config->interval * 1000);
        } else {
            if (!spoof_kaslr_offset(vmi, config->target_offset)) {
                log_message(0, "Failed to spoof KASLR offset");
                break;
            }
            log_message(1, "KASLR Offset set to target: 0x%lx", config->target_offset);
            usleep(1000000);
        }

        if (config->debug_level > 0) {
            log_message(3, "Current KASLR Offset: 0x%lx", get_current_spoofed_offset());
        }
    }

    restore_original_offset(vmi);
    cleanup_vmi(&vmi);
    free(config);

    log_message(1, "KASLR Offset Spoofer terminated");
    return 0;
}
