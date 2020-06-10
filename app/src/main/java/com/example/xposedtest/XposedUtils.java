package com.example.xposedtest;

import android.os.Build;
import android.util.Log;

import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;

import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

public class XposedUtils {

    //能hook住所有类，不管哪个classloader的
    public static void superHook(final String className, final String methodName, final Object... argsAndCallBack) {
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
                    XposedBridge.log(String.format("super hook %s classloader %s", className, cls.getClassLoader().toString()));
                    findAndHookMethod(cls, methodName, argsAndCallBack);
                }
            }
        });
    }
}
