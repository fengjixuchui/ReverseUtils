package com.example.xposedtest;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;

import java.io.BufferedReader;
import java.io.File;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

import static de.robv.android.xposed.XposedHelpers.findAndHookMethod;

public class SignHookProc implements IXposedHookLoadPackage {

    @Override
    public void handleLoadPackage(final XC_LoadPackage.LoadPackageParam lpparam) {
        //app启动时调用
        String pkgName = lpparam.packageName;
        if (pkgName.equals("com.cmbchina.ccd.pluto.cmbActivity")) {
            XposedBridge.log("***************************load-xposed*****************************");
            //System.load("/data/local/tmp/libinject.so");
            findAndHookMethod("android.app.ApplicationPackageManager", lpparam.classLoader, "getPackageInfo", String.class, int.class,
            new XC_MethodHook() {
                @Override
                protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                    super.beforeHookedMethod(param);
                    String s = (String)param.args[0];
                    int flag = (int)param.args[1];
                    XposedBridge.log("***************************hook-before*****************************");
                    XposedBridge.log(String.format("pkg=%s,flag=0x%08x", s, flag));

                }
                @Override
                protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                    super.afterHookedMethod(param);
                    int flag = (int)param.args[1];
                    if (flag == PackageManager.GET_SIGNATURES) {
                        PackageInfo pkgInfo = (PackageInfo) param.getResult();
                        Signature[] s = pkgInfo.signatures;
                        Signature s0 = s[0];
                        XposedBridge.log(String.format("sign-code=%s", s0.toCharsString()));
                        String strSig = "30820242308201ab02041e294983300d06092a864886f70d010105050030673110300e06035504031307636d626c6966653110300e060355040b1307556e6b6e6f776e3110300e060355040a1307556e6b6e6f776e3110300e06035504071307556e6b6e6f776e3110300e06035504081307556e6b6e6f776e310b300906035504061302434e3020170d3132313131393136303030305a180f32313030313131393136303030305a30673110300e06035504031307636d626c6966653110300e060355040b1307556e6b6e6f776e3110300e060355040a1307556e6b6e6f776e3110300e06035504071307556e6b6e6f776e3110300e06035504081307556e6b6e6f776e310b300906035504061302434e30819f300d06092a864886f70d010101050003818d0030818902818100973f894b8980fde70ad5d24fb679ac2a9e44c0ed6902b75ecd68119327305849f14a7ab0fae43d2d4d72967b91a6493ffe7247ce4a785740ad8c9397aebce10026a1e45bcdd6217071e5664091e9d4be6d1de32eef9db0233ed20398401976c20a5a1544fea7d74a2d8f86ed34c44359e2e5d63e20039b7ef018c476b01a7ae30203010001300d06092a864886f70d0101050500038181003b318d4e443d67e2fc9d89bba05c78bd0957304a4e3d0c0cb0ccb1fc87dea915c1c34dd992d7697e629362229d5737172b6fa6dac34a9f6c287cc1a37df2bef8e12d3b453ecd5359aeb455a526c0e0e7140b8e9e7bacd2f6f46326ed3fbdc4789447ddd1b0708d7ca5f480bad7a7865e07116747999342c5f7cf43e72a2375b2";
                        Signature fake = new Signature(strSig);
                        s[0] = fake;
                        XposedBridge.log(String.format("sign-code force=%s", strSig));

                        XposedBridge.log("***************************hook-after*****************************");
                    }
                }
            });

        }

    }
}

