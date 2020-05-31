package com.example.xposedtest;

import android.os.Build;
import android.util.Log;

import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;

import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

public class XposedUtils {

    public static void hook(final String className, ClassLoader classLoader, final String methodName, final Object... argsAndCallBack) {
        if (Build.VERSION.SDK_INT<=19) {
            findAndHookMethod(ClassLoader.class, "loadClass", String.class, new XC_MethodHook() {
                @Override
                protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                    if (param.hasThrowable()) return;
                    Class<?> cls = (Class<?>) param.getResult();
                    String name = cls.getName();
                    if (name.equals(className)) {
                        // 所有的类都是通过loadClass方法加载的
                        // 所以这里通过判断全限定类名，查找到目标类
                        // 第二步：Hook目标方法
                        XposedBridge.log("4.4 hook");
                        findAndHookMethod(cls, methodName, argsAndCallBack);
                        XposedBridge.log("after 4.4 hook");
                    }
                }
            });
        }
        else {
            findAndHookMethod(className, classLoader, methodName, argsAndCallBack);
        }
    }
}
