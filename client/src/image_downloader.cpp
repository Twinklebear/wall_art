#include <iostream>
#include <QNetworkReply>
#include <QImage>

#include "image_downloader.h"

ImageResult::ImageResult() : id(-1), blurred(false), image(nullptr){}
ImageResult::ImageResult(int id, bool blurred, std::shared_ptr<QImage> image) : id(id), blurred(blurred), image(image){}

ImageDownloader::ImageDownloader(QNetworkReply *reply) : reply(reply){}
void ImageDownloader::run(){
	const QString img_path = reply->url().path();
	// The image api path is /api/image/<image_id>/(original|blurred)
	const auto path_segments = img_path.splitRef('/', QString::SkipEmptyParts);
	bool parsed_id = false;
	const int img_id = path_segments[2].toInt(&parsed_id);
	if (!parsed_id){
		std::cout << "Invalid image API endpoint?\n";
		return;
	}
	const bool blurred = path_segments.back() == "blurred";

	std::cout << "downloader fetching: " << reply->url().toString().toStdString() << "\n"
		<< "path = " << img_path.toStdString() << "\n";
	const QVariant content_length_header = reply->header(QNetworkRequest::ContentLengthHeader);
	if (!content_length_header.isValid() || !content_length_header.canConvert(QMetaType::Int)){
		std::cout << "Error: JPEG missing valid content length header\n";
		return;
	}
	int content_len = content_length_header.toInt();
	std::cout << "got content length, about to read all\n";
	const QByteArray reply_image = reply->readAll();
	auto img = std::make_shared<QImage>();
	img->loadFromData(reinterpret_cast<const uint8_t*>(reply_image.data()), content_len, "JPEG");
	std::cout << "Downloaded image of dimension " << img->width() << "x" << img->height() << ", saving to downloaded.jpg\n";
	reply->deleteLater();
	emit finished(ImageResult{img_id, blurred, img});
}

#include "../include/moc_image_downloader.cpp"

