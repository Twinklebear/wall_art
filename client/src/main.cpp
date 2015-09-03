#define _USE_MATH_DEFINES
#include <iostream>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>
#include <cmath>
#include <string>
#include <cstring>
#include <memory>
#include <wtypes.h>
#include <windows.h>
#include <ShellScalingAPI.h>
#include "wall_art_app.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct GaussianKernel {
	// pre-computed value of 1 / (2 * sigma^2)
	const float inv_two_sigma_sqr;
	// pre-computed value of 1 / (2 * pi * sigma&^2)
	const float denom_sqr;
	const float denom;

	GaussianKernel(float sigma) : inv_two_sigma_sqr(1.f / (2.f * std::pow(sigma, 2.f))),
		denom_sqr(inv_two_sigma_sqr * (1.f / M_PI)), denom(std::sqrt(denom_sqr))
	{}
	float operator()(float x, float y) const {
		return denom_sqr * std::exp(-inv_two_sigma_sqr * (std::pow(x, 2.f) + std::pow(y, 2.f)));
	}
	float operator()(float x) const {
		return denom * std::exp(-inv_two_sigma_sqr * std::pow(x, 2.f));
	}
};

template<typename T>
T clamp(T v, T min, T max){
	return v < min ? min : v > max ? max : v;
}
float srgb_to_linear(float v){
	return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
}
float linear_to_srgb(float v){
	return v <= 0.0031308 ? 12.92 * v : 1.055 * std::pow(v, 1.0 / 2.4) - 0.055;
}

// TODO: Make box blur performance independent of kernel dimensions following from
// http://elynxsdk.free.fr/ext-docs/Blur/Fast_box_blur.pdf
void box_blur(const unsigned char *img, unsigned char *out, const int img_w,
		const int img_h, const int kern_dim){
	std::cout << "blurring image with " << kern_dim << "x" << kern_dim << " box kernel\n";
	std::vector<unsigned char> x_blur(img_w * img_h * 3, 0);
	const float weight = 1.0 / std::pow(2.0 * kern_dim, 2.0);
	// Blur along x
#pragma omp parallel for
	for (int i = 0; i < img_h * img_w; ++i){
		int x = i % img_w;
		int y = i / img_w;
		for (int c = 0; c < 3; ++c){
			float v = 0;
			float total_w = 0;
			for (int kx = -kern_dim + 1; kx < kern_dim; ++kx){
				int fy = y;
				int fx = clamp(x + kx, 0, img_w - 1);
				// Convert to linear RGB to apply blur
				float kv = static_cast<float>(img[(fy * img_w + fx) * 3 + c]) / 255.0;
				v += weight * kv;
				total_w += weight;
			}
			// Convert back to sRGB to save result
			x_blur[(y * img_w + x) * 3 + c] = static_cast<unsigned char>((v / total_w) * 255.0);
		}
	}
	// Blur along y
#pragma omp parallel for
	for (int i = 0; i < img_h * img_w; ++i){
		int x = i % img_w;
		int y = i / img_w;
		for (int c = 0; c < 3; ++c){
			float v = 0;
			float total_w = 0;
			for (int ky = -kern_dim + 1; ky < kern_dim; ++ky){
				int fy = clamp(y + ky, 0, img_h - 1);
				int fx = x;
				// Convert to linear RGB to apply blur
				float kv = static_cast<float>(x_blur[(fy * img_w + fx) * 3 + c]) / 255.0;
				v += weight * kv;
				total_w += weight;
			}
			// Convert back to sRGB to save result
			out[(y * img_w + x) * 3 + c] = static_cast<unsigned char>((v / total_w) * 255.0);
		}
	}
}

void gaussian_blur(const unsigned char *img, unsigned char *out, const int img_w, const int img_h,
		const int kern_dim, const GaussianKernel &kernel){
	// TODO: Account for sigma in the kernel dimension we send through?
	// We actually approximate the gaussian blur using 4 passes of a box blur
	std::vector<unsigned char> blur_pong(img_w * img_h * 3, 0);
	box_blur(img, blur_pong.data(), img_w, img_h, kern_dim);
	box_blur(blur_pong.data(), out, img_w, img_h, kern_dim);
	box_blur(out, blur_pong.data(), img_w, img_h, kern_dim);
	box_blur(blur_pong.data(), out, img_w, img_h, kern_dim);
}

