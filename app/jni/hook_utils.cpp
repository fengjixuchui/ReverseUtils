//
// Created by my on 20-4-27.
//

#include "hook_utils.h"
#include <android/log.h>
#include <Substrate/SubstrateHook.h>
#include <cstdio>
#include <string.h>

JNI_OnLoad_Type hook_jni_onload(JNI_OnLoad_Type ori_load, JNI_OnLoad_Type new_jni_onload) {
    JNI_OnLoad_Type old_jni_onload = 0;
    MSHookFunction((void*)ori_load, (void*)new_jni_onload, (void**)&old_jni_onload);
    return old_jni_onload;
}

bool is_package_name(const char *pkgName) {
    const char *path = "/proc/self/cmdline";
    char buf[300] = {0};
    FILE *f = fopen(path, "rb");
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (strcmp(buf, pkgName)!=0) {
        return false;
    }
    return true;
}



