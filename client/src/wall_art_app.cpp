#include <iostream>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "wall_art_app.h"

WallArtApp::WallArtApp(int argc, char **argv)
	: QApplication(argc, argv), artist_name("No artist loaded"), network_manager(this)
{
	artist_name.show();
}
void WallArtApp::fetch_url(const std::string &url){
	std::cout << "Network request for " << url << " launching\n";
	connect(&network_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestReceived(QNetworkReply*)));
	network_manager.get(QNetworkRequest(QUrl(QString::fromStdString(url))));
}
void WallArtApp::requestReceived(QNetworkReply *reply){
	reply->deleteLater();
	if (reply->error() == QNetworkReply::NoError){
		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (status >= 200 && status < 300){
			std::cout << "success\n";
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
		else {
			std::cout << "Redirect\n";
		}
	}
	else {
		std::cout << "Error\n";
	}
}

#include "../include/moc_wall_art_app.cpp"

