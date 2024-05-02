#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>

Renderer::Renderer(const ArgParser &args) : _args(args),
                                            _scene(args.input_file) {}

void Renderer::Render()
{
    int w = _args.width, h = _args.height;

    Image image(w, h), nimage(w, h), dimage(w, h);

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and callse traceRay.
    // It also write to the color, normal, and depth images.
    // You should understand what this code does.
    Camera *cam = _scene.getCamera();
    for (int y = 0; y < h; ++y)
    {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x)
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

Vector3f mirror(const Vector3f &tolight, const Vector3f &normal)
{
    return (2 * normal * Vector3f::dot(tolight, normal) - tolight).normalized();
}

Vector3f traceReflect(const Ray &r, const SceneParser &scene, int bounces, const Vector3f &indensity)
{
    if (bounces < 0)
        return Vector3f(0, 0, 0);

    Hit h;
    if (!scene.getGroup()->intersect(r, 0.001f, h))
        return scene.getBackgroundColor(r.getDirection());

    Material *m = h.getMaterial();
    if (!m)
        return Vector3f(0, 0, 0);

    auto tolight = -r.getDirection();

    auto I = m->shade(r, h, tolight, indensity);

    Ray refl(r.pointAtParameter(h.getT()), mirror(tolight, h.getNormal()));

    I += m->getSpecularColor() * traceReflect(refl, scene, bounces - 1, indensity);

    return I;
}

Vector3f Renderer::traceRay(const Ray &r, float tmin, int bounces, Hit &h) const
{
    if (!_scene.getGroup()->intersect(r, tmin, h))
        return _scene.getBackgroundColor(r.getDirection());

    Material *m = h.getMaterial();
    assert(m);
        
    auto p = r.pointAtParameter(h.getT());
    Vector3f I(0, 0, 0);
    for (auto light : _scene.lights)
    {
        Vector3f tolight, ind;
        float dist;
        light->getIllumination(p, tolight, ind, dist);
        I += m->shade(r, h, tolight, ind);

        // Ray refl(p, mirror(tolight, h.getNormal()));
        // I += h.getMaterial()->getSpecularColor() * traceReflect(refl, _scene, bounces - 1, ind);
    }
    return I + _scene.getAmbientLight() * m->getDiffuseColor();
}
