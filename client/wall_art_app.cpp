#include <iostream>
#include <cstdint>
#include <string>

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
#include "set_background.h"
#include "wall_art_app.h"

WallArtApp::WallArtApp(int argc, char **argv)
	: QApplication(argc, argv), artist_name("No artist loaded"), update_background("Change Now"),
	network_manager(this), timer(this), rng(std::random_device()())
{
	connect(&network_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(request_received(QNetworkReply*)));
	connect(&timer, SIGNAL(timeout()), this, SLOT(change_background()));
	// Update every few seconds
	// TODO: Change to actual interval that isn't insanely short
	//timer.start(20000);

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
	// Launch image fetching tasks to get the original images
	fetch_url(api_url + std::to_string(image_id) + "/original");
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
	original = result.image;
	std::cout << "format = " << result.image->format() << "\n";
	// If we have both images we can now build the background
	if (original){
		std::cout << "I have downloaded both images, will now build background\n";
		BackgroundBuilder *builder = new BackgroundBuilder(original);
		connect(builder, SIGNAL(finished(QSharedPointer<QImage>)),
				this, SLOT(background_built(QSharedPointer<QImage>)));
		QThreadPool::globalInstance()->start(builder);
		original = nullptr;
	}
}
void WallArtApp::background_built(QSharedPointer<QImage> background){
	std::cout << "WallArt has recieved the background image, setting to background\n";
	QString path = QDir::toNativeSeparators(QDir::tempPath() + "/background.jpg");
	std::cout << "path = " << path.toStdString() << "\n";
	if (!set_background(path)){
		std::cout << "Error setting background\n";
	}
	// Re-start the timer and re-enable the button
	update_background.setEnabled(true);
	//timer.start(20000);
}
void WallArtApp::change_background(){
	// Stop timer and disable button. Timer is stopped so we don't immediately
	// change again if the user clicked chang and the button is disabled so the user
	// doesn't launch another change request while we're changing
	// TODO: Maybe use a mutex or atomic bool or something?
	timer.stop();
	update_background.setEnabled(false);
	std::cout << "changing now\n";
	std::uniform_int_distribution<size_t> distrib{0, images.size() - 1};
	const auto &img = images[distrib(rng)];
	std::cout << "Selected " << img.title.toStdString() << "\n";
	// Fetch artist info and the image
	fetch_url("http://localhost:5000/api/image/" + std::to_string(img.id));
	build_background(img.id);
}
void WallArtApp::handle_api(QNetworkReply *reply){
	// If we're updating our local copy of the list of paintings, artists, etc. on the server
	if (reply->url().path() == "/api/image") {
		std::cout << "handling image data update response\n";
		images.clear();
		QStringList paintings;
		const QByteArray reply_text = reply->readAll();
		const QJsonDocument json = QJsonDocument::fromJson(reply_text);
		if (json.isArray()){
			const QJsonArray json_array = json.array();
			if (!json_array.empty()){
				for (const auto &val : json_array){
					const QJsonObject json_obj = val.toObject();
					images.emplace_back(json_obj);
					paintings.push_back(images.back().title);
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
			const ImageData img{json_obj};
			// TODO: Proper display of lists of artists
			artist_name.setText(img.artist);
		}
	}
}

#include "moc_wall_art_app.cpp"

