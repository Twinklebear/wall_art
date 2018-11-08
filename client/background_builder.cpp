#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <chrono>

#include <QRect>
#include <QApplication>
#include <QDesktopWidget>
#include <QDir>

#include "background_builder.h"

template<typename T>
T clamp(T v, T min, T max) {
	return v < min ? min : v > max ? max : v;
}
float srgb_to_linear(float v){
	return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
}
float linear_to_srgb(float v){
	return v <= 0.0031308 ? 12.92 * v : 1.055 * std::pow(v, 1.0 / 2.4) - 0.055;
}
void resize_image(const QImage &img, QImage &out){
	assert(img.format() == QImage::Format_RGB32);
	assert(out.format() == QImage::Format_RGB32);
//#pragma omp parallel for
	for (int i = 0; i < out.width() * out.height(); ++i){
		int ox = i % out.width();
		int oy = i / out.width();
		float fx = img.width() * static_cast<float>(ox) / out.width();
		float fy = img.height() * static_cast<float>(oy) / out.height();
		int ix = static_cast<int>(fx);
		int iy = static_cast<int>(fy);
		fx = fx - ix;
		fy = fy - iy;
		uint8_t *out_scanline = out.scanLine(oy);
		// This is just nearest neighbor filtering with sRGB correction
		for (int c = 0; c < 4; ++c){
			// Convert to linear space to do the filtering
			int x = ix;
			int y = iy;
			const uint8_t *img_scanline = img.scanLine(y);
			const float v00 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			x = clamp(ix + 1, 0, img.width() - 1);
			const float v10 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			x = ix;
			y = clamp(iy + 1, 0, img.height() - 1);
			img_scanline = img.scanLine(y);
			const float v01 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			x = clamp(ix + 1, 0, img.width() - 1);
			const float v11 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			// Now do the bilinear filtering
			const float v = v00 * (1.0 - fx) * (1.0 - fy) + v10 * fx * (1.0 - fy) + v01 * (1.0 - fx) * fy
				+ v11 * fx * fy;

			// Convert back to sRGB to save result
			out_scanline[ox * 4 + c] = static_cast<unsigned char>(linear_to_srgb(v) * 255.0);
		}
	}
}
// Composite the image centered on the larger background image. It's expected that the background image
// is bigger in x and y than the image to be placed on top
void composite_background(const QImage &img, const QImage &back, QImage &out){
	assert(img.format() == QImage::Format_RGB32);
	assert(out.format() == QImage::Format_RGB32);
	assert(back.format() == QImage::Format_RGB32);
	int offset_x = (back.width() - img.width()) / 2;
	int offset_y = (back.height() - img.height()) / 2;
//#pragma omp parallel for
	for (int i = 0; i < back.width() * back.height(); ++i){
		const int ox = i % back.width();
		const int oy = i / back.width();
		const uint8_t *back_scanline = back.scanLine(oy);
		uint8_t *out_scanline = out.scanLine(oy);
		for (int c = 0; c < 4; ++c){
			if (ox < offset_x || ox >= back.width() - offset_x - 1 || oy < offset_y || oy >= back.height() - offset_y - 1){
				out_scanline[ox * 4 + c] = back_scanline[ox * 4 + c];
			}
			else {
				const int ix = ox - offset_x;
				const int iy = oy - offset_y;
				const uint8_t *img_scanline = img.scanLine(iy);
				out_scanline[ox * 4 + c] = img_scanline[ix * 4 + c];
			}
		}
	}
}
void crop_image(const QImage &img, QImage &out,
		const int start_x, const int end_x,
		const int start_y, const int end_y)
{
	assert(img.format() == QImage::Format_RGB32);
	assert(out.format() == QImage::Format_RGB32);
	// Copy each scanline into its place
//#pragma omp parallel for
	for (int y = start_y; y < end_y; ++y){
		const uint8_t *src = img.scanLine(y) + start_x * 4;
		uint8_t *dst = out.scanLine(y - start_y);
		const int count = (end_x - start_x) * 4;
		std::memcpy(dst, src, count);
	}
}

BackgroundBuilder::BackgroundBuilder(std::shared_ptr<QImage> original)
	: original(original)
{}
void BackgroundBuilder::run(){
	const QRect win_dim = QApplication::desktop()->screenGeometry();
	std::cout << "Desktop is: " << win_dim.width() << "x" << win_dim.height() << "\n";
	const float desktop_aspect = static_cast<float>(win_dim.width()) / win_dim.height();

	const float image_aspect = static_cast<float>(original->width()) / original->height();
	std::cout << "Desktop aspect = " << desktop_aspect << ", image = " << image_aspect << "\n";

	auto pipeline_start = std::chrono::high_resolution_clock::now();

	// TODO: We want to crop the image so it matches the desktop's aspect ratio
	// If the image is has a higher aspect ratio than the desktop we'll want to scale y
	// to fit and let x be croppped, otherwise we do the opposite
	int cropped_w = original->width();
	int cropped_h = original->height();
	if (desktop_aspect < image_aspect){
		cropped_w = cropped_h * desktop_aspect;
	} else {
		cropped_h = cropped_w * (1.0 / desktop_aspect);
	}

	std::cout << "Original dimensions: [" << original->width()
		<< ", " << original->height() << "]\n";
	std::cout << "Cropped dimensions: [" << cropped_w << ", " << cropped_h << "]\n";
	const int start_x = (original->width() - cropped_w) / 2;
	const int end_x = cropped_w + start_x;
	const int start_y = (original->height() - cropped_h) / 2;
	const int end_y = cropped_h + start_y;
	std::cout << "Crop start: [" << start_x << ", " << start_y
		<< "] end: [" << end_x << ", " << end_y << "]\n";

	QSharedPointer<QImage> background(new QImage(cropped_w, cropped_h, QImage::Format_RGB32));
	auto start = std::chrono::high_resolution_clock::now();
	crop_image(*original, *background, start_x, end_x, start_y, end_y);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "crop took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< "ms\n";

	background->save(QString("cropped_background.jpg"), "JPEG");

	std::cout << "done making background, Saving background to background.jpg\n";
	background->save(QString::fromStdString("background.jpg"), "JPEG", 80);
	background->save(QDir::tempPath() + QString::fromStdString("/background.jpg"), "JPEG", 80);
	std::cout << "background saved\n";
	emit finished(background);
}

#include "moc_background_builder.cpp"

