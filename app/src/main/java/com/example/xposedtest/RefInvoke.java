package com.example.xposedtest;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * =============================================================================
 * Copyright (c) 2017 yuxin All rights reserved.
 * Packname com.jju.yuxin.reforceapk
 * Created by yuxin.
 * Created time 2017/6/18 0018 下午 5:08.
 * Version   1.0;
 * Describe :
 * History:
 * ==============================================================================
 */

public class RefInvoke {
    /**
     * 反射执行类的静态函数(public)
     * @param class_name	类名
     * @param method_name	函数名
     * @param pareTyple		函数的参数类型
     * @param pareVaules	调用函数时传入的参数
     * @return
     */
    public static  Object invokeStaticMethod(String class_name, String method_name, Class[] pareTyple, Object[] pareVaules){

        try {
            Class obj_class = Class.forName(class_name);
            Method method = obj_class.getDeclaredMethod(method_name,pareTyple);
            method.setAccessible(true);
            return method.invoke(null, pareVaules);
        } catch (SecurityException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        }  catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (NoSuchMethodException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (ClassNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        }

    }

    /**
     * 反射创建对象（public）
     * @param class_name
     * @param pareTyple
     * @param pareVaules
     * @return
     */
    public static  Object newInstance(String class_name, Class[] pareTyple, Object[] pareVaules){

        try {
            Class obj_class = Class.forName(class_name);
            Constructor constructor = obj_class.getDeclaredConstructor(pareTyple);
            constructor.setAccessible(true);
            return constructor.newInstance(pareVaules);
        }
        catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * 反射执行类的函数（public）
     * @param method_name
     * @param obj
     * @param pareTyple
     * @param pareVaules
     * @return
     */
    public static  Object invokeMethod(String className, String method_name, Object obj ,Class[] pareTyple, Object[] pareVaules){

        try {
            Class obj_class = Class.forName(className);
            Method method = obj_class.getDeclaredMethod(method_name, pareTyple);
            method.setAccessible(true);
            return method.invoke(obj, pareVaules);
        }
        catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * 反射得到类的属性（包括私有和保护）
     * @param class_name
     * @param obj
     * @param filedName
     * @return
     */
    public static Object getFieldOjbect(String class_name,Object obj, String filedName){
        try {
            Class obj_class = Class.forName(class_name);
            Field field = obj_class.getDeclaredField(filedName);
            field.setAccessible(true);
            return field.get(obj);
        } catch (SecurityException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (NoSuchFieldException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (ClassNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        }

    }

    /**
     * 反射得到类的静态属性（包括私有和保护）
     * @param class_name
     * @param filedName
     * @return
     */
    public static Object getStaticFieldOjbect(String class_name, String filedName){

        try {
            Class obj_class = Class.forName(class_name);
            Field field = obj_class.getDeclaredField(filedName);
            field.setAccessible(true);
            return field.get(null);
        } catch (SecurityException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (NoSuchFieldException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (ClassNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        }

    }

    /**
     * 设置类的属性（包括私有和保护）
     * @param classname
     * @param filedName
     * @param obj
     * @param filedVaule
     */
    public static void setFieldOjbect(String classname, String filedName, Object obj, Object filedVaule){
        try {
            Class obj_class = Class.forName(classname);
            Field field = obj_class.getDeclaredField(filedName);
            field.setAccessible(true);
            field.set(obj, filedVaule);
        } catch (SecurityException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (NoSuchFieldException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (ClassNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

    /**
     * 设置类的静态属性（包括私有和保护）
     * @param class_name
     * @param filedName
     * @param filedVaule
     */
    public static void setStaticOjbect(String class_name, String filedName, Object filedVaule){
        try {
            Class obj_class = Class.forName(class_name);
            Field field = obj_class.getDeclaredField(filedName);
            field.setAccessible(true);
            field.set(null, filedVaule);
        } catch (SecurityException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (NoSuchFieldException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        } catch (ClassNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

}
