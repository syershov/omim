#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../util/crashlytics.h"

#include "../platform/Platform.hpp"

crashlytics_context_t * g_crashlytics;

extern "C"
{
  // static void nativePreparePlatform(String settingsPath);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativePreparePlatform(JNIEnv * env, jclass clazz, jstring settingsPath)
  {
    android::Platform::Instance().SetSettingsDir(jni::ToNativeString(env, settingsPath));
  }

  // void nativeInitPlatform(String apkPath, String storagePath, String tmpPath, String obbGooglePath, String flavorName, String buildType, boolean isTablet);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitPlatform(JNIEnv * env, jobject thiz, jstring apkPath, jstring storagePath, jstring tmpPath,
                                                             jstring obbGooglePath, jstring flavorName, jstring buildType, jboolean isTablet)
  {
    android::Platform::Instance().Initialize(env, thiz, apkPath, storagePath, tmpPath, obbGooglePath, flavorName, buildType, isTablet);
  }

  // static void nativeInitFramework();
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitFramework(JNIEnv * env, jclass clazz)
  {
    if (!g_framework)
      g_framework = new android::Framework();
  }

  // static void nativeProcessFunctor(long functorPointer);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeProcessFunctor(JNIEnv * env, jclass clazz, jlong functorPointer)
  {
    android::Platform::Instance().ProcessFunctor(functorPointer);
  }

  // static void nativeAddLocalization(String name, String value);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeAddLocalization(JNIEnv * env, jclass clazz, jstring name, jstring value)
  {
    g_framework->AddString(jni::ToNativeString(env, name),
                           jni::ToNativeString(env, value));
  }

  // @UiThread
  // static void nativeInitCrashlytics();
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitCrashlytics(JNIEnv * env, jclass clazz)
  {
    ASSERT(!g_crashlytics, ());
    g_crashlytics = crashlytics_init();
  }
}
