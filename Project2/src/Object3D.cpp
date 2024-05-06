#include "Object3D.h"

static auto dot = Vector3f::dot;

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); // Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center; // Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0)
        return false;

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f * a);
    float tminus = (-b - d) / (2.0f * a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin))
        return false;

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin)
        t = tminus;

    // one intersection at the front. one at the back
    if ((tplus > tmin) && (tminus < tmin))
        t = tplus;

    if (t < h.getT())
    {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal.normalize();
        h.set(t, this->material, normal);
        return true;
    }
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj)
{
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const
{
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    bool hit = false;
    for (Object3D *o : m_members)
        hit |= (o->intersect(r, tmin, h));
    return hit;
}

Plane::Plane(const Vector3f &normal, float d, Material *m)
    : Object3D(m), _normal(normal), _d(d) {}

bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    Vector3f ori = r.getOrigin(), dir = r.getDirection();
    if (dot(dir, _normal) == 0)
        return false;

    float t = (_d - dot(ori, _normal)) / dot(dir, _normal);

    if (t < tmin || h.getT() < t)
        return false;

    h.set(t, material, _normal);
    return true;
}

static auto cross = Vector3f::cross;

bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const
{
    Vector3f origin = r.getOrigin(), direction = r.getDirection();
    Vector3f a = _v[0], b = _v[1], c = _v[2];

    float dno = dot(cross(direction, c - a), b - a);

    Vector3f num = cross(origin - a, b - a);

    float t = (dot(num, c - a)) / dno;
    float beta = (dot(cross(direction, c - a), origin - a)) / dno;
    float gamma = (dot(num, direction)) / dno;

    if (!(beta > 0 && gamma > 0 && (1 - beta - gamma > 0) && t > tmin && t < h.getT()))
        return false;

    h.set(t, material, ((1 - beta - gamma) * _normals[0] + beta * _normals[1] + gamma * _normals[2]).normalized());
    return true;
}

Transform::Transform(const Matrix4f &m, Object3D *obj)
    : _object(obj), _m(m), _inv(m.inverse()) {}

static Vector3f trn(Matrix4f m, Vector3f v, float w)
{
    return (m * Vector4f(v, w)).xyz();
}

bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    Ray tr(trn(_inv, r.getOrigin(), 1),
           trn(_inv, r.getDirection(), 0).normalized());
    Hit th;
    tmin = (trn(_inv, r.pointAtParameter(tmin), 1) - tr.getOrigin()).abs();

    if (!_object->intersect(tr, tmin, th))
        return false;

    float t = (trn(_m, tr.pointAtParameter(th.getT()), 1) - r.getOrigin()).abs();
    if (t < tmin || t > h.getT())
        return false;

    h.set(t, th.getMaterial(), trn(_inv.transposed(), th.getNormal(), 0).normalized());
    return true;
}
