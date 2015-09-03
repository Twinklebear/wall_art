#ifndef WALL_ART_APP_H
#define WALL_ART_APP_H

#include <string>

#include <QApplication>
#include <QNetworkAccessManager>
#include <QLabel>

class WallArtApp : public QApplication {
	Q_OBJECT

	QLabel artist_name;
	QNetworkAccessManager network_manager;

public:
	WallArtApp(int argc, char **argv);
	void fetch_url(const std::string &url);

public slots:
	void requestReceived(QNetworkReply *reply);
};

#include "wall_art_app.moc"

#endif

