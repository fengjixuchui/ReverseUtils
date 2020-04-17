#include "Substrate/SubstrateHook.h"

#include <jni.h>
#include <string>
#include <vector>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <android/log.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/prctl.h>
#include "StackDump.h"
#include "ElfUtils.h"


typedef jclass (*find_class_type)(JNIEnv *, const char *);
find_class_type old_find_class = 0;
jclass my_find_class(JNIEnv *env, const char *name) {
    jclass r = old_find_class(env, name);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "FindClass %s tid %d return %p", name, gettid(), r);
    return r;
}

typedef jmethodID (*get_method_id_type)(JNIEnv *env, jclass clz, const char *name, const char *sig);
get_method_id_type old_get_method_id = 0;
jmethodID my_get_method_id(JNIEnv *env, jclass clz, const char *name, const char *sig) {

    char tname[255] = "";
    prctl(PR_GET_NAME, tname, 0, 0);
    jmethodID r = old_get_method_id(env, clz, name, sig);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "GetMethodId %p %s %s tid %d %s return %p", clz, name, sig, gettid(), tname, r);
    return r;
}

get_method_id_type old_get_static_method_id = 0;
jmethodID my_get_static_method_id(JNIEnv *env, jclass clz, const char *name, const char *sig) {
    char tname[255] = "";
    prctl(PR_GET_NAME, tname, 0, 0);
    jmethodID r = old_get_static_method_id(env, clz, name, sig);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "GetStaticMethodId %p %s %s tid %d %s return %p", clz, name, sig, gettid(), tname, r);
    return r;
}

typedef jobject (*call_object_method_v_type)(JNIEnv *env, jobject obj, jmethodID mid, void *args);
call_object_method_v_type old_call_object_method_v = 0;
jobject my_call_object_method_v(JNIEnv *env, jobject obj, jmethodID mid, void *args) {
    jobject r = old_call_object_method_v(env, obj, mid, args);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "CallObjectMethodV %p %p %p tid %d return %p", obj, mid, args, gettid(), r);
    return r;
}

call_object_method_v_type old_call_static_object_method_v = 0;
jobject my_call_static_object_method_v(JNIEnv *env, jobject obj, jmethodID mid, void *args) {
    jobject r = old_call_static_object_method_v(env, obj, mid, args);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "CallStaticObjectMethodV %p %p %p tid %d return %p", obj, mid, args, gettid(), r);
    return r;
}

typedef jint (*call_int_method_v_type)(JNIEnv *env, jobject obj, jmethodID mid, void *args);
call_int_method_v_type old_call_int_method_v = 0;
jint my_call_int_method_v(JNIEnv *env, jobject obj, jmethodID mid, void *args) {
    jint r = old_call_int_method_v(env, obj, mid, args);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "CallIntMethodV %p %p %p tid %d return %d", obj, mid, args, gettid(), r);
    return r;
}

call_int_method_v_type old_call_static_int_method_v = 0;
jint my_call_static_int_method_v(JNIEnv *env, jobject obj, jmethodID mid, void *args) {
    jint r = old_call_static_int_method_v(env, obj, mid, args);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "CallStaticIntMethodV %p %p %p tid %d return %d", obj, mid, args, gettid(), r);
    return r;
}

typedef jboolean (*call_bool_method_v_type)(JNIEnv *env, jobject obj, jmethodID mid, void *args);
call_bool_method_v_type old_call_bool_method_v = 0;
jboolean my_call_bool_method_v(JNIEnv *env, jobject obj, jmethodID mid, void *args) {
    jboolean r = old_call_bool_method_v(env, obj, mid, args);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "CallBooleanMethodV %p %p %p tid %d return %d", obj, mid, args, gettid(), r);
    return r;
}

call_bool_method_v_type old_call_static_bool_method_v = 0;
jboolean my_call_static_bool_method_v(JNIEnv *env, jobject obj, jmethodID mid, void *args) {
    jboolean r = old_call_static_bool_method_v(env, obj, mid, args);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj",
                        "CallStaticBooleanMethodV %p %p %p tid %d return %d", obj, mid, args,
                        gettid(), r);
    return r;
}


typedef jint (*register_native_type)(JNIEnv *env, jclass clazz, const JNINativeMethod* methods,
                        jint nMethods);

register_native_type old_register_native = 0;

jint my_register_native(JNIEnv *env, jclass clazz, const JNINativeMethod* methods,
                     jint nMethods) {

    for (int i = 0; i < nMethods; i++)  {
        const JNINativeMethod &m = methods[i];
        __android_log_print(ANDROID_LOG_INFO, "librev-dj",
                            "RegisterNative name=%s sig=%s ptr=%p tid=%d", m.name, m.signature, m.fnPtr, gettid());
    }

    jint r = old_register_native(env, clazz, methods, nMethods);
    return r;
}

void hook_jni(JNIEnv *env) {
    MSHookFunction((void*)env->functions->FindClass, (void*)my_find_class, (void**)&old_find_class);
    MSHookFunction((void*)env->functions->GetMethodID, (void*)my_get_method_id, (void**)&old_get_method_id);
    MSHookFunction((void*)env->functions->GetStaticMethodID, (void*)my_get_static_method_id, (void**)&old_get_static_method_id);
    MSHookFunction((void*)env->functions->CallObjectMethodV, (void*)my_call_object_method_v, (void**)&old_call_object_method_v);
    MSHookFunction((void*)env->functions->CallStaticObjectMethod, (void*)my_call_static_object_method_v, (void**)&old_call_static_object_method_v);

    MSHookFunction((void*)env->functions->CallIntMethodV, (void*)my_call_int_method_v, (void**)&old_call_int_method_v);
    MSHookFunction((void*)env->functions->CallStaticIntMethodV, (void*)my_call_static_int_method_v, (void**)&old_call_static_int_method_v);

    MSHookFunction((void*)env->functions->CallBooleanMethodV, (void*)my_call_bool_method_v, (void**)&old_call_bool_method_v);
    MSHookFunction((void*)env->functions->CallStaticBooleanMethodV, (void*)my_call_static_bool_method_v, (void**)&old_call_static_bool_method_v);

    MSHookFunction((void*)env->functions->RegisterNatives, (void*)my_register_native, (void**)&old_register_native);

}
