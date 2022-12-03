/*
//android native function
//pngchs@qq.com
*/

#include "../etc/LOIO.h"
#include <unistd.h>
#include <jni.h>
#include <string>


enum ANDROID_SET{
    SET_READ_DIR = 1,
    SET_WRITE_DIR = 2,
    SET_SAVE_DIR = 3
};

std::string JStringToString(JNIEnv* env, jstring *js){
    std::string  s ;
    int len = (*env).GetStringUTFLength(*js) ;
    /*
    __android_log_print(ANDROID_LOG_INFO, "LONS", "len is:%d",len);
    len = (*env).GetStringUTFLength(*js) ;
    __android_log_print(ANDROID_LOG_INFO, "LONS", "len is:%d",len);
     */

    if(len > 0){
        char *buf = new char[len + 1] ;
        memset(buf,0,len+1);
        memcpy(buf, (*env).GetStringUTFChars(*js,0), len) ;
        s.assign(buf) ;
        delete [] buf ;
    }

    return s;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_xc_lons_MainActivity_LonsSetStringFromJNI(
        JNIEnv* env, jobject,
        jint param, jstring value) {

    std::string s_value = JStringToString(env,&value) ;
    //__android_log_print(ANDROID_LOG_INFO, "LONS", "Param is:%s",s_value.c_str());

    switch (param) {
        case SET_READ_DIR:
            LOIO::ioReadDir = s_value ;
            //__android_log_print(ANDROID_LOG_INFO, "LONS", "readDir is:%s",s_value.c_str());
            break;
        case SET_WRITE_DIR:
            LOIO::ioWriteDir = s_value;
            //__android_log_print(ANDROID_LOG_INFO, "LONS", "writeDir is:%s",s_value.c_str());
            break;
        case SET_SAVE_DIR:
            LOIO::ioSaveDir = s_value;
            break;
    }

    return 0 ;
}