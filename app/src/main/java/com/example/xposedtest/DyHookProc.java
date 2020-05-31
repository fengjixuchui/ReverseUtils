package com.example.xposedtest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

public class DyHookProc implements IXposedHookLoadPackage {

    @Override
    public void handleLoadPackage(final XC_LoadPackage.LoadPackageParam lpparam) {
        //app启动时调用
        String pkgName = lpparam.packageName;
        if (pkgName.equals("com.ss.android.ugc.aweme")) {
            //if (pkgName.equals("com.leaves.httpServer")) {
            //System.load("/data/local/tmp/librev.so");
            //XposedBridge.log("so load...");
            findAndHookMethod("java.lang.Thread", lpparam.classLoader, "getStackTrace",
                    new XC_MethodHook() {
                        @Override
                        protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                            super.beforeHookedMethod(param);
                            XposedBridge.log("***************************getStackTrace*****************************");
                            StackTraceElement[] eles = (StackTraceElement[]) XposedBridge.invokeOriginalMethod(param.method, param.thisObject, param.args);

                            StackTraceElement ele1 = new StackTraceElement("com.ss.sys.ces.a", "", "", 0);
                            StackTraceElement ele2 = new StackTraceElement("com.ss.sys.ces.gg.tt$1", "", "", 0);
                            StackTraceElement ele3 = new StackTraceElement("com.bytedance.frameworks.baselib.network.http.e.a", "", "", 0);
                            eles[2] = ele1;
                            eles[3] = ele2;
                            eles[4] = ele3;
                            param.setResult(eles);
                        }


                        @Override
                        protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                            super.afterHookedMethod(param);
                        }
                    });
        findAndHookMethod("com.ss.sys.ces.a", lpparam.classLoader, "leviathan", int.class, byte[].class,
                new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                        super.beforeHookedMethod(param);
                        Integer p1 = (Integer) param.args[0];
                        byte[] p2 = (byte[]) param.args[1];
                        XposedBridge.log("***************************lev-hook-before*****************************");
                        String s = Arrays.toString(p2);
                        XposedBridge.log(String.format("pkg=%d,flag=%s", p1, s));
                        Thread t = Thread.currentThread();
                        StackTraceElement[] traceElements = t.getStackTrace();
                        String str = "";
                        for (StackTraceElement e : traceElements) {
                            str += e.getClassName() + "\n";
                        }
                        XposedBridge.log(String.format("thread %d %s", t.getId(), t.getName()));
                        XposedBridge.log(str);
                    }

                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                    }
                });
        }

    }
}

