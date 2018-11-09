#include <string>
#include <windows.h>
#include "set_background.h"

bool set_background(const QString &path){
	std::wstring wstring = path.toStdWString();
	return SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, &wstring[0], SPIF_UPDATEINIFILE);
}

