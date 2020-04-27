//
// Created by my on 20-4-27.
//

#include <android/log.h>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>
#include "hook_utils.h"
#include "ElfUtils.h"

#define TAG "librev-jd"

static void *g_jd_base_addr = 0;

static JNI_OnLoad_Type old_jni_onload_jd = 0;
jint my_jni_on_load(JavaVM *vm) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "jd jni_onload call");
    return old_jni_onload_jd(vm);
}

__attribute__((constructor)) void __init_jd() {

    const char *pkgName = "com.jingdong.app.mall";
    //__android_log_print(ANDROID_LOG_INFO, "librev-dj", "call");
    if (!is_package_name(pkgName)) {
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "jd call pid %d!!!", getpid());
    char sec_path[255];
    sprintf(sec_path, "/data/data/%s/lib/libJDMobileSec.so", pkgName);
    void *lib = dlopen(sec_path, RTLD_NOW);
    __android_log_print(ANDROID_LOG_INFO, TAG, "jd sec %p", lib);

    MapInfo mapInfo;
    get_map_infos(&mapInfo, "libcms.so");

    g_jd_base_addr = mapInfo.baseAddr;
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "jd load base %p", g_jd_base_addr);
    void *ori_load = dlsym(lib, "JNI_OnLoad");

    old_jni_onload_jd = hook_jni_onload((JNI_OnLoad_Type)ori_load, my_jni_on_load);


    /*
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook dlopen");
    MSHookFunction((void*)dlopen, (void*)my_dlopen, (void**)&old_dlopen);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook dlopen %p", old_dlopen);
     */


}

