#ifndef WALL_ART_APP_H
#define WALL_ART_APP_H

#include <string>
#include <QApplication>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QUrl>

class WallArtApp : public QApplication {
	Q_OBJECT
public:
	WallArtApp(int argc, char **argv);

	void fetch_url(const std::string &url);
public slots:
	void requestReceived(QNetworkReply *reply);
};

#include "wall_art_app.moc"

#endif

