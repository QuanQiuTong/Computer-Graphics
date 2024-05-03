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
    assert(material);
    h.set(t, material, _normal);
    return true;
}
static auto cross = Vector3f::cross;
// bool rayTriangleIntersect(const FVector& v0, const FVector& v1, const FVector& v2, const FVector& orig,
// 		const FVector& dir, float& tnear, float& u, float& v) const
// {
// 	// TODO: Implement this function that tests whether the triangle
// 	// that's specified bt v0, v1 and v2 intersects with the ray (whose
// 	// origin is *orig* and direction is *dir*)
// 	// Also don't forget to update tnear, u and v.
// 	FVector E1 = v1 - v0;
// 	FVector E2 = v2 - v0;
// 	FVector S = orig - v0;
// 	FVector S1 = FVector::CrossProduct(dir, E2);
// 	FVector S2 = FVector::CrossProduct(S, E1);

// 	float factor = FVector::DotProduct(S1, E1);
// 	float t = 1.0f / factor * FVector::DotProduct(S2, E2);
// 	float b1 = 1.0f / factor * FVector::DotProduct(S1, S);
// 	float b2 = 1.0f / factor * FVector::DotProduct(S2, dir);

// 	if (t > 0 && b1 > 0 && b2 > 0 && (1 - b1 - b2) > 0) {
// 		tnear = t;
// 		u = b1;
// 		v = b2;
// 		return true;
// 	}
// 	return false;
// }

bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const
{
    static auto det = Matrix3f::determinant3x3;
    Vector3f R = r.getOrigin();
    Vector3f D = r.getDirection();
    Vector3f a = _v[0], b = _v[1], c = _v[2];
    float mA, mx, my, mz, by, bz, newt;
    mA = det(a.x() - b.x(), a.y() - b.y(), a.z() - b.z(), a.x() - c.x(), a.y() - c.y(), a.z() - c.z(), D.x(), D.y(), D.z());
    mx = det(a.x() - R.x(), a.y() - R.y(), a.z() - R.z(), a.x() - c.x(), a.y() - c.y(), a.z() - c.z(), D.x(), D.y(), D.z());
    my = det(a.x() - b.x(), a.y() - b.y(), a.z() - b.z(), a.x() - R.x(), a.y() - R.y(), a.z() - R.z(), D.x(), D.y(), D.z());
    mz = det(a.x() - b.x(), a.y() - b.y(), a.z() - b.z(), a.x() - c.x(), a.y() - c.y(), a.z() - c.z(), a.x() - R.x(), a.y() - R.y(), a.z() - R.z());
    by = mx / mA;
    bz = my / mA;
    newt = mz / mA;
    if (by + bz < 1 && by > 0 && bz > 0 && newt >= tmin && newt < h.getT())
    {
        Vector3f sn = (1 - by - bz) * _normals[0] + by * _normals[1] + bz * _normals[2];
        h.set(newt, material, sn.normalized());
        return true;
    }
    return false;
}

// bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const
// {
//     Vector3f origin = r.getOrigin(),direction = r.getDirection();
//     Vector3f a = _v[0], b = _v[1], c = _v[2];

//     float dno = dot(cross(direction, c - a),b - a);

//     Vector3f num = cross(origin - a, b - a);

//     float t = (dot(num ,c - a)) / dno;
//     float beta = (dot(cross(direction, c - a),origin - a)) / dno;
//     float gamma = (dot(num,direction)) / dno;

//     if (beta > 0 && gamma > 0 && (1 - beta - gamma > 0) && t > tmin && t < h.getT())
//     {
//         Vector3f intersectPoint = (1 - beta - gamma) * a + beta * b + gamma * c;
//         h.set(t, this->getMaterial(), cross(b - a, c - b));
//         return true;
//     }

//     return false;
// }

Transform::Transform(const Matrix4f &m, Object3D *obj)
    : _object(obj), _inv(m.inverse()) {}

static auto trn(Matrix4f m, Vector3f v, float w = 0)
{
    return (m * Vector4f(v, w)).xyz();
}

bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    Ray tr(trn(_inv, r.getOrigin(), 1),
           trn(_inv, r.getDirection()).normalized());
    Hit th;
    tmin = (trn(_inv, r.pointAtParameter(tmin), 1) - tr.getOrigin()).abs();

    if (!_object->intersect(tr, tmin, th))
        return false;

    float t = (trn(_inv.inverse(), r.pointAtParameter(th.getT()), 1) - tr.getOrigin()).abs();

    h.set(t, th.getMaterial(), trn(_inv.transposed(), th.getNormal()).normalized());
    return true;
}
