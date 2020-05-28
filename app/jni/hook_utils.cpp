//
// Created by my on 20-4-27.
//

#include "hook_utils.h"
#include <android/log.h>
#include <Substrate/SubstrateHook.h>
#include <cstdio>
#include <string.h>
#include <unistd.h>

JNI_OnLoad_Type hook_jni_onload(JNI_OnLoad_Type ori_load, JNI_OnLoad_Type new_jni_onload) {
    JNI_OnLoad_Type old_jni_onload = 0;
    MSHookFunction((void*)ori_load, (void*)new_jni_onload, (void**)&old_jni_onload);
    return old_jni_onload;
}

bool is_package_name(const char *pkgName, bool useLike) {
    const char *path = "/proc/self/cmdline";
    char buf[300] = {0};
    FILE *f = fopen(path, "rb");
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (!useLike) {
        if (strcmp(buf, pkgName) != 0) {
            return false;
        }
        return true;
    }
    else {
        if (strstr(buf, pkgName) != 0) {
            return true;
        }
        return false;
    }
}

static int get_tracer(pid_t pid) {
    char line[255] = {0};

    char name[255] = {0};
    sprintf(name, "/proc/self/task/%d/status", pid);
    FILE *f = fopen(name, "r");

    int iid = 0;
    while(!feof(f)) {
        fgets(line, sizeof(line), f);
        if (strstr(line, "TracerPid")) {
            const char *sp = strchr(line, ':');
            const char *id = sp + 1;
            iid = atoi(id);
            break;
        }
    }
    fclose(f);
    return iid;

}

void wait_for_attach() {
    pid_t pid = gettid();
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "waiting attach %d...", pid);
    while (1){
        int id = get_tracer(pid);
        if (id != 0) {
            __android_log_print(ANDROID_LOG_INFO, "librev-dj", "debugger attached trace_id:%d...", id);
            break;
        }
        sleep(1);
    }

}



