#ifndef BACKGROUND_BUILDER_H
#define BACKGROUND_BUILDER_H

#include <memory>

#include <QSharedPointer>
#include <QRunnable>
#include <QImage>

// The background builder takes two images and composes them into
// the wallpaper we're generating. The background is the blurred image
// and the original image will be placed centered on top of it
class BackgroundBuilder : public QObject, public QRunnable {
	Q_OBJECT

	std::shared_ptr<QImage> original, blurred;

public:
	BackgroundBuilder(std::shared_ptr<QImage> original, std::shared_ptr<QImage> blurred);
	void run() override;

signals:
	void finished(QSharedPointer<QImage> background);
};

Q_DECLARE_METATYPE(QSharedPointer<QImage>);

#endif

