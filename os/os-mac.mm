#include <Cocoa/Cocoa.h>

#include "os-mac.h"

// Fix for new Yosemite fileID URL style
// See the ticket here: https://bugreports.qt-project.org/browse/QTBUG-40449
QString get_good_url_from_fileid_url(const QString& url) {
    NSString* idURL = [NSString stringWithCString:url.toUtf8().data() encoding:NSUTF8StringEncoding];
    NSString* path = [[[NSURL URLWithString:idURL] filePathURL] path];
    return QString::fromUtf8([path UTF8String], [path lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
}
