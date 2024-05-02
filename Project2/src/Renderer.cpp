#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>
#include <random>
#include <functional>

Renderer::Renderer(const ArgParser &args) : _args(args),
                                            _scene(args.input_file) {}

constexpr int jittersamples = 16;

static std::default_random_engine generator;
static std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
static auto random_float = std::bind(distribution, generator);

void Renderer::Render()
{
    int w = _args.width, h = _args.height;

    Image image(w, h), nimage(w, h), dimage(w, h);

    Camera *cam = _scene.getCamera();
    for (int y = 0; y < h; ++y)
    {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x)
        {
            if (!_args.jitter)
            {
                float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
                // Use PerspectiveCamera to generate a ray.
                // You should understand what generateRay() does.
                Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

                Hit h;
                Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);

                image.setPixel(x, y, color);
                nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
                float range = (_args.depth_max - _args.depth_min);
                if (range)
                    dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));

                continue;
            }
            Vector3f color(0, 0, 0), normal(0, 0, 0);
            float depth = 0;
            for (int i = 0; i < jittersamples; ++i)
            {
                float ndcx = 2 * (x + random_float()) / (w - 1.0f) - 1.0f;
                float ndcy = 2 * (y + random_float()) / (h - 1.0f) - 1.0f;
                Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

                Hit h;
                color += traceRay(r, cam->getTMin(), _args.bounces, h);
                normal += (h.getNormal() + 1.0f) / 2.0f;
                float range = (_args.depth_max - _args.depth_min);
                if (range)
                    depth += (h.t - _args.depth_min) / range;
            }
            image.setPixel(x, y, color / jittersamples);
            nimage.setPixel(x, y, normal / jittersamples);
            dimage.setPixel(x, y, Vector3f(depth / jittersamples));
        }
    }

    // save the files
    if (_args.output_file.size())
        image.savePNG(_args.output_file);
    if (_args.depth_file.size())
        dimage.savePNG(_args.depth_file);
    if (_args.normals_file.size())
        nimage.savePNG(_args.normals_file);
}

static Ray refl(const Ray &r, const Hit &h)
{
    return {r.pointAtParameter(h.getT()),
            (r.getDirection() - 2 * h.getNormal() * Vector3f::dot(r.getDirection(), h.getNormal())).normalized()};
}

Vector3f Renderer::traceRay(const Ray &r, float tmin, int bounces, Hit &h) const
{
    if (!_scene.getGroup()->intersect(r, tmin, h))
        return _scene.getBackgroundColor(r.getDirection());

    Material *m = h.getMaterial();

    auto p = r.pointAtParameter(h.getT());
    Vector3f I = _scene.getAmbientLight() * m->getDiffuseColor(), tolight, ind;
    for (Light *light : _scene.lights)
    {
        float dist;
        light->getIllumination(p, tolight, ind, dist);

        Hit sh, rh;
        if (_args.shadows && _scene.getGroup()->intersect({p, tolight}, 0.0001f, sh) && sh.getT() < dist)
            continue;
        I += m->shade(r, h, tolight, ind);
        if (bounces > 0)
            I += traceRay(refl(r, h), 0.0001f, bounces - 1, rh) * m->getSpecularColor();
    }
    return I;
}
