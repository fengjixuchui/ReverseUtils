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

#define TAG "librev-dj"

JavaVM *g_vm = 0;
void *g_base_addr = 0;
void sys_trace();
extern "C" JNIEXPORT jstring JNICALL
Java_com_reverse_my_reverseutils_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    //sys_trace();
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

int get_tracer(pid_t pid) {
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

void wait_for_attach(pid_t pid) {
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

typedef int (*sys_type)(int num, int p1, int p2, int p3);
sys_type sys_ori = 0;

int __sys_c(int num, int p1, int p2, int p3) {
    //原函数是靠r0-r7传递参数的，所以只hook前四个参数，后面不符合函数调用约定了
    int trueNum = num - 0xE9;
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "num 0x%x is call [0x%08X] [0x%08X] [0x%08X]", trueNum, p1, p2, p3);
    return sys_ori(num, p1, p2, p3);
}
sys_type sys_ori2 = 0;

int __sys_c2(int p0, int p1, int p2, int p3) {
    //原函数是靠r0-r7传递参数的，所以只hook前四个参数，后面不符合函数调用约定了
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "clone is call [0x%08X] [0x%08X] [0x%08X] [0x%08X]", p0, p1, p2, p3);
    return sys_ori2(p0, p1, p2, p3);
}

typedef int (*pthread_create_type)(pthread_t *thread, pthread_attr_t const * attr,
                   void *(*start_routine)(void *), void * arg);

pthread_create_type pthread_create_ori = 0;
int my_pthread_create(pthread_t *thread, pthread_attr_t const * attr,
                                   void *(*start_routine)(void *), void * arg) {
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "call pthread_create");
    int r = pthread_create_ori(thread, attr, start_routine, arg);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after call pthread_create %d, route %p",
                        (int)(*thread), start_routine);
    return r;
}


typedef int (*cb_type) (void *p);
cb_type cb = 0;
cb_type cb2 = 0;

int my_cb (void *p) {
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cb call skip!!!");
    return 0;
    return cb(p);
}

int my_anti_hook_proc(void *p) {
    //DUMP_CALL_STACK("lib-rev-dj");
    //注意，不能return，可能dy原来这个函数根本没打算return，以return就会crash，所以不返回就行
    sleep(10000000);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cb2 call skip!!!");
    return 0;
    //return cb2(p);
}

