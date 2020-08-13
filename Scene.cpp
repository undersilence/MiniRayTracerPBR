#include "Scene.hpp"

void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const {
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const {
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()) {
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()) {
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum) {
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(const Ray &ray, const std::vector<Object *> &objects,
                  float &tNear, uint32_t &index, Object **hitObject) {
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
Vector3f Scene::castRay(const Ray &ray, int depth) const {
    // TO DO Implement Path Tracing Algorithm here
    // caculate light_direct
    Intersection inter_object;
    Vector3f color(0);
    Vector3f L_dir(0), L_indir(0);
    inter_object = intersect(ray);
    if(inter_object.emit.norm() > 0.001) {
        return Vector3f(1);
    }

    if (inter_object.happened) {
        Intersection inter_light;
        float pdf_light;
        sampleLight(inter_light, pdf_light);
        Vector3f p = inter_object.coords;
        Vector3f n = inter_object.normal;
        Vector3f x = inter_light.coords;
        Vector3f nn = inter_light.normal;
        Vector3f ws = normalize(x - p);
        Vector3f wo = -ray.direction;
        Material *m = inter_object.m;
        Vector3f f_r = m->eval(ws, wo, n);
        if ((intersect(Ray(p, ws)).coords - x).norm() < 0.01) {
            /*std::clog << "calc(" << inter_light.emit << "),(" << f_r << "),(" << dotProduct(-ws, nn) << "),(" << dotProduct(ws, n) << "),(" << dotProduct(x - p, x - p) << "),(" << pdf_light << ")" << std::endl;*/
            L_dir = inter_light.emit * f_r *
                     dotProduct(-ws, nn) * dotProduct(ws, n) /
                     dotProduct(x - p, x - p) /
                     pdf_light;
        }

        //indirect light
        if (get_random_float() < Scene::RussianRoulette) {
            Vector3f wi = m->sample(wo, n);
            // std::clog << "sample dir " << wi << std::endl;
            Ray ray_reflect(p, wi);
            f_r = m->eval(wi, wo, n);
            //std::clog << "calc indirect ("<<castRay_t(Ray(inter_light.coords, wi), 0)<<")*("<<f_r<<")*("<<dotProduct(wi, n)<<")/("<<m->pdf(wi, wo, n)<<")/("<<RussianRoulette<<")"<<std::endl;
			Vector3f irradience = castRay(ray_reflect, depth);
        	L_indir = irradience * f_r * dotProduct(wi, n) / std::max(0.0001f, m->pdf(wi, wo, n)) / Scene::RussianRoulette;
        }
        color = L_dir + L_indir;
    }

    // std::clog << "return (" << L_dir << ")+(" << L_indir << ")" << std::endl;
    return Vector3f::Min(Vector3f::Max(color, Vector3f(0)), Vector3f(1));
}
