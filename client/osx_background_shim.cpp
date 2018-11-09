#include <iostream>
#include <QByteArray>
#include "set_background.h"

extern bool osx_set_background(const char *path);

bool set_background(const QString &path) {
	const QByteArray pathutf8 = path.toUtf8();
	return osx_set_background(pathutf8.constData());
}

