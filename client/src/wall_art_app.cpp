#include <iostream>
#include <cstdint>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThreadPool>

#include "image_downloader.h"
#include "wall_art_app.h"

WallArtApp::WallArtApp(int argc, char **argv)
	: QApplication(argc, argv), artist_name("No artist loaded"), network_manager(this)
{
	artist_name.show();
}
void WallArtApp::fetch_url(const std::string &url){
	std::cout << "Network request for " << url << " launching\n";
	connect(&network_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(request_received(QNetworkReply*)));
	network_manager.get(QNetworkRequest(QUrl(QString::fromStdString(url))));
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
			std::cout << "success\n";
			const QString content_type = content_type_header.toString();
			if (content_type == "image/jpeg"){
				// The ImageDownloader thread needs the reply for longer, so don't flag it for deletion
				ImageDownloader *downloader = new ImageDownloader(reply);
				connect(downloader, SIGNAL(finished(ImageResult)), this, SLOT(image_downloaded(ImageResult)));
				QThreadPool::globalInstance()->start(downloader);
				return;
			}
			else {
				const QByteArray reply_text = reply->readAll();
				const QJsonDocument json = QJsonDocument::fromJson(reply_text);
				if (json.isArray()){
					const QJsonArray json_array = json.array();
					if (!json_array.empty()){
						const QJsonObject json_obj = json_array.at(0).toObject();
						auto artist_fnd = json_obj.constFind("artist");
						if (artist_fnd != json_obj.end()){
							artist_name.setText(artist_fnd->toString());
						}
					}
				}
				std::cout << "Reply: " << json.toJson().toStdString() << "\n";
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
	std::cout << "Saving to downloaded.jpg\n";
	result.image->save(QString::fromStdString("downloaded.jpg"), "JPEG");
	std::cout << "image saved\n";
}

#include "../include/moc_wall_art_app.cpp"

