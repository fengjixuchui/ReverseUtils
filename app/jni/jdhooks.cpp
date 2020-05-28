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

#define TAG "librev-jd"
static void *g_jd_base_addr = 0;
static JavaVM *s_vm = 0;
#if false
void print_java_call_stack(JNIEnv *env) {
    env->PushLocalFrame(20);
    /*
    jclass cls_exp = env->FindClass("java/lang/Exception");
    jmethodID ctor = env->GetMethodID(cls_exp, "<init>", "()V");
    jobject exp = env->NewObject(cls_exp, ctor);

    jclass cls_log = env->FindClass("android/util/Log");
    jmethodID logm = env->GetStaticMethodID(cls_log, "getStackTraceString", "(Ljava/lang/Throwable;)Ljava/lang/String;");
    jstring st = (jstring )env->CallStaticObjectMethod(cls_log, logm, exp);
    const char *s = env->GetStringUTFChars(st, 0);
    __android_log_print(ANDROID_LOG_INFO, TAG, "call stack %s", s);
    jmethodID printStackTrace = env->GetMethodID(cls_exp, "printStackTrace", "()V");
    env->CallVoidMethod(exp, printStackTrace);
     */
    jclass cls_thread = env->FindClass("java/lang/Thread");
    jmethodID curthread_m = env->GetStaticMethodID(cls_thread, "currentThread", "()Ljava/lang/Thread;");

    jmethodID mid_getStackTrace =
            env->GetMethodID(cls_thread,
                              "getStackTrace",
                              "()[Ljava/lang/StackTraceElement;");

    jclass frame_class = env->FindClass("java/lang/StackTraceElement");
    jmethodID mid_frame_toString =
            env->GetMethodID(frame_class,
                              "toString",
                              "()Ljava/lang/String;");

    jobject t = env->CallStaticObjectMethod(cls_thread, curthread_m);
    jobjectArray traces = (jobjectArray)env->CallObjectMethod(t, mid_getStackTrace);
    jsize n = env->GetArrayLength(traces);
    __android_log_print(ANDROID_LOG_INFO, TAG, "call stack:");
    for (int i = 0; i < n; i++) {
        env->PushLocalFrame(10);
        jobject trace = env->GetObjectArrayElement(traces, i);
        jstring s = (jstring )env->CallObjectMethod(trace, mid_frame_toString);
        const char *cs = env->GetStringUTFChars(s, 0);
        __android_log_print(ANDROID_LOG_INFO, TAG, "%s", cs);
        env->ReleaseStringUTFChars(s, cs);
        env->PopLocalFrame(0);
    }

    env->PopLocalFrame(0);

}

typedef int (*connect_type)(int, sockaddr *, socklen_t);
connect_type old_connect;
int my_connect(int socket, sockaddr *addr, socklen_t socklen) {
    sockaddr_in *inaddr = (sockaddr_in*)addr;
    if(inaddr->sin_family == AF_INET) {
        unsigned port = ntohs(inaddr->sin_port);
        const char *ip = inet_ntoa(inaddr->sin_addr);
        __android_log_print(ANDROID_LOG_INFO, TAG, "hook connect socket:%d ip:%s port=%u", socket, ip, port);
        if (s_vm) {
            JNIEnv *env = 0;
            s_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                print_java_call_stack(env);
            }
            else {
                __android_log_print(ANDROID_LOG_INFO, TAG, "env is null");
            }
        }
    }
    return old_connect(socket, addr, socklen);
}


static JNI_OnLoad_Type old_jni_onload_jd = 0;
jint my_jni_on_load(JavaVM *vm) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "jd jni_onload call");
    s_vm = vm;
    JNIEnv *env = 0;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    //hook_jni(env);
    int r = old_jni_onload_jd(vm);
    __android_log_print(ANDROID_LOG_INFO, TAG, "jd jni_onload return");
    return r;
}


__attribute__((constructor)) void __init_jd() {

    const char *pkgName = "com.jingdong.app.mall";
    //__android_log_print(ANDROID_LOG_INFO, "librev-dj", "call");
    if (!is_package_name(pkgName, true)) {
        return;
    }



    __android_log_print(ANDROID_LOG_INFO, TAG, "jd call pid %d!!!", getpid());
    char sec_path[255];
    sprintf(sec_path, "/data/data/%s/lib/libJDMobileSec.so", pkgName);
    void *lib = dlopen(sec_path, RTLD_NOW);
    __android_log_print(ANDROID_LOG_INFO, TAG, "jd sec %p", lib);

    MapInfo mapInfo;
    get_map_infos(&mapInfo, "libJDMobileSec.so");

    g_jd_base_addr = mapInfo.baseAddr;
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "jd load base %p", g_jd_base_addr);
    void *ori_load = dlsym(lib, "JNI_OnLoad");

    old_jni_onload_jd = hook_jni_onload((JNI_OnLoad_Type)ori_load, my_jni_on_load);


    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook connect");
    MSHookFunction((void*)connect, (void*)my_connect, (void**)&old_connect);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook connect %p", old_connect);

}
#endif

