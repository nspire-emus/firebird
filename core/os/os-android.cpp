#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QAndroidActivityResultReceiver>
#include <QScopeGuard>

#include "os.h"

// A handler to open android content:// URLs.
// Based on code by Florin9doi: https://github.com/nspire-emus/firebird/pull/94/files
FILE *fopen_utf8(const char *path, const char *mode)
{
    const char pattern[] = "content:";
    if(strncmp(pattern, path, sizeof(pattern)-1) != 0)
        return fopen(path, mode);

    QString android_mode; // Why did they have to NIH...
    if(strcmp(mode, "rb") == 0)
        android_mode = QStringLiteral("r");
    else if(strcmp(mode, "r+b") == 0)
        android_mode = QStringLiteral("rw");
    else if(strcmp(mode, "wb") == 0)
        android_mode = QStringLiteral("rwt");
    else
        return nullptr;

    QAndroidJniObject jpath = QAndroidJniObject::fromString(QString::fromUtf8(path));
    QAndroidJniObject jmode = QAndroidJniObject::fromString(android_mode);
    QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
                "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
                jpath.object<jstring>());

    QAndroidJniObject contentResolver = QtAndroid::androidActivity()
            .callObjectMethod("getContentResolver",
                              "()Landroid/content/ContentResolver;");

    // Call contentResolver.takePersistableUriPermission as we save the URI
    int permflags = 1; // Intent.FLAG_GRANT_READ_URI_PERMISSION
    if(android_mode.contains(QLatin1Char('w')))
        permflags |= 2; // Intent.FLAG_GRANT_WRITE_URI_PERMISSION
    contentResolver.callMethod<void>("takePersistableUriPermission",
                                     "(Landroid/net/Uri;I)V", uri.object<jobject>(), permflags);

    QAndroidJniEnvironment env;

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    QAndroidJniObject parcelFileDescriptor = contentResolver
            .callObjectMethod("openFileDescriptor",
                              "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
                              uri.object<jobject>(), jmode.object<jobject>());

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // The file descriptor needs to be duplicated as
    QAndroidJniObject parcelFileDescriptorDup = parcelFileDescriptor
            .callObjectMethod("dup",
                              "()Landroid/os/ParcelFileDescriptor;");

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    int fd = parcelFileDescriptorDup.callMethod<jint>("detachFd", "()I");
    if(fd < 0)
        return nullptr;

    return fdopen(fd, mode);
}

char *android_basename(const char *path)
{
    const char pattern[] = "content:";
    if(strncmp(pattern, path, sizeof(pattern)-1) != 0)
        return nullptr;

    QAndroidJniObject jpath = QAndroidJniObject::fromString(QString::fromUtf8(path));
    QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
                "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
                jpath.object<jstring>());

    QAndroidJniObject contentResolver = QtAndroid::androidActivity()
            .callObjectMethod("getContentResolver",
                              "()Landroid/content/ContentResolver;");

    QAndroidJniEnvironment env;
    QAndroidJniObject col = QAndroidJniObject::getStaticObjectField("android/provider/OpenableColumns", "DISPLAY_NAME", "Ljava/lang/String;");
    QAndroidJniObject proj = env->NewObjectArray(1, env->FindClass("java/lang/String"), col.object<jstring>());

    QAndroidJniObject cursor = contentResolver.callObjectMethod("query", "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;", uri.object<jobject>(), proj.object<jobject>(), nullptr, nullptr);
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    if(!cursor.isValid())
        return nullptr;

    auto closeCursor = qScopeGuard([&] { cursor.callMethod<void>("close", "()V"); });

    bool hasContent = cursor.callMethod<jboolean>("moveToFirst", "()Z");
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    if(!hasContent)
        return nullptr;

    QAndroidJniObject name = cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", 0);
    if (!name.isValid())
        return nullptr;

    return strdup(name.toString().toUtf8().data());
}
