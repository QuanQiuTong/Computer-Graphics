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

Ray refl(const Ray &r, const Hit &h)
{
    Vector3f tolight = -r.getDirection();
    Vector3f normal = h.getNormal();
    Vector3f reflection = (2 * normal * Vector3f::dot(tolight, normal) - tolight).normalized();
    return Ray(r.pointAtParameter(h.getT()), reflection);
}

Vector3f traceReflect(const Ray &r, const SceneParser &scene, int bounces, const Vector3f &indensity)
{
    if (bounces < 0)
        return {0, 0, 0};

    Hit h;
    if (!scene.getGroup()->intersect(r, 0.0001f, h))
        return scene.getBackgroundColor(r.getDirection());

    Material *m = h.getMaterial();

    return m->shade(r, h, -r.getDirection(), indensity) +
           traceReflect(refl(r, h), scene, bounces - 1, indensity) * m->getSpecularColor();
}

Vector3f Renderer::traceRay(const Ray &r, float tmin, int bounces, Hit &h) const
{
    if (!_scene.getGroup()->intersect(r, tmin, h))
        return _scene.getBackgroundColor(r.getDirection());

    Material *m = h.getMaterial();

    auto p = r.pointAtParameter(h.getT());
    Vector3f I = _scene.getAmbientLight() * m->getDiffuseColor();
    for (Light *light : _scene.lights)
    {
        Vector3f tolight, ind;
        float dist;
        light->getIllumination(p, tolight, ind, dist);

        Hit sh;
        if (_args.shadows && _scene.getGroup()->intersect({p, tolight}, 0.0001f, sh) && sh.getT() < dist)
            continue;
        I += m->shade(r, h, tolight, ind);

        Ray ref = {p, mirror(tolight, h.getNormal())};
        Ray good = refl(r, h);
        I += traceReflect(ref, _scene, bounces - 1, ind) * m->getSpecularColor();

        if ((ref.getOrigin() - good.getOrigin()).abs() > 0.0001 || (ref.getDirection() - good.getDirection()).abs() > 0.0001)
        {
            printf("ref %f %f %f %f %f %f\n", ref.getOrigin().x(), ref.getOrigin().y(), ref.getOrigin().z(), ref.getDirection().x(), ref.getDirection().y(), ref.getDirection().z());
            printf("good %f %f %f %f %f %f\n", good.getOrigin().x(), good.getOrigin().y(), good.getOrigin().z(), good.getDirection().x(), good.getDirection().y(), good.getDirection().z());
            puts("");
        }
    }
    return I;
}
