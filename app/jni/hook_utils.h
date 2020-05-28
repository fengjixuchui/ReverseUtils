//
// Created by my on 20-4-27.
//

#include <jni.h>
typedef jint (*JNI_OnLoad_Type)(JavaVM *);
JNI_OnLoad_Type hook_jni_onload(JNI_OnLoad_Type ori_load, JNI_OnLoad_Type new_jni_onload);

bool is_package_name(const char *pkgName, bool useLike=false);

void hook_jni(JNIEnv *env);

void wait_for_attach();

