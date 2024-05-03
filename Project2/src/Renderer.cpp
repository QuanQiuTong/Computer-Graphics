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
constexpr int scale = 3, weight[3][3] = {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}}, sum = 16;

#define For(i, n) for (int i = 0; i < n; ++i)
void Renderer::Render()
{
    int w = _args.width, h = _args.height;
    int samples = _args.jitter ? jittersamples : 1;
    auto jitter = _args.jitter ? ([]
                                  { static std::default_random_engine generator;
                                    static std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
                                    return distribution(generator); })
                               : ([]
                                  { return 0.0f; });

    Image image(w, h), nimage(w, h), dimage(w, h);

    Camera *cam = _scene.getCamera();

    if (!_args.filter)
        For(y, h) For(x, w)
        {
            Vector3f color = {0, 0, 0}, norm = {0, 0, 0};
            float depth = 0;
            For(_, samples)
            {
                float ndcy = 2 * ((y + jitter()) / (h - 1.0f)) - 1.0f;
                float ndcx = 2 * ((x + jitter()) / (w - 1.0f)) - 1.0f;
                Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
                Hit h;
                color += traceRay(r, cam->getTMin(), _args.bounces, h);
                norm += (h.getNormal() + 1.0f) / 2.0f;
                float range = (_args.depth_max - _args.depth_min);
                if (range)
                    depth += (h.t - _args.depth_min) / range;
            }
            image.setPixel(x, y, color / samples);
            nimage.setPixel(x, y, norm / samples);
            dimage.setPixel(x, y, Vector3f(depth / samples));
        }
    else
        For(y, h) For(x, w)
        {
            Vector3f color = {0, 0, 0}, norm = {0, 0, 0};
            float depth = 0;
            For(i, scale) For(j, scale)
                For(_, samples)
            {
                float ndcy = 2 * ((y * scale + i + jitter()) / (h * scale - 1.0f)) - 1.0f;
                float ndcx = 2 * ((x * scale + j + jitter()) / (w * scale - 1.0f)) - 1.0f;
                Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
                Hit h;
                color += traceRay(r, cam->getTMin(), _args.bounces, h) * weight[i][j];
                norm += (h.getNormal() + 1.0f) / 2.0f * weight[i][j];
                float range = (_args.depth_max - _args.depth_min);
                if (range)
                    depth += (h.t - _args.depth_min) / range * weight[i][j];
            }
            image.setPixel(x, y, color / sum / samples);
            nimage.setPixel(x, y, norm / sum / samples);
            dimage.setPixel(x, y, Vector3f(depth / sum / samples));
        }

    if (_args.output_file.size())
        image.savePNG(_args.output_file);
    if (_args.depth_file.size())
        dimage.savePNG(_args.depth_file);
    if (_args.normals_file.size())
        nimage.savePNG(_args.normals_file);
}
#undef For

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
    Hit sh, rh;
    for (Light *light : _scene.lights)
    {
        float dist;
        light->getIllumination(p, tolight, ind, dist);

        if (_args.shadows && _scene.getGroup()->intersect({p, tolight}, 0.0001f, sh) && sh.getT() < dist + 0.0001f)
            continue;
        I += m->shade(r, h, tolight, ind);
    }
    if (bounces > 0)
        I += traceRay(refl(r, h), 0.0001f, bounces - 1, rh) * m->getSpecularColor();
    return I;
}

/**
 * The following code is a refactored version of the code above.
 * It is even harder to read and less efficient.
 * The only advantage is that it is shorter.
 */
/*
For(y, h) For(x, w)
{
    Vector3f color = {0, 0, 0}, norm = {0, 0, 0};
    float depth = 0;
    int scale = _args.filter ? scale : 1;
    float sum = _args.filter ? sum : samples;
    For(i, scale) For(j, scale)
    {
        Vector3f color2 = {0, 0, 0}, norm2 = {0, 0, 0};
        float depth2 = 0;
        For(_, samples)
        {
            float ndcy = 2 * ((y * scale + i + jitter()) / (h * scale - 1.0f)) - 1.0f;
            float ndcx = 2 * ((x * scale + j + jitter()) / (w * scale - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
            Hit h;
            color2 += traceRay(r, cam->getTMin(), _args.bounces, h);
            norm2 += (h.getNormal() + 1.0f) / 2.0f;
            float range = (_args.depth_max - _args.depth_min);
            if (range)
                depth2 += (h.t - _args.depth_min) / range;
        }
        color += _args.filter ? color2 * weight[i][j] / samples : color2 / samples;
        norm += _args.filter ? norm2 * weight[i][j] / samples : norm2 / samples;
        depth += _args.filter ? depth2 * weight[i][j] / samples : depth2 / samples;
    }
    image.setPixel(x, y, color / sum);
    nimage.setPixel(x, y, norm / sum);
    dimage.setPixel(x, y, Vector3f(depth / sum));
}

*/