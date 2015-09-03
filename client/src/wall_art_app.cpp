#include <iostream>
#include "wall_art_app.h"

WallArtApp::WallArtApp(int argc, char **argv) : QApplication(argc, argv){}
void WallArtApp::fetch_url(const std::string &url){
	// TODO: Manager should probably be a member
	std::cout << "Network request for " << url << " launching\n";
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestReceived(QNetworkReply*)));
	manager->get(QNetworkRequest(QUrl(QString::fromStdString(url))));
}
void WallArtApp::requestReceived(QNetworkReply *reply){
	reply->deleteLater();
	if (reply->error() == QNetworkReply::NoError){
		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (status >= 200 && status < 300){
			std::cout << "success\n";
			QString replyText = reply->readAll();
			std::cout << "Reply: " << replyText.toStdString() << "\n";
		}
		else {
			std::cout << "Redirect\n";
		}
	}
	else {
		std::cout << "Error\n";
	}
	reply->manager()->deleteLater();
	quit();
}

#include "../include/moc_wall_art_app.cpp"

