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
#include "StackDump.h"

#define TAG "librev-dj"

void *g_load_base = 0;

static JNI_OnLoad_Type old_jni_onload = 0;

static JNI_OnLoad_Type old_jni_onload_avmp = 0;


typedef jobject (*doCommandNative_type)(JNIEnv *env, jclass clz, jint cmd, jobjectArray array);
doCommandNative_type old_doCommandNative = 0;

void print_object_array(JNIEnv *env, jobjectArray array) {
    env->PushLocalFrame(20);
    jclass cls = env->FindClass("java/lang/Object");
    jmethodID m = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    jmethodID m2 = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");

    int n = env->GetArrayLength(array);
    for (int i = 0; i < n; i++) {
        jobject obj = env->GetObjectArrayElement(array, i);
        if (obj != 0) {
            env->PushLocalFrame(20);
            jstring s = (jstring) env->CallObjectMethod(obj, m);
            const char *str = env->GetStringUTFChars(s, 0);
            jobject c = env->CallObjectMethod(obj, m2);
            jstring sClassName = (jstring)env->CallObjectMethod(c, m);
            const char *strClassName = env->GetStringUTFChars(sClassName, 0);
            __android_log_print(ANDROID_LOG_INFO, "librev-dj", "param%d %s [%s]", i, str, strClassName);
            env->PopLocalFrame(0);
        }
        else {
            __android_log_print(ANDROID_LOG_INFO, "librev-dj", "param%d is null", i);
        }
    }
    env->PopLocalFrame(0);
}


static void force_call_cmd_10401(JNIEnv *env, jclass cls) {
    env->PushLocalFrame(20);
    jclass mapClass = env->FindClass("java/util/HashMap");
    jmethodID init = env->GetMethodID(mapClass, "<init>", "()V");
    jobject hashMap = env->NewObject(mapClass, init);

    jmethodID put = env->GetMethodID(mapClass, "put",
                                     "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jstring str = env->NewStringUTF("INPUT");
    const char *v = "XtX3M1bJ69cDAFWqkBwQYXgY&&&21465214&94de0d14487a78f08caa8b9366df870e&1591240459&mto.alimusic.search.searchservice.searchsongs&1.3&&701287@xiami_android_8.3.8&AohsPSPH-F7lQLJzyIvh_6geqxEqIetYwOxZ0laI9k_9&&&27";

    jstring jv = env->NewStringUTF(v);

    env->CallObjectMethod(hashMap, put, str, jv);

    jstring appId = env->NewStringUTF("21465214");

    jclass intClass = env->FindClass("java/lang/Integer");
    jmethodID initInt = env->GetMethodID(intClass, "<init>", "(I)V");
    jobject intval = env->NewObject(intClass, initInt, 7);

    __android_log_print(ANDROID_LOG_INFO, TAG, "call 1");

    jclass boolClass = env->FindClass("java/lang/Boolean");
    jmethodID initBool = env->GetMethodID(boolClass, "<init>", "(Z)V");
    jobject boolval = env->NewObject(boolClass, initBool, true);

    __android_log_print(ANDROID_LOG_INFO, TAG, "call 2");
    jclass objClass = env->FindClass("java/lang/Object");
    jobjectArray objarr = env->NewObjectArray(5, objClass, NULL);
    env->SetObjectArrayElement(objarr, 0, hashMap);
    env->SetObjectArrayElement(objarr, 1, appId);
    env->SetObjectArrayElement(objarr, 2, intval);
    env->SetObjectArrayElement(objarr, 3, NULL);
    env->SetObjectArrayElement(objarr, 4, boolval);

    __android_log_print(ANDROID_LOG_INFO, TAG, "call 3");
    print_object_array(env, objarr);

    jstring jr = (jstring)old_doCommandNative(env, cls, 10401, objarr);

    const char * crs = env->GetStringUTFChars(jr, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call cmd 10401 return %s", crs);
    env->ReleaseStringUTFChars(jr, crs);
    env->PopLocalFrame(0);
}

jobject my_doCommandNative(JNIEnv *env, jclass clz, jint cmd, jobjectArray array) {

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call my_doCommandNative %d", cmd);
    print_object_array(env, array);

    jobject r = old_doCommandNative(env, clz, cmd, array);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call my_doCommandNative return %p", r);
    if (cmd == 10401) {
        jstring rs = (jstring)r;
        const char * crs = env->GetStringUTFChars(rs, 0);
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cmd 10401 return %s", crs);
        env->ReleaseStringUTFChars(rs, crs);
    }
    else if (cmd == 60902) {
        //avmp对签名再签名返回值
        jbyteArray jb = (jbyteArray)r;
        jbyte *b = env->GetByteArrayElements(jb, 0);
        int nb = env->GetArrayLength(jb);
        char *blob = (char*)malloc(nb+1);
        memset(blob, 0, nb+1);
        memcpy(blob, b, nb);
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cmd 60902 return %s", blob);
        free(blob);
        env->ReleaseByteArrayElements(jb, b, 0);
    }
    /*
    if (cmd == 10101) {
        force_call_cmd_10401(env, clz);
    }
     */
    return r;
}

jint my_jni_on_load(JavaVM *vm, void *reserve) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "my_jni_onload call");
    MapInfo info = {0};
    get_map_infos(&info, "libsgmainso-6.4.163.so");

    __android_log_print(ANDROID_LOG_INFO, TAG, "libsgmainso-6.4.163.so map base %p", info.baseAddr);
    g_load_base = info.baseAddr;
    void *doCmd = (void*)((unsigned)g_load_base + 0x0000FDF0+1);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook docmd");
    MSHookFunction((void*)doCmd, (void*)my_doCommandNative, (void**)&old_doCommandNative);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook docmd %p", old_doCommandNative);


    JNIEnv *env = 0;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    //hook_jni(env);
    int r = old_jni_onload(vm, reserve);
    __android_log_print(ANDROID_LOG_INFO, TAG, "my_jni_onload return");

    return r;
}


