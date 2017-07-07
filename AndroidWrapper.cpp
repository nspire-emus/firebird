#include "AndroidWrapper.h"

AndroidWrapper::AndroidWrapper(QObject *parent) : QObject(parent) {}

int AndroidWrapper::is_content_provider_supported() {
    return (QtAndroid::androidSdkVersion() >= API__STORAGE_ACCESS_FRAMEWORK);
}

void AndroidWrapper::handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data) {
    int RESULT_OK = QAndroidJniObject::getStaticField<int>("android/app/Activity", "RESULT_OK");
    if (receiverRequestCode == SDCARD_DOCUMENT_REQUEST && resultCode == RESULT_OK && data != NULL) {
        QAndroidJniObject uri = data.callObjectMethod("getData", "()Landroid/net/Uri;");

        jint FLAG_GRANT_READ_URI_PERMISSION = QAndroidJniObject::getStaticField<jint>(
                    "android.content.Intent", "FLAG_GRANT_READ_URI_PERMISSION");
        jint FLAG_GRANT_WRITE_URI_PERMISSION = QAndroidJniObject::getStaticField<jint>(
                    "android.content.Intent", "FLAG_GRANT_WRITE_URI_PERMISSION");

        int takeFlags = data.callMethod<jint>("getFlags", "()I");
        takeFlags &= ( FLAG_GRANT_READ_URI_PERMISSION | FLAG_GRANT_WRITE_URI_PERMISSION );
        QAndroidJniObject contentResolver = QtAndroid::androidActivity()
                .callObjectMethod("getContentResolver",
                "()Landroid/content/ContentResolver;");
        contentResolver.callMethod<void>("takePersistableUriPermission","(Landroid/net/Uri;I)V",
                uri.object<jobject>(), takeFlags);

        emit filePicked(uri.toString());
    }
}

void AndroidWrapper::selectFile(bool selectExisting) {
    QAndroidJniObject intent("android/content/Intent");
    QAndroidJniObject setAction = NULL;
    if (selectExisting) {
        setAction = QAndroidJniObject::fromString(QLatin1String("android.intent.action.OPEN_DOCUMENT"));
    } else {
        setAction = QAndroidJniObject::fromString(QLatin1String("android.intent.action.CREATE_DOCUMENT"));
    }
    QAndroidJniObject setType = QAndroidJniObject::fromString(QLatin1String("*/*"));
    QAndroidJniObject setExtra = QAndroidJniObject::fromString(QLatin1String("android.content.extra.SHOW_ADVANCED"));

    if (setAction.isValid() && intent.isValid()) {
        intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", setAction.object<jstring>());
        intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", setType.object<jstring>());
        intent.callObjectMethod("putExtra","(Ljava/lang/String;Z)Landroid/content/Intent;",
                setExtra.object<jstring>(), jboolean(true));
        QtAndroid::startActivity(intent.object<jobject>(), SDCARD_DOCUMENT_REQUEST, this);
    }
}

/* do some C functions which can be called from anywhere */

extern "C"
int is_android_provider_file(const char *path) {
    const char pattern[] = "content:";
    return ((path != NULL) && (strncmp(pattern, path, sizeof(pattern)-1) == 0));
}

extern "C"
int android_get_fd_for_uri(const char *path, const char *mode) {
    QAndroidJniObject jpath = QAndroidJniObject::fromString(QLatin1String(path));
    QAndroidJniObject jmode = QAndroidJniObject::fromString(QLatin1String(mode));
    QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod("android/net/Uri",
            "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jpath.object<jstring>());

    QAndroidJniObject contentResolver = QtAndroid::androidActivity()
            .callObjectMethod("getContentResolver",
            "()Landroid/content/ContentResolver;");

    QAndroidJniObject parcelFileDescriptor = contentResolver
            .callObjectMethod("openFileDescriptor",
            "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
            uri.object<jobject>(), jmode.object<jobject>());

    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return -1;
    }

    return parcelFileDescriptor.callMethod<jint>("getFd", "()I");
}