typedef jbyteArray (*lev_type) (JNIEnv *env, jclass thiz, jint p1, jbyteArray p2);
lev_type lev_ori=0;
void *dummp_thread(void *p) {
    JNIEnv *env = 0;
    g_vm->AttachCurrentThread(&env, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "in thread %d env %p", gettid(), env);
    sleep(0);
    int n = 64;
    jbyte b[] = {71,57,-52,16,-33,-74,56,-78,88,-1,81,113,90,-56,-109,-114,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,-89,102,-14,26,-10,-97,-18,-41,27,113,-106,-61,36,106,-12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    jbyteArray barray = env->NewByteArray(n);
    env->SetByteArrayRegion(barray, 0, n, b);

    jint p1 = 1585841725;
    jclass cls = env->FindClass("com/ss/sys/ces/a");
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev %p in thread %d", lev_ori, gettid());
    jbyteArray rlev = lev_ori(env, cls, p1, barray);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev return %p in thread %d", rlev, gettid());
    g_vm->DetachCurrentThread();
    return 0;
}

jbyteArray my_lev(JNIEnv *env, jclass thiz, jint p1, jbyteArray p2) {
    pid_t tid = gettid();
    char name[255] = "";

    prctl(PR_GET_NAME, name, 0, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_lev tid %d [%s]!!!", tid, name);
    wait_for_attach(tid);

    unsigned *ptr1 = (unsigned*)((unsigned)g_base_addr + 0x93da4);
    unsigned *ptr2 = (unsigned*)((unsigned)g_base_addr + 0x95f3c);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_lev %d ptr1 0x%08X ptr2 0x%08X", tid, *ptr1, *ptr2);

    /*
    int n = env->GetArrayLength(p2);
    std::string s = "{";
    jbyte *b = env->GetByteArrayElements(p2, 0);
    for (int i = 0; i < n; i++) {
        char buf[100] = "";
        sprintf(buf, "%d", b[i]);
        s = s + "," + buf;
    }
    s+= "}";

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "p1:%d, nsz:%d %s", p1, n, s.c_str());
     */
    /*
    pthread_t t;
    pthread_create(&t, 0, dummp_thread, 0);
    int rp = 0;
    pthread_join(t, (void**)&rp);
     */

    jbyteArray r = lev_ori(env, thiz, p1, p2);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_lev tid %d return %p!!!", tid, (void*)r);
    syscall(1, 231);

    return r;
}


void hook_jni(JNIEnv *env);
#include <map>
using namespace std;
typedef jint (*jni_onload_type)(JavaVM *);
jni_onload_type old_jni_onload = 0;
jint my_jni_onload(JavaVM *vm) {

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my jni onload call tid %d", gettid());
    g_vm = vm;
    JNIEnv *env = 0;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);

    hook_jni(env);
    jint r = old_jni_onload(vm);

    unsigned *ptr1 = (unsigned*)((unsigned)g_base_addr + 0x93da4);
    unsigned *ptr2 = (unsigned*)((unsigned)g_base_addr + 0x95f3c);

    char name[255] = "tp-io-21";
    //prctl(PR_SET_NAME, name, 0, 0);
    prctl(PR_GET_NAME, name, 0, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "jni_onload %d %s ptr1 0x%08X ptr2 0x%08X", gettid(), name, *ptr1, *ptr2);
    int n = 64;
    jbyte b[] = {71,57,-52,16,-33,-74,56,-78,88,-1,81,113,90,-56,-109,-114,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,-89,102,-14,26,-10,-97,-18,-41,27,113,-106,-61,36,106,-12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    jbyteArray barray = env->NewByteArray(n);
    env->SetByteArrayRegion(barray, 0, n, b);

    map<unsigned , unsigned char> m;
    m[0x000942C8] = 0x1;
    m[0x000942C9] = 0x0;
    m[0x000942CA] = 0x0;
    m[0x000942CB] = 0x0;
    m[0x000942CC] = 0x6d;
    m[0x000942CD] = 0xcf;
    m[0x000942D4] = 0x2f;
    m[0x000942D5] = 0x64;
    m[0x000942D6] = 0x65;
    m[0x000942D7] = 0x76;
    m[0x000942D8] = 0x2f;
    m[0x000942D9] = 0x6c;
    m[0x000942DA] = 0x6f;
    m[0x000942DB] = 0x67;
    m[0x000942DC] = 0x2f;
    m[0x000942DD] = 0x6d;
    m[0x000942DE] = 0x61;
    m[0x000942DF] = 0x69;
    m[0x000942E0] = 0x6e;
    m[0x000942E1] = 0x0;
    m[0x000942E4] = 0x46;
    m[0x000942E5] = 0x0;
    m[0x000942E6] = 0x0;
    m[0x000942E7] = 0x0;
    m[0x000942E8] = 0x47;
    m[0x000942E9] = 0x0;
    m[0x000942EA] = 0x0;
    m[0x000942EB] = 0x0;
    m[0x000942EC] = 0x48;
    m[0x000942ED] = 0x0;
    m[0x000942EE] = 0x0;
    m[0x000942EF] = 0x0;
    m[0x000942F0] = 0x49;
    m[0x000942F1] = 0x0;
    m[0x000942F2] = 0x0;
    m[0x000942F3] = 0x0;
    m[0x00094304] = 0x2f;
    m[0x00094305] = 0x64;
    m[0x00094306] = 0x65;
    m[0x00094307] = 0x76;
    m[0x00094308] = 0x2f;
    m[0x00094309] = 0x6c;
    m[0x0009430A] = 0x6f;
    m[0x0009430B] = 0x67;
    m[0x0009430C] = 0x2f;
    m[0x0009430D] = 0x72;
    m[0x0009430E] = 0x61;
    m[0x0009430F] = 0x64;
    m[0x00094310] = 0x69;
    m[0x00094311] = 0x6f;
    m[0x00094312] = 0x0;
    m[0x00094313] = 0x2f;
    m[0x00094314] = 0x64;
    m[0x00094315] = 0x65;
    m[0x00094316] = 0x76;
    m[0x00094317] = 0x2f;
    m[0x00094318] = 0x6c;
    m[0x00094319] = 0x6f;
    m[0x0009431A] = 0x67;
    m[0x0009431B] = 0x2f;
    m[0x0009431C] = 0x65;
    m[0x0009431D] = 0x76;
    m[0x0009431E] = 0x65;
    m[0x0009431F] = 0x6e;
    m[0x00094320] = 0x74;
    m[0x00094321] = 0x73;
    m[0x00094322] = 0x0;
    m[0x00094323] = 0x2f;
    m[0x00094324] = 0x64;
    m[0x00094325] = 0x65;
    m[0x00094326] = 0x76;
    m[0x00094327] = 0x2f;
    m[0x00094328] = 0x6c;
    m[0x00094329] = 0x6f;
    m[0x0009432A] = 0x67;
    m[0x0009432B] = 0x2f;
    m[0x0009432C] = 0x73;
    m[0x0009432D] = 0x79;
    m[0x0009432E] = 0x73;
    m[0x0009432F] = 0x74;
    m[0x00094330] = 0x65;
    m[0x00094331] = 0x6d;
    m[0x00094332] = 0x0;
    m[0x00094EA0] = 0x0;
    m[0x00094F74] = 0x41;
    m[0x00094F75] = 0x59;
    m[0x00094F76] = 0x45;
    m[0x00094F77] = 0x0;
    m[0x00094F78] = 0x4e;
    m[0x00094F79] = 0x45;
    m[0x00094F7A] = 0x49;
    m[0x00094F7B] = 0x4e;
    m[0x00094F7C] = 0x0;
    m[0x00094F7D] = 0x43;
    m[0x00094F7E] = 0x5a;
    m[0x00094F7F] = 0x4c;
    m[0x00094F80] = 0x5f;
    m[0x00094F81] = 0x4d;
    m[0x00094F82] = 0x45;
    m[0x00094F83] = 0x54;
    m[0x00094F84] = 0x41;
    m[0x00094F85] = 0x0;
    m[0x00094F90] = 0xd;
    m[0x00094F91] = 0xa;
    m[0x00094F92] = 0x5b;
    m[0x00094F93] = 0x25;
    m[0x00094F94] = 0x64;
    m[0x00094F95] = 0x5d;
    m[0x00094F96] = 0x20;
    m[0x00094F97] = 0x20;
    m[0x00094F98] = 0x20;
    m[0x00094F99] = 0x20;
    m[0x00094F9A] = 0x6d;
    m[0x00094F9B] = 0x65;
    m[0x00094F9C] = 0x74;
    m[0x00094F9D] = 0x61;
    m[0x00094F9E] = 0x28;
    m[0x00094F9F] = 0x25;
    m[0x00094FA0] = 0x64;
    m[0x00094FA1] = 0x29;
    m[0x00094FA2] = 0xd;
    m[0x00094FA3] = 0xa;
    m[0x00094FA4] = 0x0;
    m[0x00095688] = 0x80;
    m[0x00095689] = 0x12;
    m[0x0009568A] = 0x3c;
    m[0x0009568B] = 0x71;
    m[0x0009568C] = 0xc0;
    m[0x0009568D] = 0x14;
    m[0x0009568E] = 0x3c;
    m[0x0009568F] = 0x71;
    m[0x00095690] = 0x8;
    m[0x00095691] = 0xa9;
    m[0x00095692] = 0x36;
    m[0x00095693] = 0x7b;
    m[0x00095694] = 0x38;
    m[0x00095695] = 0xf;
    m[0x00095696] = 0x4a;
    m[0x00095697] = 0x7b;
    m[0x00095698] = 0x90;
    m[0x00095699] = 0xb5;
    m[0x0009569A] = 0x9b;
    m[0x0009569B] = 0x7c;
    m[0x000956B8] = 0x1;
    m[0x000956BC] = 0x1;
    m[0x000956C0] = 0x1;
    m[0x000956C4] = 0x1;
    m[0x00096404] = 0x1;
    m[0x00096405] = 0x1;
    m[0x00096978] = 0x68;
    m[0x00096979] = 0x4;
    m[0x0009697C] = 0x40;
    m[0x0009697D] = 0x9a;
    m[0x0009697E] = 0x36;
    m[0x0009697F] = 0x7b;
    m[0x00096980] = 0x52;
    m[0x00096981] = 0x3;
    m[0x00096A20] = 0x9;
    m[0x00096A84] = 0x1;
    m[0x00096A88] = 0x1;
    m[0x00096A8C] = 0x1;
    m[0x00096A90] = 0x1;

    unsigned char *baddr = (unsigned char*)g_base_addr;
    map<unsigned , unsigned char >::iterator it;
    for (; it != m.end(); ++it) {
        unsigned off = it->first;
        unsigned char v = it->second;
        *(baddr+off) = v;
    }
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force write lev");

    jint p1 = 1585841725;
    jclass cls = env->FindClass("com/ss/sys/ces/a");
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev %p", lev_ori);
    jbyteArray rlev = lev_ori(env, cls, p1, barray);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev return %p", rlev);


    /*
    pthread_t t;
    pthread_create(&t, 0, dummp_thread, 0);
    int rp = 0;
     */
    //pthread_join(t, (void**)&rp);

            /*
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call 2 lev %p", lev_ori);
    rlev = lev_ori(env, cls, p1, barray);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call 2 lev return %p", rlev);
    syscall(1, 11);
             */
    wait_for_attach(gettid());
    return r;
}



__attribute__((constructor)) void __init__() {
    const char *path = "/proc/self/cmdline";
    char buf[300] = {0};
    FILE *f = fopen(path, "rb");
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    const char *pkgName = "com.ss.android.ugc.aweme";
    const char *pkgName2 = "com.leaves.httpServer";
    //__android_log_print(ANDROID_LOG_INFO, "librev-dj", "pkg_name %s", buf);
    //__android_log_print(ANDROID_LOG_FATAL, TAG, "cmdline %s", buf);
    if (strcmp(buf, pkgName)!=0 && strcmp(buf, pkgName2) !=0) {
        //__android_log_print(ANDROID_LOG_FATAL, TAG, "%s not the target pkgName", pkgName);
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "douyin call pid %d!!!", getpid());
    char cms_path[255];
    sprintf(cms_path, "/data/data/%s/lib/libcms.so", pkgName);
    void *cms = dlopen(cms_path, RTLD_NOW);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cms %p", cms);

    MapInfo mapInfo;
    get_map_infos(&mapInfo, "libcms.so");

    /*
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cms base %p", mapInfo.baseAddr);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cms end %p", mapInfo.endAddr);

    void *hook_anti_cb = (void*)((unsigned)mapInfo.baseAddr + 0x0005C070+1);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook");
    MSHookFunction((void*)hook_anti_cb, (void*)my_cb, (void**)&cb);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook %p", cb);
     */

    g_base_addr = mapInfo.baseAddr;
    void *ori_load = dlsym(cms, "JNI_OnLoad");

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook jni_onload");
    MSHookFunction((void*)ori_load, (void*)my_jni_onload, (void**)&old_jni_onload);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook jni_onload %p", old_jni_onload);


    void *lev = (void*)((unsigned)g_base_addr + 0x0005A788+1);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook lev");
    MSHookFunction((void*)lev, (void*)my_lev, (void**)&lev_ori);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook lev %p", lev_ori);

    /*
    //0x00065ED0 这个是反hook的函数
    //但是这样hook会导致网络登录卡住，这是由于这个函数有其他功能的原因，暂时不追究
    void *hook_anti_cb2 = (void*)((unsigned)mapInfo.baseAddr + 0x00065ED0+1);
    //0x66116 read syscall addr anti hook 0000836E maybe anti hook

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook2");
    MSHookFunction((void *) hook_anti_cb2, (void *) my_anti_hook_proc, (void **) &cb2);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook 2 %p", cb2);

    void *syscall = (void*)((unsigned)mapInfo.baseAddr + 0x00009E7C);


    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook");
    MSHookFunction((void*)syscall, (void*)__sys_c, (void**)&sys_ori);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook %p", sys_ori);
    */

    void *clone_call = (void*)((unsigned)mapInfo.baseAddr + 0x000189EC);
    /*
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook clone");
    MSHookFunction((void*)clone_call, (void*)__sys_c2, (void**)&sys_ori2);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook clone %p", sys_ori2);
     */

    /*
    void *b = (void *)((unsigned )syscall & (~0x0FFF));
    mprotect(b, 0x1000, PROT_NONE);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "mprotect %p", b);
     */

    /*
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook pthread");
    MSHookFunction((void*)pthread_create, (void*)my_pthread_create, (void**)&pthread_create_ori);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook pthread %p", pthread_create_ori);
     */

    __android_log_print(ANDROID_LOG_FATAL, "librev-dj", "pkgName %s here", buf);
    //wait_for_attach();
}
