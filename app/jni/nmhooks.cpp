//
// Created by my on 20-4-27.
//

#include <android/log.h>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <Substrate/SubstrateHook.h>
#include "hook_utils.h"
#include "ElfUtils.h"

#if 0
#define TAG "librev-dj"
void *g_load_base = 0;
typedef jstring (*serialdata_type)(JNIEnv *env, jclass clz, jstring url, jstring str);
serialdata_type old_serialdata = 0;
jstring my_serialdata(JNIEnv *env, jclass clz, jstring url, jstring str) {

    const char *curl = env->GetStringUTFChars(url, 0);
    const char *cstr = env->GetStringUTFChars(str, 0);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call serialdata %s %s", curl, cstr);
    jstring r = old_serialdata(env, clz, url, str);
    const char *cr = env->GetStringUTFChars(r, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call serialdata return %s", cr);
    return r;
}

typedef void (*nativeInit_type)(JNIEnv *env, jclass clz, jobject ctx);
nativeInit_type old_nativeInit = 0;
void my_nativeInit(JNIEnv *env, jclass clz, jobject ctx) {

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call nativeInit");
    old_nativeInit(env, clz, ctx);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call nativeInit return ");
}


static void force_call_serial(JNIEnv *env) {
    env->PushLocalFrame(20);
    jclass cls = env->FindClass("com/netease/cloudmusic/utils/NeteaseMusicUtils");

    jstring jurl = env->NewStringUTF("/api/sp/flow/status/v2");
    const char *s = "{\"deviceid\":\"MzUzNjI3MDc2NTI2MjQ2CTAwOmE3OjEwOjkzOjY0OjU3CThmOTcyMzdlMjIzYTQxMjkJOTllYTY0YTY5NGZjZjM4ZA%3D%3D_android\",\"header\":{\"URS_APPID\":\"9A655D8373C42875421957AA8A68FF2587AD2149DBBCE40400A376B554866AA1888BBDD2609A548EFA6B1B3C7AFAE500\",\"appver\":\"7.1.51\",\"buildver\":\"1589354697\",\"channel\":\"tencent\",\"deviceId\":\"MzUzNjI3MDc2NTI2MjQ2CTAwOmE3OjEwOjkzOjY0OjU3CThmOTcyMzdlMjIzYTQxMjkJOTllYTY0YTY5NGZjZjM4ZA%3D%3D\",\"mobilename\":\"Nexus5X\",\"ntes_kaola_ad\":\"1\",\"os\":\"android\",\"osver\":\"6.0.1\",\"requestId\":\"1590242909868_120\",\"resolution\":\"1794x1080\",\"versioncode\":\"7001051\"},\"e_r\":\"true\"}";
    jstring serial = env->NewStringUTF(s);

    wait_for_attach();
    jstring jr = old_serialdata(env, cls, jurl, serial);

    const char *r = env->GetStringUTFChars(jr, 0);

    __android_log_print(ANDROID_LOG_INFO, TAG, "force call serial return %s", r);
    env->PopLocalFrame(0);
}

static JNI_OnLoad_Type old_jni_onload = 0;
jint my_jni_on_load(JavaVM *vm, void *reserve) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "my_jni_onload call");
    MapInfo info = {0};
    get_map_infos(&info, "libpoison.so");

    __android_log_print(ANDROID_LOG_INFO, TAG, "libpoison map base %p", info.baseAddr);
    g_load_base = info.baseAddr;
    //hook_jni(env);
    int r = old_jni_onload(vm, reserve);


    void *serial = (void*)((unsigned)info.baseAddr + 0x00043CD0 + 1);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook serialdata kkk");
    MSHookFunction((void*)serial, (void*)my_serialdata, (void**)&old_serialdata);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook serialdata %p", old_serialdata);

    __android_log_print(ANDROID_LOG_INFO, TAG, "before hook native init");
    nativeInit_type nativeInit = (nativeInit_type)((unsigned)g_load_base + 0x000435A4 + 1);
    MSHookFunction((void*)nativeInit, (void*)my_nativeInit , (void**)&old_nativeInit);
    __android_log_print(ANDROID_LOG_INFO, TAG, "after hook native init");

    JNIEnv *env = 0;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    force_call_serial(env);

    __android_log_print(ANDROID_LOG_INFO, TAG, "my_jni_onload return");
    return r;
}


const char *pkgName = "com.netease.cloudmusic";

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}


__attribute__((constructor)) void __init_nm() {

    if (!is_package_name(pkgName, true)) {
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "netease music call pid %d!!!", getpid());

    char sec_path[255];
    sprintf(sec_path, "/data/data/%s/lib/libpoison.so", pkgName);
    void *lib = dlopen(sec_path, RTLD_NOW);
    void *ori_load = dlsym(lib, "JNI_OnLoad");

    __android_log_print(ANDROID_LOG_INFO, TAG, "dlopen return %p", lib);


    old_jni_onload = hook_jni_onload((JNI_OnLoad_Type)ori_load, my_jni_on_load);
    /*

    wait_for_attach();
     */
}
#endif

