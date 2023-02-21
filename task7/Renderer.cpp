//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <thread>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <mutex>
#include <vector>


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

std::mutex mtx;
int finishLineCount = 0;

void run(int low, int high, const Scene& scene, int spp, std::vector<Vector3f>& framebuffer) {
    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);

    for (int i = low; i < high; ++i) {
        for (int j = 0; j < scene.width; ++j) {
            for (int k = 0; k < spp; k++){
                float xPerturbation = get_random_float(), yPerturbation = get_random_float();
                float x = (2 * (j + xPerturbation) / (float)scene.width - 1) *
                           imageAspectRatio * scale;
                float y = (1 - 2 * (i + yPerturbation) / (float)scene.height) * scale;
                Vector3f dir = normalize(Vector3f(-x, y, 1));
                Ray ray = Ray(eye_pos, dir);
                framebuffer[i*scene.width + j] += scene.castRay(ray, 0) / spp;
            }
        }
        mtx.lock();
        finishLineCount++;
        UpdateProgress(finishLineCount/(float)scene.height);
        mtx.unlock();
    }
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    // change the spp value to change sample amount
    int spp = 32;
    std::cout << "SPP: " << spp << "\n";

    int numThreads = (int)std::thread::hardware_concurrency();
    int lines = scene.height / numThreads + 1;
    std::vector<std::thread> threads;
    for(int i = 0; i < numThreads; i++) {
        int low = i * lines;
        int high = low + lines < scene.height ? low + lines : scene.height;
        threads.push_back(std::thread(run, low, high, std::ref(scene), spp, std::ref(framebuffer)));
    }

    for(auto& thread: threads) {
        thread.join();
    }
    UpdateProgress(1.f);


    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}
