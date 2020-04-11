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
#include <dirent.h>
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
    char a = -71;
    unsigned int b = (unsigned int)a;

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

int get_tids(void (*func)(pid_t, void *aux),void*aux){
    DIR *proc_dir;
    char dirname[256] = {0};
    pid_t pid;
    if ( ! func ) return -1;

    snprintf(dirname, sizeof(dirname), "/proc/%d/task", getpid());
    proc_dir = opendir(dirname);

    if (proc_dir) {
        /*  /proc available, iterate through tasks... */
        struct dirent *entry;
        while ((entry = readdir(proc_dir)) != NULL) {
            if(entry->d_name[0] == '.')
                continue;
            pid = atoi(entry->d_name);
            func(pid, aux);
        }
        closedir(proc_dir);
        return 0;
    } else {
        return -1;
    }
}

void tids_callback(pid_t pid, void *aux) {
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "tid %d", pid);
    pid_t cur_thread = gettid();
    if (pid != cur_thread && pid != getpid()) {
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "stoping thread %d", pid);
        kill(pid, SIGSTOP);
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after stoping thread %d", pid);
    }
}

void stop_all_others_threads() {
    get_tids(tids_callback, 0);
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
cb_type cb2 = 0;

int my_anti_hook_proc(void *p) {
    //DUMP_CALL_STACK("lib-rev-dj");
    //注意，不能return，可能dy原来这个函数根本没打算return，以return就会crash，所以不返回就行
    char name[255] = "";
    prctl(PR_GET_NAME, name, 0, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "cb2 call hold %d %s!!!", gettid(), name);
    sleep(10000000);
    return 0;
    //return cb2(p);
}

typedef jbyteArray (*lev_type) (JNIEnv *env, jclass thiz, jint p1, jbyteArray p2);
lev_type lev_ori=0;
void print_byte_array(char *buf, JNIEnv *env, jbyteArray rlev) {
    if (rlev) {
        int off = 0;
        int narr = env->GetArrayLength(rlev);
        jbyte *ba = env->GetByteArrayElements(rlev, 0);
        for (int i = 0; i < narr; i++) {
            off += sprintf(buf + off, "%02x", (unsigned char)ba[i]);
        }
    }
}

void force_call_lev(JNIEnv *env, bool wait=false, char *out=0) {

    int n = 64;
    //jbyte b[] = {71,57,-52,16,-33,-74,56,-78,88,-1,81,113,90,-56,-109,-114,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,-89,102,-14,26,-10,-97,-18,-41,27,113,-106,-61,36,106,-12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    jbyte b[] = {-84, -34, 116, -87, 78, 107, 73, 58, 51, -103, -6, -56, 60, 124, 8, -77, 93, 88, -78, 29, -107, -126, -81, 119, 100, 127, -55, -112, 46, 54, -82, 112, -7, -64, 1, -23, 51, 78, 110, -108, -111, 102, -126, 34, 79, -66, 78, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    jbyteArray barray = env->NewByteArray(n);
    env->SetByteArrayRegion(barray, 0, n, b);

    char name[255] = "";

    prctl(PR_GET_NAME, name, 0, 0);
    //jint p1 = 1585841725;
    jint p1 = 1585841725;
    jclass cls = env->FindClass("com/ss/sys/ces/a");
    char buf2[512] = "[null]";
    print_byte_array(buf2, env, barray);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev %p tid %d %s input %s", lev_ori, gettid(), name, buf2);
    if (wait)
        wait_for_attach(gettid());
    jbyteArray rlev = lev_ori(env, cls, p1, barray);
    if (wait)
        syscall(1,111);

    char buf[512] = "[null]";
    print_byte_array(buf, env, rlev);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev tid %d %s return %s", gettid(), name, buf);
    env->DeleteLocalRef(cls);
    if (out)
        strcpy(out, buf);
}

void *dummp_thread(void *p) {
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "dummp_thread run");
    /*
    const char *name = "my-thread";
    prctl(PR_SET_NAME, name, 0, 0);
    JNIEnv *env = 0;
    g_vm->AttachCurrentThread(&env, 0);
    force_call_lev(env);
    g_vm->DetachCurrentThread();
     */
    return 0;
}

typedef jobject (*meta_type)(JNIEnv *env, jclass clz, jint n, jobject ctx, jobject arg);
meta_type old_meta = 0;
static int count = 0;
jobject my_meta(JNIEnv *env, jclass clz, jint optype, jobject ctx, jobject arg) {

    pid_t tid = gettid();
    char name[255] = "";

    prctl(PR_GET_NAME, name, 0, 0);
    /*
    if (optype == 101) {
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_meta tid %d [%s] %d skip", tid, name, optype);
        return 0;
    }
     */
    const char *sarg = "null";
    if (arg != 0) {
        if ((optype >= 101 && optype <=110) ||
                optype == 222 || optype == 302 || optype == 1020) {

            //string
            jstring myarg = (jstring)arg;
            sarg = env->GetStringUTFChars(myarg, 0);
        }
        else if (optype == 601 || optype == 200) {
            jbyteArray myarg = (jbyteArray)arg;
            sarg = "[bytes]";
        }
        else {
            sarg = "[unknown]";
        }
    }

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_meta call tid %d [%s] %d %p %s!!!", tid, name, optype, ctx, sarg);

    jobject r = old_meta(env, clz, optype, ctx, arg);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_meta tid %d return %p!!!", tid, r);

    /*
    count++;
    if (count == 3) {
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force lev after meta");

        pthread_t t;
        pthread_create(&t, 0, dummp_thread, 0);
        //force_call_lev(env);
        int rp = 0;
        //wait_for_attach(gettid());
        //force_call_lev(env);
        //syscall(1, 111);
    }
     */
    return r;
}

