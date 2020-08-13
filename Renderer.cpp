//
// Created by UnderSilence on 8/11/20.
//
#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <thread>

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::RenderMultithread(const Scene& scene) {
	//treat as camera
	Vector3f eye_pos(278, 273, -800);
	std::vector<Vector3f> framebuffer(scene.width * scene.height);

	int total_pixel = scene.width * scene.height;
	int pixel_per_thread = total_pixel / num_of_thread;
	int rest_pixel = total_pixel - pixel_per_thread * num_of_thread;
	int low = 0;
	std::vector<std::thread> tasks;
	std::clog << "num_of_thread: " << num_of_thread << ", SPP: " << spp << "\n";
	for (int i = 0; i < num_of_thread; i++) {
		MonotaskInfo info(low, low + pixel_per_thread, eye_pos, spp, framebuffer);
		low += pixel_per_thread;
		if (rest_pixel) {
			rest_pixel--;
			info.highIndex++;
			low++;
		}
		tasks.emplace_back(&Renderer::RenderMonotask, this, info, std::ref(scene), i == num_of_thread-1);
		// tasks.push_back(std::thread(&Renderer::doSomething, this, i));
	}


	for (int i = 0; i < tasks.size(); i++) {
		tasks[i].join();
	}
	printf("rendering...complete!\n");

	char image_name[256];
	sprintf(image_name, "image/%dx%d_%dspp_%d.ppm", scene.width, scene.height, spp, std::time(0));
	SavePPM(image_name, scene.width, scene.height, framebuffer);
}

void Renderer::SavePPM(const char* filename, int width, int height, std::vector<Vector3f>& framebuffer) const {
	FILE* fp = fopen(filename, "wb");
	(void)fprintf(fp, "P6\n%d %d\n255\n", width, height);
	for (auto i = 0; i < height * width; ++i) {
		static unsigned char color[3];
#ifdef GAMMA_CORRECTION
		framebuffer[i] = framebuffer[i] / (framebuffer[i] + Vector3f(1.0));
		framebuffer[i] = powf(framebuffer[i], Vector3f(1.0 / 2.2));
#endif
		color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
		color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
		color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
		fwrite(color, 1, 3, fp);
}
	fclose(fp);
}

void Renderer::RenderMonotask(MonotaskInfo info, const Scene& scene, bool displayProgress) {
	float scale = tan(deg2rad(scene.fov * 0.5));
	float imageAspectRatio = scene.width / (float)scene.height;
	int last = info.lowIndex;
	printf( "thread#%d render %d to %d\n", std::this_thread::get_id(), info.lowIndex, info.highIndex);
	for (int index = info.lowIndex; index < info.highIndex; index++) {
		int i = index % scene.width;
		int j = index / scene.width;
#ifdef ANTI_ALIASING
		// anti-aliasing, generate random ray inside one pixel.
		// should set random ray each spp loop, otherwise well get jagged edge
		float pixel_width = 2.0f * imageAspectRatio * scale / scene.width;
		float pixel_height = -2.0f * scale / scene.height;
		float x = (2.0f * i / (float)scene.width - 1) * imageAspectRatio * scale;
		float y = (1 - 2.0f * j / (float)scene.height) * scale;
		for (int k = 0; k < info.spp; k++) {
			Vector3f dir = normalize(Vector3f(-(x + pixel_width * get_random_float()), y + pixel_height * get_random_float(), 1));
		 	info.bufferRef[index] += scene.castRay(Ray(info.eye_pos, dir), 0) / info.spp;
		}
#else
		float x = (2 * (i + 0.5f) / (float)scene.width - 1) * imageAspectRatio * scale;
		float y = (1 - 2 * (j + 0.5f) / (float)scene.height) * scale;
		Vector3f dir = normalize(Vector3f(-x, y, 1));
		for (int k = 0; k < info.spp; k++) {
		 	info.bufferRef[index] += scene.castRay(Ray(info.eye_pos, dir), 0) / info.spp;
		}
#endif
		float step = 0.05f * (info.highIndex - info.lowIndex);
		if (displayProgress) {
			if(index - last >= step) {
				last = index;
				printf("rendering...%.2f%%\n", 100 * float(index - info.lowIndex) / (info.highIndex - info.lowIndex));
			}
			// UpdateProgress(float(index - info.lowIndex) / (info.highIndex - info.lowIndex));
		}
	}
}

void Renderer::Render(const Scene& scene) {
	std::vector<Vector3f> framebuffer(scene.width * scene.height);

	float scale = tan(deg2rad(scene.fov * 0.5));
	float imageAspectRatio = scene.width / (float)scene.height;
	Vector3f eye_pos(278, 273, -800);
	int m = 0;

	// change the spp value to change sample ammount
	std::cout << "SPP: " << spp << "\n";
	for (uint32_t j = 0; j < scene.height; ++j) {
		for (uint32_t i = 0; i < scene.width; ++i) {
			// generate primary ray direction
			float x = (2 * (i + 0.5) / (float)scene.width - 1) *
				imageAspectRatio * scale;
			float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

			Vector3f dir = normalize(Vector3f(-x, y, 1));
			for (int k = 0; k < spp; k++) {
				framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
			}
			m++;
		}
		UpdateProgress(j / (float)scene.height);
	}
	UpdateProgress(1.f);
	// save framebuffer to file
	char image_name[256];
	sprintf(image_name, "image/%dx%d_%dspp_%d.ppm", scene.width, scene.height, spp, std::time(0));
	SavePPM(image_name, scene.width, scene.height, framebuffer);
}

