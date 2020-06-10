package com.example.xposedtest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

/**
 * Created by maiyao on 2018/6/13.
 */

public class XiamiHookProcs implements IXposedHookLoadPackage {

    private String bytes2Hex(byte[] bytes) {
        StringBuffer result = new StringBuffer();
        for (byte b : bytes) {
            result.append(String.format("%02X", b));
        }
        return result.toString();
    }

    @Override
    public void handleLoadPackage(final XC_LoadPackage.LoadPackageParam lpparam) {
        //app启动时调用
        String pkgName = lpparam.packageName;
        if (pkgName.equals("fm.xiami.main")) {
            XposedBridge.log("***************************load-Xposed-xiami*****************************");
            findAndHookMethod("mtopsdk.security.b",
                    lpparam.classLoader, "getMtopApiSign", HashMap.class, String.class, String.class,
                    new XC_MethodHook() {
                        @Override
                        protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                            super.beforeHookedMethod(param);
                            XposedBridge.log("***************************getMtopApiSign*****************************");
                        }


                        @Override
                        protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                            super.afterHookedMethod(param);
                            String r = (String) param.getResult();
                            XposedBridge.log("return " + r);
                        }
                    });
        }

        findAndHookMethod("com.alibaba.wireless.security.open.SecurityGuardManager",
                lpparam.classLoader, "a", int.class,
                new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                        super.beforeHookedMethod(param);
                        int id = (int)param.args[0];
                        XposedBridge.log("***************************a*****************************");;
                        XposedBridge.log("id " + id);

                    }

                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                        Object r = (Object) param.getResult();
                        //打印出实现类名
                        XposedBridge.log("return " + r.toString());
                    }
                });
        String clsName = "com.taobao.wireless.security.adapter.datacollection.DeviceInfoCapturer";
        XposedUtils.superHook(clsName, "doCommandForString", int.class, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                super.beforeHookedMethod(param);
            }

            @Override
            protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                super.afterHookedMethod(param);
                int id = (int)param.args[0];
                String r = (String) param.getResult();
                XposedBridge.log(String.format("doCommandForString params %d return %s", id, r));
            }
        });

    }
}

