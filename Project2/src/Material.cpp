#include "Material.h"

static float clamp(const Vector3f &l, const Vector3f &r)
{
    float dot = Vector3f::dot(l, r);
    return dot > 0 ? dot : 0;
}

Vector3f Material::shade(const Ray &ray,
                         const Hit &hit,
                         const Vector3f &dirToLight,
                         const Vector3f &lightIntensity)
{
    Vector3f normal = hit.getNormal(), L = dirToLight;
    Vector3f V = -ray.getDirection(), R = normal * 2.0f * Vector3f::dot(L, normal) - L;

    return lightIntensity * (clamp(L, normal) * _diffuseColor +
                             pow(clamp(R, V), _shininess) * _specularColor);
    // pow(clamp(normal, (V+L).normalized()), _shininess) * _specularColor);
}
/*
当给定光线方向 L 和屏幕法
向量 N ，当 L·N 小于 0 时，光源在切平面以下，不计算漫反射。
结合漫反射材料的反射率
k_diffuse 和光强 L ，漫反射的强度为
注意每个
RGB 通道单独计算。 k_diffuse 由 Material 类的 _diffuseColor 定义。
*/

/*
镜面反射强度取决于：光泽度 s 、物体
表面对相机的方向 E 、理想反射 矢量 R 、对光的方向 L 、表面法线 N 和反射率 k_specular 。镜面
反射 项的公式为：
这个公式与漫反射相似，不同点在于使用
理想反射矢量 R 和 L 的夹角计算光强 ，这能使得高光
随着相机的移动而移动；此外， 计算夹角的 s 次幂 ，较高的光泽 s 使高亮部分更窄，表面显得
更有光泽，较小的 s 使表面显得更有哑光外观。 k_specular 由 Material 类的 _specularColor 定义。
*/