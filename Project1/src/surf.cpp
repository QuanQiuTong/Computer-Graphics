#include "surf.h"
#include "vertexrecorder.h"
using namespace std;

namespace
{

    // We're only implenting swept surfaces where the profile curve is
    // flat on the xy-plane.  This is a check function.
    static bool checkFlat(const Curve &profile)
    {
        for (unsigned i = 0; i < profile.size(); i++)
            if (profile[i].V[2] != 0.0 ||
                profile[i].T[2] != 0.0 ||
                profile[i].N[2] != 0.0)
                return false;

        return true;
    }
}

// DEBUG HELPER
Surface quad()
{
    Surface ret;
    ret.VV.push_back(Vector3f(-1, -1, 0));
    ret.VV.push_back(Vector3f(+1, -1, 0));
    ret.VV.push_back(Vector3f(+1, +1, 0));
    ret.VV.push_back(Vector3f(-1, +1, 0));

    ret.VN.push_back(Vector3f(0, 0, 1));
    ret.VN.push_back(Vector3f(0, 0, 1));
    ret.VN.push_back(Vector3f(0, 0, 1));
    ret.VN.push_back(Vector3f(0, 0, 1));

    ret.VF.push_back(Tup3u(0, 1, 2));
    ret.VF.push_back(Tup3u(0, 2, 3));
    return ret;
}

void addTriangles(vector<Tup3u> &VF, size_t sweepSize, size_t curveSize)
{
    for (unsigned i = 0; i < (sweepSize - 1) * curveSize; i++)
    {
        if ((i + 1) % curveSize == 0)
            continue;
        VF.push_back({i, i + 1, i + curveSize});
        VF.push_back({i + 1, i + curveSize + 1, i + curveSize});
    }
    // same as:
    /* for (unsigned i = 0; i < sweepSize - 1; i++)
        for (unsigned j = 0; j < curveSize - 1; j++)
        {
            VF.push_back({i * curveSize + j, i * curveSize + j + 1, (i + 1) * curveSize + j});
            VF.push_back({i * curveSize + j + 1, (i + 1) * curveSize + j + 1, (i + 1) * curveSize + j});
        } */
}

Surface makeSurfRev(const Curve &profile, unsigned steps)
{
    if (!checkFlat(profile))
        throw "makeSurfRev: profile curve must be flat on xy plane.";

    constexpr float PI2 = 2 * 3.14159265358979323846f;
    Surface surface;
    for (unsigned j = 0; j <= steps; j++)
    {
        Matrix3f M = Matrix3f::rotateY(PI2 * j / steps);
        for (const auto &p : profile)
            surface.VV.push_back(M * p.V),
                surface.VN.push_back(M.inverse().transposed() * -p.N); //.normalized());
    }
    addTriangles(surface.VF, steps + 1, profile.size());
    return surface; // VV.size()  == profile.size() * (steps + 1)
}

Surface makeGenCyl(const Curve &profile, const Curve &sweep)
{
    if (!checkFlat(profile))
        throw "makeGenCyl: profile curve must be flat on xy plane.";

    Surface surface;
    for (unsigned i = 0; i < sweep.size(); i++)
    {
        Matrix4f M = {{sweep[i].N, 0}, {sweep[i].B, 0}, {sweep[i].T, 0}, {sweep[i].V, 1}};
        for (const auto &p : profile)
            surface.VV.push_back((M * Vector4f(p.V, 1)).xyz()),
                surface.VN.push_back(-(M * Vector4f(p.N, 0)).xyz()); //.normalized();
    }
    addTriangles(surface.VF, sweep.size(), profile.size());
    return surface;
}

void recordSurface(const Surface &surface, VertexRecorder *recorder)
{
    const Vector3f WIRECOLOR(0.4f, 0.4f, 0.4f);
    for (int i = 0; i < (int)surface.VF.size(); i++)
    {
        recorder->record(surface.VV[surface.VF[i][0]], surface.VN[surface.VF[i][0]], WIRECOLOR);
        recorder->record(surface.VV[surface.VF[i][1]], surface.VN[surface.VF[i][1]], WIRECOLOR);
        recorder->record(surface.VV[surface.VF[i][2]], surface.VN[surface.VF[i][2]], WIRECOLOR);
    }
}

void recordNormals(const Surface &surface, VertexRecorder *recorder, float len)
{
    const Vector3f NORMALCOLOR(0, 1, 1);
    for (int i = 0; i < (int)surface.VV.size(); i++)
    {
        recorder->record_poscolor(surface.VV[i], NORMALCOLOR);
        recorder->record_poscolor(surface.VV[i] + surface.VN[i] * len, NORMALCOLOR);
    }
}

void outputObjFile(ostream &out, const Surface &surface)
{

    for (int i = 0; i < (int)surface.VV.size(); i++)
        out << "v  "
            << surface.VV[i][0] << " "
            << surface.VV[i][1] << " "
            << surface.VV[i][2] << endl;

    for (int i = 0; i < (int)surface.VN.size(); i++)
        out << "vn "
            << surface.VN[i][0] << " "
            << surface.VN[i][1] << " "
            << surface.VN[i][2] << endl;

    out << "vt  0 0 0" << endl;

    for (int i = 0; i < (int)surface.VF.size(); i++)
    {
        out << "f  ";
        for (unsigned j = 0; j < 3; j++)
        {
            unsigned a = surface.VF[i][j] + 1;
            out << a << "/"
                << "1"
                << "/" << a << " ";
        }
        out << endl;
    }
}
