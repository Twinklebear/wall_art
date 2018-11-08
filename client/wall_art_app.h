#ifndef WALL_ART_APP_H
#define WALL_ART_APP_H

#include <memory>
#include <string>
#include <vector>
#include <random>

#include <QApplication>
#include <QSharedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLabel>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>

#include "image_data.h"
#include "image_downloader.h"

class WallArtApp : public QApplication {
	Q_OBJECT

	QWidget window;
	QGridLayout layout;
	QLabel artist_name;
	QListWidget painting_list;
	QPushButton update_background;

	QNetworkAccessManager network_manager;
	// Timer to alert us when to change the background
	QTimer timer;
	// Should we have support for more than one running background generation job?
	// I'll have the button gray out while we're doing the processing but could
	// we get multiple click events? I guess we could keep a bool and to track
	// if we're processing a background switch
	std::shared_ptr<QImage> original;
	std::vector<ImageData> images;
	std::mt19937_64 rng;

public:
	WallArtApp(int argc, char **argv);
	void fetch_url(const std::string &url);
	// Fetch the image with the id and crop it to create the wallpaper
	void build_background(const int image_id);

public slots:
	void request_received(QNetworkReply *reply);
	void image_downloaded(ImageResult result);
	void background_built(QSharedPointer<QImage> background);
	void change_background();

private:
	// Handle a JSON API endpoint response and update UI with the resulting info
	void handle_api(QNetworkReply *reply);
};

#endif

