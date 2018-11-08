#ifndef BACKGROUND_BUILDER_H
#define BACKGROUND_BUILDER_H

#include <memory>

#include <QSharedPointer>
#include <QRunnable>
#include <QImage>

// The background builder takes the original image and crops
// it to fit nicely on the user's screen
class BackgroundBuilder : public QObject, public QRunnable {
	Q_OBJECT

	std::shared_ptr<QImage> original;

public:
	BackgroundBuilder(std::shared_ptr<QImage> original);
	void run() override;

signals:
	void finished(QSharedPointer<QImage> background);
};

Q_DECLARE_METATYPE(QSharedPointer<QImage>);

#endif

