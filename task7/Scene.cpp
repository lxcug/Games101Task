//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }
    return (*hitObject != nullptr);
}



// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TODO Implement Path Tracing Algorithm here
    Intersection inter = intersect(ray);
    if(!inter.happened)
        return {0.f};
    return Shade(inter, -ray.direction);
}

Vector3f Scene::Shade(const Intersection& hitPos, const Vector3f& wo) const {
    // Light Emission
    if(hitPos.m->hasEmission()) {
        return hitPos.m->getEmission();
    }

    const float epsilon = 0.005f;

    // Sample from Area Light
    Vector3f L_dir = Vector3f(0.);  // Direct Light
    Intersection lightSamplePos;
    float lightPdf;
    sampleLight(lightSamplePos, lightPdf);

    Vector3f p = hitPos.coords, x = lightSamplePos.coords;
    Vector3f ws = normalize(x - p);
    Vector3f NN = lightSamplePos.normal, N = hitPos.normal;

    if((intersect(Ray(p, ws)).coords - x).norm() < epsilon) {
        L_dir = lightSamplePos.emit * hitPos.m->eval(wo, ws, N)
        * dotProduct(N, ws) * dotProduct(NN, -ws) / ((x-p).norm() * (x-p).norm()) / lightPdf;
    }

    Vector3f L_indir;  // Indirect Light
    if(get_random_float() < RussianRoulette) {
        Vector3f wi = hitPos.m->sample(wo, hitPos.normal).normalized();
        float pdf = hitPos.m->pdf(wo, wi, hitPos.normal);
        if(pdf > epsilon) {
            Intersection hit = intersect(Ray(hitPos.coords, wi));
            if(hit.happened && !hit.m->hasEmission()) {
                L_indir = Shade(hit, wi) * hitPos.m->eval(wo, wi, hitPos.normal) *
                        dotProduct(wi, hitPos.normal) / pdf / RussianRoulette;
            }
        }
    }

    return L_dir + L_indir;
}
