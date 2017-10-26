#include <jni.h>

/* Header for class GraoImpl */

//export functions declarations
#ifndef _Included_GraoImpl
#define _Included_GraoImpl
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     GraoImpl
 * Method:    getPersonInfoJson
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_grao_integration_GraoImpl_getPersonInfoJson
  (JNIEnv *, jobject,  jstring);

/*
 * Class:     GraoImpl
 * Method:    getPersonInfoXml
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_grao_integration_GraoImpl_getPersonInfoXml
  (JNIEnv *, jobject,  jstring);

/*
 * Class:     GraoImpl
 * Method:    initializeCom
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_initializeCom
	(JNIEnv *, jobject);

/*
 * Class:     GraoImpl
 * Method:    unInitializeCom
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_unInitializeCom
  (JNIEnv *, jobject);


/***
Debug functions normally not defined in the class GraoImpl
*/
JNIEXPORT void JNICALL Java__grao_integration_showConsole(
	JNIEnv *env, jobject);


/**
Java export that sets the debug flags in grao dll
*/
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_setFlags(
	JNIEnv *env, jobject,  jint flags);

/**
Prints person information in console
*/
JNIEXPORT void JNICALL printPersonInfo(WCHAR *idn);

#ifdef __cplusplus
}
#endif
#endif
