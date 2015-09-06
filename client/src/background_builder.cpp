#include <cassert>
#include <iostream>
#include <chrono>
#include <QRect>
#include <QApplication>
#include <QDesktopWidget>

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

// Note: this assumes RBG32 format
void resize_image(const QImage &img, QImage &out){
	assert(img.format() == QImage::Format_RGB32);
	assert(out.format() == QImage::Format_RGB32);
#pragma omp parallel for
	for (int i = 0; i < out.width() * out.height(); ++i){
		int ox = i % out.width();
		int oy = i / out.width();
		float fx = img.width() * static_cast<float>(ox) / out.width();
		float fy = img.height() * static_cast<float>(oy) / out.height();
		int ix = static_cast<int>(fx);
		int iy = static_cast<int>(fy);
		fx = fx - ix;
		fy = fy - iy;
		const uint8_t *img_scanline = img.scanLine(iy);
		uint8_t *out_scanline = out.scanLine(oy);
		// This is just nearest neighbor filtering with sRGB correction
		for (int c = 0; c < 4; ++c){
			// Convert to linear space to do the filtering
			int x = ix;
			int y = iy;
			float v00 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			x = clamp(ix + 1, 0, img.width() - 1);
			float v10 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			x = ix;
			y = clamp(iy + 1, 0, img.height() - 1);
			float v01 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			x = clamp(ix + 1, 0, img.width() - 1);
			float v11 = srgb_to_linear(static_cast<float>(img_scanline[x * 4 + c]) / 255.0);

			// Now do the bilinear filtering
			float v = v00 * (1.0 - fx) * (1.0 - fy) + v10 * fx * (1.0 - fy) + v01 * (1.0 - fx) * fy
				+ v11 * fx * fy;

			// Convert back to sRGB to save result
			out_scanline[ox * 4 + c] = static_cast<unsigned char>(linear_to_srgb(v) * 255.0);
		}
	}
}
// Composite the image centered on the larger background image. It's expected that the background image
// is bigger in x and y than the image to be placed on top
void composite_background(const QImage &img, const QImage &back, QImage &out){
	int offset_x = (back.width() - img.width()) / 2;
	int offset_y = (back.height() - img.height()) / 2;
	std::cout << "image will be placed starting at [" << offset_x << ", " << offset_y << "]\n";
#pragma omp parallel for
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

void crop_image(const QImage &img, QImage &out, const int start_x, const int end_x, const int start_y, const int end_y){
	std::cout << "cropping to " << out.width() << "x" << out.height() << "\n";
	// Copy each scanline into its place
#pragma omp parallel for
	for (int y = 0; y < img.height(); ++y){
		if (y >= start_y && y < end_y){
			const uint8_t *src = img.scanLine(y) + start_x * 4;
			uint8_t *dst = out.scanLine(y - start_y);
			const int count = (end_x - start_x) * 4;
			std::memcpy(dst, src, count);
		}
	}
}

BackgroundBuilder::BackgroundBuilder(std::shared_ptr<QImage> original, std::shared_ptr<QImage> blurred)
	: original(original), blurred(blurred)
{}
void BackgroundBuilder::run(){
	original->save(QString("original.jpg"), "JPEG");
	blurred->save(QString("blurred.jpg"), "JPEG");
	const QRect win_dim = QApplication::desktop()->screenGeometry();
	std::cout << "Desktop is: " << win_dim.width() << "x" << win_dim.height() << "\n";
	const float desktop_aspect = static_cast<float>(win_dim.width()) / win_dim.height();

	const float image_aspect = static_cast<float>(original->width()) / original->height();
	std::cout << "Desktop aspect = " << desktop_aspect << ", image = " << image_aspect << "\n";

	// If the image is has a higher aspect ratio than the desktop we'll want to scale y
	// to fit and let x be croppped, otherwise we do the opposite
	int scaled_w = 0;
	int scaled_h = 0;
	if (desktop_aspect < image_aspect){
		scaled_h = win_dim.height() + 32;
		scaled_w = scaled_h * image_aspect;
	}
	else {
		scaled_w = win_dim.width() + 32;
		scaled_h = scaled_w * (1.0 / image_aspect);
	}
	std::cout << "scaling image to " << scaled_w << "x" << scaled_h << "\n";
	auto start = std::chrono::high_resolution_clock::now();

	QImage scaled_background{scaled_w, scaled_h, QImage::Format_RGB32};
	resize_image(*blurred, scaled_background);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "resize took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< "ms\n";

	scaled_background.save(QString("scaled_background.jpg"), "JPEG");

	int crop_x = (scaled_w - win_dim.width()) / 2;
	int start_x = crop_x;
	int end_x = scaled_w - crop_x;
	// Sometimes our crop may be off by a few pixels, so just drop the extra ones if we don't match
	if (end_x - start_x != win_dim.width()){
		std::cout << "width mismatch of " << end_x - start_x - win_dim.width() << "pixels\n";
		end_x -= end_x - start_x - win_dim.width();
	}

	int crop_y = (scaled_h - win_dim.height()) / 2;
	int start_y = crop_y;
	int end_y = scaled_h - crop_y;
	if (end_y - start_y != win_dim.height()){
		std::cout << "height mismatch of " << end_y - start_y - win_dim.height() << "pixels\n";
		end_y -= end_y - start_y - win_dim.height();
	}

	QSharedPointer<QImage> background(new QImage{win_dim.width(), win_dim.height(), QImage::Format_RGB32});
	start = std::chrono::high_resolution_clock::now();
	crop_image(scaled_background, *background, start_x, end_x, start_y, end_y);
	end = std::chrono::high_resolution_clock::now();
	std::cout << "crop took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< "ms\n";

	background->save(QString("cropped_background.jpg"), "JPEG");

	std::cout << "compositing original image with blurred background\n";

	int centered_w = 0;
	int centered_h = 0;
	float border_percent = 0.04f;
	if (desktop_aspect > image_aspect) {
		centered_h = win_dim.height() - border_percent * win_dim.height();
		centered_w = centered_h * image_aspect;
		std::cout << "border size of " << border_percent * win_dim.height() << " along x\n";
	}
	else {
		centered_w = win_dim.width() - border_percent * win_dim.width();
		centered_h = centered_w * (1.0 / image_aspect);
		std::cout << "border size of " << border_percent * win_dim.width() << " along y\n";
	}
	std::cout << "Centered image will be scaled to " << centered_w << "x" << centered_h << "\n";
	QImage centered_image{centered_w, centered_h,  QImage::Format_RGB32};

	start = std::chrono::high_resolution_clock::now();
	resize_image(*original, centered_image);
	end = std::chrono::high_resolution_clock::now();
	std::cout << "scaling centered image took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
	centered_image.save(QString("centered_image.jpg"), "JPEG");

	start = std::chrono::high_resolution_clock::now();
	composite_background(centered_image, *background, *background);
	end = std::chrono::high_resolution_clock::now();
	std::cout << "compositing took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

	std::cout << "done making background\n";
	std::cout << "Saving background to background.jpg\n";
	background->save(QString::fromStdString("background.jpg"), "JPEG");
	std::cout << "background saved\n";
	emit finished(background);
}

#include "../include/moc_background_builder.cpp"

