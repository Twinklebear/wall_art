#ifndef WALL_ART_APP_H
#define WALL_ART_APP_H

#include <memory>
#include <string>

#include <QApplication>
#include <QNetworkAccessManager>
#include <QLabel>

#include "image_downloader.h"

class WallArtApp : public QApplication {
	Q_OBJECT

	QLabel artist_name;
	QNetworkAccessManager network_manager;

public:
	WallArtApp(int argc, char **argv);
	void fetch_url(const std::string &url);

public slots:
	void request_received(QNetworkReply *reply);
	void image_downloaded(ImageResult result);
};

#endif