void resize_image(const unsigned char *img, unsigned char *out, const int img_w, const int img_h,
		const int out_w, const int out_h){
#pragma omp parallel for
	for (int i = 0; i < out_w * out_h; ++i){
		int ox = i % out_w;
		int oy = i / out_w;
		float fx = img_w * static_cast<float>(ox) / out_w;
		float fy = img_h * static_cast<float>(oy) / out_h;
		int ix = static_cast<int>(fx);
		int iy = static_cast<int>(fy);
		fx = fx - ix;
		fy = fy - iy;
		// This is just nearest neighbor filtering with sRGB correction
		for (int c = 0; c < 3; ++c){
			// Convert to linear space to do the filtering
			int x = ix;
			int y = iy;
			float v00 = srgb_to_linear(static_cast<float>(img[(y * img_w + x) * 3 + c]) / 255.0);

			x = clamp(ix + 1, 0, img_w - 1);
			float v10 = srgb_to_linear(static_cast<float>(img[(y * img_w + x) * 3 + c]) / 255.0);

			x = ix;
			y = clamp(iy + 1, 0, img_h - 1);
			float v01 = srgb_to_linear(static_cast<float>(img[(y * img_w + x) * 3 + c]) / 255.0);

			x = clamp(ix + 1, 0, img_w - 1);
			float v11 = srgb_to_linear(static_cast<float>(img[(y * img_w + x) * 3 + c]) / 255.0);

			// Now do the bilinear filtering
			float v = v00 * (1.0 - fx) * (1.0 - fy) + v10 * fx * (1.0 - fy) + v01 * (1.0 - fx) * fy
				+ v11 * fx * fy;

			// Convert back to sRGB to save result
			out[(oy * out_w + ox) * 3 + c] = static_cast<unsigned char>(linear_to_srgb(v) * 255.0);
		}
	}
}
// Composite the image centered on the larger background image. It's expected that the background image
// is bigger in x and y than the image to be placed on top
void composite_background(const unsigned char *img, const unsigned char *back, unsigned char *out,
		const int img_w, const int img_h, const int back_w, const int back_h){
	int offset_x = (back_w - img_w) / 2;
	int offset_y = (back_h - img_h) / 2;
	std::cout << "image will be placed starting at [" << offset_x << ", " << offset_y << "]\n";
#pragma omp parallel for
	for (int i = 0; i < back_w * back_h; ++i){
		const int ox = i % back_w;
		const int oy = i / back_w;
		for (int c = 0; c < 3; ++c){
			if (ox < offset_x || ox >= back_w - offset_x - 1 || oy < offset_y || oy >= back_h - offset_y - 1){
				out[(oy * back_w + ox) * 3 + c] = back[(oy * back_w + ox) * 3 + c];
			}
			else {
				const int ix = ox - offset_x;
				const int iy = oy - offset_y;
				out[(oy * back_w + ox) * 3 + c] = img[(iy * img_w + ix) * 3 + c];
			}
		}
	}
}

void crop_image(const unsigned char *img, unsigned char *out, const int img_w, const int img_h,
		const int start_x, const int end_x, const int start_y, const int end_y){
	const int out_w = end_x - start_x;
	const int out_h = end_y - start_y;
	std::cout << "cropping to " << out_w << "x" << out_h << "\n";
	// Copy each scanline into its place
#pragma omp parallel for
	for (int y = 0; y < img_h; ++y){
		if (y >= start_y && y < end_y){
			const unsigned char *st = &img[y * img_w * 3 + start_x * 3];
			const int count = (end_x - start_x) * 3;
			std::memcpy(&out[(y - start_y) * out_w * 3], st, count);
		}
	}
}

std::pair<int, int> desktop_resolution(){
	RECT desktop_dim;
	const HWND desktop = GetDesktopWindow();
	HMONITOR monitor = MonitorFromWindow(desktop, MONITOR_DEFAULTTOPRIMARY);
	GetWindowRect(desktop, &desktop_dim);
	return std::make_pair<int, int>(static_cast<int>(desktop_dim.right),
		static_cast<int>(desktop_dim.bottom));
}