jint my_jni_on_load_avmp(JavaVM *vm, void *reverve) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "my_jni_onload_avmp call");
    DUMP_CALL_STACK("librev")
    int r = old_jni_onload_avmp(vm, reverve);
    __android_log_print(ANDROID_LOG_INFO, TAG, "my_jni_onload_avmp return");
    return r;
}


const char *pkgName = "fm.xiami.main";

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}


__attribute__((constructor)) void __init_nm() {

    if (!is_package_name(pkgName, true)) {
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "xiami call pid %d!!!", getpid());

    char sec_path[255];
    sprintf(sec_path, "/data/data/%s/lib/libsgmainso-6.4.163.so", pkgName);
    void *lib = dlopen(sec_path, RTLD_NOW);
    void *ori_load = dlsym(lib, "JNI_OnLoad");

    __android_log_print(ANDROID_LOG_INFO, TAG, "dlopen sgmain return %p", lib);

    //old_jni_onload = hook_jni_onload((JNI_OnLoad_Type)ori_load, my_jni_on_load);

    char avmp_path[255];
    sprintf(avmp_path, "/data/data/%s/lib/libsgavmpso-6.4.35.so", pkgName);
    void *libavmp = dlopen(avmp_path, RTLD_NOW);
    void *ori_load_avmp = dlsym(libavmp, "JNI_OnLoad");

    __android_log_print(ANDROID_LOG_INFO, TAG, "dlopen avmp return %p", libavmp);

    MapInfo info = {0};
    get_map_infos(&info, "libsgavmpso-6.4.35.so");

    __android_log_print(ANDROID_LOG_INFO, TAG, "libsgavmpso-6.4.35.so map base %p", info.baseAddr);
    old_jni_onload_avmp = hook_jni_onload((JNI_OnLoad_Type)ori_load_avmp, my_jni_on_load_avmp);

    /*

    wait_for_attach();
     */
}

