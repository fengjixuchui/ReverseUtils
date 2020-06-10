package com.example.xposedtest;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Bundle;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Array;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;

import dalvik.system.DexFile;
import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

import static de.robv.android.xposed.XposedHelpers.findAndHookConstructor;
import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

/**
 * Created by maiyao on 2018/6/13.
 */

public class NmHookProcs implements IXposedHookLoadPackage {

    public static void writeLog(String dataDir, String log) {
        File logFile = new File(dataDir, "mapsLog.txt");
        FileWriter fw = null;
        try {
            fw = new FileWriter(logFile, true);
            fw.write(log + "\n");
            fw.flush();
            fw.close();
        } catch (FileNotFoundException e) {
            System.out.println("file not found!");
        } catch (IOException e) {
            System.out.println("Output error!");
        }
    }

    private void printMaps(String dataDir) {

        List<String> r = new ArrayList<>();
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader("/proc/self/maps"));
            String line = reader.readLine();
            while (!line.isEmpty()) {
                writeLog(dataDir, line);
                line = reader.readLine();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

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
        //if (pkgName.equals("com.ss.android.ugc.aweme")) {
        //if (pkgName.equals("com.leaves.httpServer")) {
        if (pkgName.equals("com.netease.cloudmusic")) {
            XposedBridge.log("***************************load-Xposed-cloudmusic*****************************");
            findAndHookMethod("com.netease.cloudmusic.utils.NeteaseMusicUtils",
                    lpparam.classLoader, "deserialdata", byte[].class,
                    new XC_MethodHook() {
                        @Override
                        protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                            super.beforeHookedMethod(param);
                            XposedBridge.log("***************************deserialdata*****************************");
                            byte[] bs = (byte[]) param.args[0];
                            //XposedBridge.log(String.format("before call [%s]", bytes2Hex(bs)));
                        }


                        @Override
                        protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                            super.afterHookedMethod(param);
                            byte[] bs = (byte[]) param.args[0];
                            //XposedBridge.log(String.format("after call [%s]", bytes2Hex(bs)));
                            XposedBridge.log(String.format("decode [%s]", new String(bs, "utf-8")));
                            int r = (int) param.getResult();
                            XposedBridge.log("return " + r);
                        }
                    });

            findAndHookMethod("com.netease.cloudmusic.utils.NeteaseMusicUtils",
                    lpparam.classLoader, "serialdata", String.class, String.class,
                    new XC_MethodHook() {
                        @Override
                        protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                            super.beforeHookedMethod(param);
                            XposedBridge.log("***************************serialdata*****************************");
                            String url = (String) param.args[0];
                            String data = (String) param.args[1];
                            XposedBridge.log(String.format("url:[%s], data:[%s]", url, data));
                        }
                    });
        }
    }
}