int main(int argc, char** argv){
	if (argc < 2){
		std::cout << "Usage ./image_blur <url>\n";
		return 1;
	}
	std::unique_ptr<WallArtApp> app = std::make_unique<WallArtApp>(argc, argv);
	QPushButton button("Hi");
	button.show();
	app->fetch_url(argv[1]);
	return app->exec();

	if (argc < 2){
		std::cout << "Usage ./image_blur <image_file>\n";
		return 1;
	}
	// Tell Windows we're DPI aware so we can get the true resolution of
	// the display
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

	const auto win_dim = desktop_resolution();
	std::cout << "Desktop is " << win_dim.first << "x" << win_dim.second << " pixels\n";
	const float desktop_aspect = static_cast<float>(win_dim.first) / win_dim.second;

	int w, h, n;
	unsigned char *img = stbi_load(argv[1], &w, &h, &n, 3);
	std::cout << "\'" << argv[1] << "\' is " << w << "x" << h
		<< " pixels with " << n << " components per pixel (fixed to 3 comp)\n";
	const float image_aspect = static_cast<float>(w) / h;
	std::cout << "Desktop aspect = " << desktop_aspect << ", image = " << image_aspect << "\n";

	// If the image is has a higher aspect ratio than the desktop we'll want to scale y
	// to fit and let x be croppped, otherwise we do the opposite
	int scaled_w = 0;
	int scaled_h = 0;
	if (desktop_aspect < image_aspect){
		scaled_h = win_dim.second + 32;
		scaled_w = scaled_h * image_aspect;
	}
	else {
		scaled_w = win_dim.first + 32;
		scaled_h = scaled_w * (1.0 / image_aspect);
	}
	std::cout << "scaling image to " << scaled_w << "x" << scaled_h << "\n";
	std::vector<unsigned char> scaled(scaled_w * scaled_h * 3, 0);

	auto start = std::chrono::high_resolution_clock::now();
	resize_image(img, scaled.data(), w, h, scaled_w, scaled_h);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "resize took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< "ms\n";

	if (!stbi_write_png("scaled.png", scaled_w, scaled_h, 3, scaled.data(), 0)){
		std::cout << "Error saving the resulting image\n";
	}

	std::vector<unsigned char> cropped(win_dim.first * win_dim.second * 3, 0);
	int crop_x = (scaled_w - win_dim.first) / 2;
	int start_x = crop_x;
	int end_x = scaled_w - crop_x;
	// Sometimes our crop may be off by a few pixels, so just drop the extra ones if we don't match
	if (end_x - start_x != win_dim.first){
		std::cout << "width mismatch of " << end_x - start_x - win_dim.first << "pixels\n";
		end_x -= end_x - start_x - win_dim.first;
	}

	int crop_y = (scaled_h - win_dim.second) / 2;
	int start_y = crop_y;
	int end_y = scaled_h - crop_y;
	if (end_y - start_y != win_dim.second){
		std::cout << "height mismatch of " << end_y - start_y - win_dim.second << "pixels\n";
		end_y -= end_y - start_y - win_dim.second;
	}

	start = std::chrono::high_resolution_clock::now();
	crop_image(scaled.data(), cropped.data(), scaled_w, scaled_h, start_x, end_x, start_y, end_y);
	end = std::chrono::high_resolution_clock::now();
	std::cout << "crop took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< "ms\n";

	if (!stbi_write_png("cropped.png", win_dim.first, win_dim.second, 3, cropped.data(), 0)){
		std::cout << "Error saving the resulting image\n";
	}

	// Now blur the cropped background
	std::vector<unsigned char> blurred(win_dim.first * win_dim.second * 3, 0);
	const int kern_dim = 24;

	start = std::chrono::high_resolution_clock::now();
	gaussian_blur(cropped.data(), blurred.data(), win_dim.first, win_dim.second, kern_dim,
			GaussianKernel{kern_dim / 3.0});
	end = std::chrono::high_resolution_clock::now();
	std::cout << "blur took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< "ms\n";

	if (!stbi_write_png("blurred.png", win_dim.first, win_dim.second, 3, blurred.data(), 0)){
		std::cout << "Error saving the resulting image\n";
	}

	std::cout << "compositing original image with blurred background\n";

	int centered_w = 0;
	int centered_h = 0;
	float border_percent = 0.04;
	if (desktop_aspect > image_aspect) {
		centered_h = win_dim.second - border_percent * win_dim.second;
		centered_w = centered_h * image_aspect;
		std::cout << "border size of " << border_percent * win_dim.second << " along x\n";
	}
	else {
		centered_w = win_dim.first - border_percent * win_dim.first;
		centered_h = centered_w * (1.0 / image_aspect);
		std::cout << "border size of " << border_percent * win_dim.first << " along y\n";
	}
	std::cout << "Centered image will be scaled to " << centered_w << "x" << centered_h << "\n";
	std::vector<uint8_t> centered_image(centered_h * centered_w * 3, 0);
	start = std::chrono::high_resolution_clock::now();
	resize_image(scaled.data(), centered_image.data(), scaled_w, scaled_h, centered_w, centered_h);
	end = std::chrono::high_resolution_clock::now();
	std::cout << "scaling centered image took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

	std::vector<unsigned char> composite(win_dim.first * win_dim.second * 3, 0);
	start = std::chrono::high_resolution_clock::now();
	composite_background(centered_image.data(), blurred.data(), composite.data(), centered_w, centered_h,
			win_dim.first, win_dim.second);
	end = std::chrono::high_resolution_clock::now();
	std::cout << "compositing took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

	if (!stbi_write_png("composite.png", win_dim.first, win_dim.second, 3, composite.data(), 0)){
		std::cout << "Error saving the resulting image\n";
	}

	stbi_image_free(img);
	return 0;
}

