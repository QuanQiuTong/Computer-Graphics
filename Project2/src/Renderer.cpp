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
    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

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
            {
                dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
            }
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

Vector3f traceReflect(const Ray &r, const SceneParser &scene, int bounces)
{
    if (bounces < 0)
        return Vector3f(0, 0, 0);

    Hit h;
    if (!scene.getGroup()->intersect(r, 0.001f, h))
        return scene.getBackgroundColor(r.getDirection());

    Vector3f I = -r.getDirection(),
             R = h.getNormal() * 2.0f * Vector3f::dot(I, h.getNormal()) - I;
    Ray reflectionRay(r.pointAtParameter(h.getT()), R);

    //h.getMaterial()->shade(r, h, );

    return {0, 0, 0};
}

Vector3f
Renderer::traceRay(const Ray &r,
                   float tmin,
                   int bounces,
                   Hit &h) const
{
    // The starter code only implements basic drawing of sphere primitives.
    // You will implement phong shading, recursive ray tracing, and shadow rays.

    if (!_scene.getGroup()->intersect(r, tmin, h))
        return _scene.getBackgroundColor(r.getDirection());

    Vector3f I(0, 0, 0);
    for (auto light : _scene.lights)
    {
        Vector3f tolight, ind;
        float dist;
        light->getIllumination(r.pointAtParameter(h.getT()), tolight, ind, dist);
        I += h.getMaterial()->shade(r, h, tolight, ind);

        traceReflect(r, _scene, bounces - 1);

        Ray shadowRay(r.pointAtParameter(h.getT()), tolight);
        Hit shadowHit;
        if (_scene.getGroup()->intersect(shadowRay, 0.001f, shadowHit) && shadowHit.getT() < dist)
            I += h.getMaterial()->shade(r, h, tolight, ind) * 0.5f;
        bool shadow = _args.shadows;
        if (shadow)
        {
            // Implement shadow rays here
            // You should shoot a new ray in the direction of the light.
            // If the ray hits an object, you should add the ambient color to the pixel.
            // The starter code only implements no shadow rays.
            // You will implement shadow rays.
        }

        if (bounces > 0)
        {
            // Implement recursive ray tracing here
            // You should shoot a new ray in the direction of the reflection.
            // The starter code only implements one bounce of recursive ray tracing.
            // You will implement more bounces.
            Vector3f R = h.normal * 2.0f * Vector3f::dot(tolight, h.normal) - tolight;
            Ray reflectionRay(r.pointAtParameter(h.getT()), R);
            Hit reflectionHit;

            Vector3f reflectionColor = traceRay(reflectionRay, 0.001f, bounces - 1, reflectionHit);
            I += reflectionColor * h.getMaterial()->getSpecularColor();
        }

        // Implement Fresnel effects here
        // You should shoot a new ray in the direction of the refraction.
        // The starter code only implements reflection.
        // You will implement refraction.

        // Implement depth of field here
        // You should jitter the origin of the ray within the aperture of the camera.
        // The starter code only implements no depth of field.
        // You will implement depth of field.

        // Implement super-sampling here
        // You should shoot jittered rays within the pixel.
        // The starter code only implements no super-sampling.
        // You will implement super-sampling.
    }
    return I + _scene.getAmbientLight() * h.getMaterial()->getDiffuseColor();
}
