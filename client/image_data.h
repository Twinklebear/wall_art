#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <QString>
#include <QStringList>
#include <QJsonObject>

// ImageData stores information about an image on the server, such as its
// id, artists, title, etc.
struct ImageData {
	QString artist, title, work_type;
	bool has_nudity;
	int id;

	// Parse the ImageData from the JSON response (eg. from the image api endpoint)
	ImageData(const QJsonObject &json);
};

#endif

