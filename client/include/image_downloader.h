#ifndef IMAGE_DOWNLOADER_H
#define IMAGE_DOWNLOADER_H

#include <memory>

#include <QNetworkReply>
#include <QRunnable>
#include <QImage>

// Image data stores the downloaded image and its id
struct ImageResult {
	int id;
	bool blurred;
	std::shared_ptr<QImage> image;

	ImageResult();
	ImageResult(int id, bool blurred, std::shared_ptr<QImage> image);
};
Q_DECLARE_METATYPE(ImageResult);

// The ImageDownloader is used to move the task of reading the image
// data from the reply and building the QImage to separate thread to
// prevent hanging the UI
class ImageDownloader : public QObject, public QRunnable {
	Q_OBJECT

	QNetworkReply *reply;

public:
	ImageDownloader(QNetworkReply *reply);
	void run() override;

signals:
	void finished(ImageResult result);
};

#endif

