#include <iostream>
#include <cstdint>
#include <string>

#include <windows.h>

#include <QNetworkRequest>
#include <QUrl>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThreadPool>
#include <QDir>

#include "background_builder.h"
#include "image_downloader.h"
#include "wall_art_app.h"

WallArtApp::WallArtApp(int argc, char **argv)
	: QApplication(argc, argv), artist_name("No artist loaded"), update_background("Change Now"), network_manager(this)
{
	connect(&network_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(request_received(QNetworkReply*)));

	connect(&update_background, SIGNAL(clicked(bool)), this, SLOT(change_background()));
	layout.addWidget(&artist_name, 0, 0, Qt::AlignLeft | Qt::AlignTop);
	layout.addWidget(&painting_list, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
	layout.addWidget(&update_background, 0, 1, Qt::AlignRight | Qt::AlignTop);
	window.setLayout(&layout);
	window.show();
	// Launch a request to get information about images on the server
	fetch_url("http://localhost:5000/api/image");
}
void WallArtApp::fetch_url(const std::string &url){
	std::cout << "Network request for " << url << " launching\n";
	network_manager.get(QNetworkRequest(QUrl(QString::fromStdString(url))));
}
void WallArtApp::build_background(const int image_id){
	const std::string api_url = "http://localhost:5000/api/image/";
	// Launch image fetching tasks to get the original and blurred images
	fetch_url(api_url + std::to_string(image_id) + "/original");
	fetch_url(api_url + std::to_string(image_id) + "/blurred");
}
void WallArtApp::request_received(QNetworkReply *reply){
	if (reply->error() == QNetworkReply::NoError){
		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (status >= 200 && status < 300){
			// Validate that this is either text (JSON data listing) or an image we're downloading
			const QVariant content_type_header = reply->header(QNetworkRequest::ContentTypeHeader);
			if (!content_type_header.isValid() || !content_type_header.canConvert(QMetaType::QString)){
				std::cout << "Invalid content type header, text or jpeg expected\n";
				return;
			}
			std::cout << "successful fetch of " << reply->url().toString().toStdString() << "\n";
			const QString content_type = content_type_header.toString();
			if (content_type == "image/jpeg"){
				// The ImageDownloader thread needs the reply for longer, so don't flag it for deletion
				ImageDownloader *downloader = new ImageDownloader(reply);
				connect(downloader, SIGNAL(finished(ImageResult)), this, SLOT(image_downloaded(ImageResult)));
				QThreadPool::globalInstance()->start(downloader);
				return;
			}
			else {
				handle_api(reply);
			}
		}
		else {
			std::cout << "Redirect\n";
		}
	}
	else {
		std::cout << "Error\n";
	}
	reply->deleteLater();
}
void WallArtApp::image_downloaded(ImageResult result){
	std::cout << "Image with id " << result.id << " was fetched\n";
	if (!result.blurred){
		original = result.image;
	}
	else {
		blurred = result.image;
	}
	std::cout << "format = " << result.image->format() << "\n";
	// If we have both images we can now build the background
	if (original && blurred){
		std::cout << "I have downloaded both images, will now build background\n";
		BackgroundBuilder *builder = new BackgroundBuilder(original, blurred);
		connect(builder, SIGNAL(finished(QSharedPointer<QImage>)), this, SLOT(background_built(QSharedPointer<QImage>)));
		QThreadPool::globalInstance()->start(builder);
		original = nullptr;
		blurred = nullptr;
	}
}
void WallArtApp::background_built(QSharedPointer<QImage> background){
	std::cout << "WallArt has recieved the background image, setting to background\n";
	QString path = QDir::toNativeSeparators(QDir::tempPath() + "/background.png");
	std::cout << "path = " << path.toStdString() << "\n";
	std::wstring wstring = path.toStdWString();
	if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, &wstring[0], SPIF_UPDATEINIFILE)){
		std::cout << "Error setting background\n";
	}
	update_background.setEnabled(true);
}
void WallArtApp::change_background(){
	update_background.setEnabled(false);
	std::cout << "changing now\n";
	// Fetch artist info and the image
	fetch_url("http://localhost:5000/api/image/7");
	build_background(7);
}
void WallArtApp::handle_api(QNetworkReply *reply){
	// If we're updating our local copy of the list of paintings, artists, etc. on the server
	if (reply->url().path() == "/api/image") {
		std::cout << "handling image data update response\n";
		QStringList paintings;
		const QByteArray reply_text = reply->readAll();
		const QJsonDocument json = QJsonDocument::fromJson(reply_text);
		if (json.isArray()){
			const QJsonArray json_array = json.array();
			if (!json_array.empty()){
				for (const auto &val : json_array){
					const QJsonObject json_obj = val.toObject();
					auto title = json_obj.constFind("title");
					if (title != json_obj.end()){
						paintings.push_back(title->toString());
					}
				}
			}
		}
		painting_list.clear();
		painting_list.addItems(paintings);
	}
	// Handle case where we're querying info about a specific image
	else if (reply->url().path().startsWith("/api/image/")){
		const QByteArray reply_text = reply->readAll();
		const QJsonDocument json = QJsonDocument::fromJson(reply_text);
		if (json.isObject()){
			const QJsonObject json_obj = json.object();
			auto artist = json_obj.constFind("artist");
			if (artist != json_obj.end()){
				artist_name.setText(artist->toString());
			}
		}
	}
}

#include "../include/moc_wall_art_app.cpp"