jbyteArray my_lev(JNIEnv *env, jclass thiz, jint p1, jbyteArray p2) {
    pid_t tid = gettid();
    char name[255] = "";

    prctl(PR_GET_NAME, name, 0, 0);
    char buf2[512] = "[null]";
    print_byte_array(buf2, env, p2);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_lev tid %d [%s] input %s!!!", tid, name, buf2);

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

    //force_call_lev(env);
    //wait_for_attach(gettid());
    jbyteArray r = lev_ori(env, thiz, p1, p2);

    char buf[512] = "[null]";
    print_byte_array(buf, env, r);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "my_lev tid %d [%s] return %s!!!", tid, name, buf);


    return r;
}


void hook_jni(JNIEnv *env);

void fake_meta(JNIEnv *env) {
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call meta %p tid %d", old_meta, gettid());
    jclass clz = env->FindClass("com/ss/sys/ces/a");
    old_meta(env, clz, 101, 0, env->NewStringUTF("0"));

    old_meta(env, clz, 102, 0, env->NewStringUTF("1028"));
    old_meta(env, clz, 1020, 0, env->NewStringUTF(""));
}


int fun(int n)//定义fun函数
{
    int z;
    if (n > 2)
    {
        z = fun(n - 1) + fun(n - 2);//直接调用本身，由fun(n)转换为fun(n-1)+fun(n-2)
    }
    else
        z = 1;//n=2时退出返回到main函数
    return z;
}

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

    /*
    unsigned *ptr1 = (unsigned*)((unsigned)g_base_addr + 0x93da4);
    unsigned *ptr2 = (unsigned*)((unsigned)g_base_addr + 0x95f3c);
     */

    char name[255] = "tp-io-21";
    //prctl(PR_SET_NAME, name, 0, 0);
    prctl(PR_GET_NAME, name, 0, 0);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "jni_onload %d %s", gettid(), name);

    /*
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call meta after jni onload");
    fake_meta(env);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "force call lev after jni onload");
    force_call_lev(env);
     */

    //pthread_t t;
    //pthread_create(&t, 0, dummp_thread, 0);
    //int rp = 0;
    //sleep(2);


    char outbuf[512] = {0};
    char path[256] = {0};
    int count = 0;
    /*
    stop_all_others_threads();
    while(true) {
        //wait_for_attach(gettid());
        char tmpbuf[512] = {0};
        __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before force call lev");
        force_call_lev(env, false, tmpbuf);
        if (strcmp(tmpbuf, outbuf) != 0) {
            sprintf(path, "/sdcard/cms-%d.so", count++);
            __android_log_print(ANDROID_LOG_INFO, "librev-dj", "output is different write to %s", path);
            FILE *f = fopen(path, "wb");
            __android_log_print(ANDROID_LOG_INFO, "librev-dj", "open return %p", f);
            fwrite(g_base_addr, 1, 0x97000, f);
            fclose(f);
            strcpy(outbuf, tmpbuf);
        }
        sleep(1);
    }
     */


    //wait_for_attach(gettid());
    return r;
}



__attribute__((constructor)) void __init__() {

    const char *pkgName = "com.ss.android.ugc.aweme";
    const char *pkgName2 = "com.leaves.httpServer";

    const char *path = "/proc/self/cmdline";
    char buf[300] = {0};
    FILE *f = fopen(path, "rb");
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    //__android_log_print(ANDROID_LOG_INFO, "librev-dj", "pkg_name %s", buf);
    //__android_log_print(ANDROID_LOG_FATAL, TAG, "cmdline %s", buf);
    if (strcmp(buf, pkgName)!=0) {
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

    g_base_addr = mapInfo.baseAddr;
    void *ori_load = dlsym(cms, "JNI_OnLoad");

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook jni_onload");
    MSHookFunction((void*)ori_load, (void*)my_jni_onload, (void**)&old_jni_onload);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook jni_onload %p", old_jni_onload);


    void *lev = (void*)((unsigned)g_base_addr + 0x0005A788+1);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook lev");
    MSHookFunction((void*)lev, (void*)my_lev, (void**)&lev_ori);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook lev %p", lev_ori);

    void *meta = (void*)((unsigned)g_base_addr + 0x65cb4+1);

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook meta");
    MSHookFunction((void*)meta, (void*)my_meta, (void**)&old_meta);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook meta %p", old_meta);

    /*
    //0x00065ED0 这个是反hook的函数
    //但是这样hook会导致网络登录卡住，这是由于这个函数有其他功能的原因，暂时不追究
    void *hook_anti_cb2 = (void*)((unsigned)mapInfo.baseAddr + 0x00065ED0+1);
    //0x66116 read syscall addr anti hook 0000836E maybe anti hook

    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook2");
    MSHookFunction((void *) hook_anti_cb2, (void *) my_anti_hook_proc, (void **) &cb2);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook 2 %p", cb2);
     */


    /*
    void *syscall = (void*)((unsigned)mapInfo.baseAddr + 0x00009E7C);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "before hook");
    MSHookFunction((void*)syscall, (void*)__sys_c, (void**)&sys_ori);
    __android_log_print(ANDROID_LOG_INFO, "librev-dj", "after hook %p", sys_ori);
     */

    //void *clone_call = (void*)((unsigned)mapInfo.baseAddr + 0x000189EC);
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

    //wait_for_attach();
}
