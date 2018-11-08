#include <QJsonArray>

#include "image_data.h"

ImageData::ImageData(const QJsonObject &json){
	for (auto j = json.begin(); j != json.end(); ++j){
		if (j.key() == "artists" && j.value().isArray()){
			const QJsonArray a = j.value().toArray();
			for (const auto &s : a){
				artists.push_back(s.toString());
			}
		}
		else if (j.key() == "culture" && j.value().isString()){
			culture = j.value().toString();
		}
		else if (j.key() == "title" && j.value().isString()){
			title = j.value().toString();
		}
		else if (j.key() == "work_type" && j.value().isString()){
			work_type = j.value().toString();
		}
		else if (j.key() == "has_nudity" && j.value().isBool()){
			has_nudity = j.value().toBool();
		}
		else if (j.key() == "id"){
			id = j.value().toInt();
		}
	}
}
