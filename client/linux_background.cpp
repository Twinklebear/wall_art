#include <iostream>
#include <unordered_map>
#include <cstdio>
#include <memory>
#include <QProcess>
#include "linux_background.h"

const static std::unordered_map<std::string, std::string> managers{
	{"gsettings", "set org.gnome.desktop.background picture-uri file://%s"},
	{"gconftool-2", "--set /desktop/gnome/background/picture_filename --type string %s"},
	{"dcop", "kdesktop KBackgroundIface setWallpaper %s 1"},
	{"dconf", "write /org/mate/desktop/background/picture-filename \"%s\""},
	{"feh", "--bg-scale %s"},
	{"pcmanfm", "-w %s"}
	// TODO: XFCE4
};

bool set_background(const QString &path) {
	// There's a lot of options on Linux for the window manager, and
	// since some draw over the X11 root window we can't change that to set
	// the background. So we take the same approach as https://github.com/micooz/wallpaper
	// and just try until we succeed.
	const QByteArray pathutf8 = path.toUtf8();
	for (const auto &wm : managers) {
		QByteArray arg(wm.second.size() + path.size(), 'x');
		std::snprintf(arg.data(), arg.size(), wm.second.c_str(), pathutf8.constData());
		QStringList arglist = QString::fromUtf8(arg).split(' ');

		std::shared_ptr<QProcess> proc = std::make_shared<QProcess>();
		proc->start(wm.first.c_str(), arglist);
		proc->waitForFinished();

		if (proc->exitCode() == 0) {
			return true;
		}
	}
	return false;
}

