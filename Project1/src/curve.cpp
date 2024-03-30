#include "curve.h"
#include "vertexrecorder.h"
using namespace std;

const float c_pi = 3.14159265358979323846f;

namespace
{
	// Approximately equal to.  We don't want to use == because of
	// precision issues with floating point.
	inline bool approx(const Vector3f &lhs, const Vector3f &rhs)
	{
		const float eps = 1e-8f;
		return (lhs - rhs).absSquared() < eps;
	}

}

Matrix3f rotate(const Vector3f &axis, float theta)
{
	float c = cos(theta), s = sin(theta), t = 1 - c, x = axis[0], y = axis[1], z = axis[2];
	return {
		{t * x * x + c, t * x * y - s * z, t * x * z + s * y},
		{t * x * y + s * z, t * y * y + c, t * y * z - s * x},
		{t * x * z - s * y, t * y * z + s * x, t * z * z + c}};
}

Curve &fixClosed(Curve &curve)
{
	// If curve is not closed or need not fix, return it as is.
	if (!approx(curve.front().V, curve.back().V) || approx(curve.front().N, curve.back().N))
		return curve;

	float theta = acos(Vector3f::dot(curve.front().N, curve.back().N));
	for (size_t i = 0; i < curve.size(); i++)
	{
		Matrix3f M = rotate(curve[i].T, theta * i / curve.size());
		curve[i].N = M * curve[i].N;
		curve[i].B = M * curve[i].B;
	}

	return curve;
}

Curve evalBezier(const vector<Vector3f> &P, unsigned steps)
{
	if (P.size() < 4 || P.size() % 3 != 1)
	{
		cerr << "evalBezier must be called with 3n+1 control points." << endl;
		exit(0);
	}

	Curve R(steps + 1);
	Vector3f B(0, 0, 1); // Vector3f::cross(Vector3f(1, 0, 0), P[1] - P[0]);

	for (size_t i = 0, numCurves = (P.size() - 1) / 3; i < numCurves; ++i)
	{
		const Vector3f &P0 = P[i * 3];
		const Vector3f &P1 = P[i * 3 + 1];
		const Vector3f &P2 = P[i * 3 + 2];
		const Vector3f &P3 = P[i * 3 + 3];

		for (unsigned j = 0; j <= steps; ++j)
		{
			float t = static_cast<float>(j) / steps;
			float t2 = t * t;
			float t3 = t2 * t;

			CurvePoint &point = R[i * steps + j];
			point.V = (1 - 3 * t + 3 * t2 - t3) * P0 +
					  (3 * t - 6 * t2 + 3 * t3) * P1 +
					  (3 * t2 - 3 * t3) * P2 +
					  t3 * P3;

			point.T = ((-3 + 6 * t - 3 * t2) * P0 +
					   (3 - 12 * t + 9 * t2) * P1 +
					   (6 * t - 9 * t2) * P2 +
					   3 * t2 * P3)
						  .normalized();

			point.N = Vector3f::cross(B, point.T).normalized();
			point.B = B = Vector3f::cross(point.T, point.N);
		}
	}

	cerr << "\t>>> evalBezier has been called with the following input:" << endl;
	cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
	for (const auto &p : P)
		cerr << "\t>>> " << p << endl;
	cerr << "\t>>> Steps (type steps): " << steps << endl;
	cerr << "\t>>> Returning curve." << endl;

	return fixClosed(R);
}

Curve evalBspline(const vector<Vector3f> &P, unsigned steps)
{
	if (P.size() < 4)
	{
		cerr << "evalBspline must be called with 4 or more control points." << endl;
		exit(0);
	}

	Curve R;
	Vector3f B(0, 0, 1); // Vector3f::cross(Vector3f(1, 0, 0), P[1] - P[0]);

	for (size_t i = 0, numCurves = P.size() - 3; i < numCurves; ++i)
	{
		const Vector3f &P0 = P[i];
		const Vector3f &P1 = P[i + 1];
		const Vector3f &P2 = P[i + 2];
		const Vector3f &P3 = P[i + 3];

		for (unsigned j = 0; j <= steps; ++j)
		{
			float t = static_cast<float>(j) / steps;
			float t2 = t * t;
			float t3 = t2 * t;

			CurvePoint point;
			point.V = (1.0f / 6.0f) * ((-t3 + 3 * t2 - 3 * t + 1) * P0 +
									   (3 * t3 - 6 * t2 + 4) * P1 +
									   (-3 * t3 + 3 * t2 + 3 * t + 1) * P2 +
									   t3 * P3);

			point.T = ((-t2 + 2 * t - 1) * P0 +
					   (3 * t2 - 4 * t) * P1 +
					   (-3 * t2 + 2 * t + 1) * P2 +
					   t2 * P3)
						  .normalized();

			point.N = Vector3f::cross(B, point.T).normalized();
			point.B = B = Vector3f::cross(point.T, point.N);

			R.push_back(point);
		}
	}

	cerr << "\t>>> evalBSpline has been called with the following input:" << endl;
	cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
	for (const auto &p : P)
		cerr << "\t>>> " << p << endl;
	cerr << "\t>>> Steps (type steps): " << steps << endl;
	cerr << "\t>>> Returning curve." << endl;

	return fixClosed(R);
}

Curve evalCircle(float radius, unsigned steps)
{
	// This is a sample function on how to properly initialize a Curve
	// (which is a vector< CurvePoint >).

	// Preallocate a curve with steps+1 CurvePoints
	Curve R(steps + 1);

	// Fill it in counterclockwise
	for (unsigned i = 0; i <= steps; ++i)
	{
		// step from 0 to 2pi
		float t = 2.0f * c_pi * float(i) / steps;

		// Initialize position
		// We're pivoting counterclockwise around the y-axis
		R[i].V = radius * Vector3f(cos(t), sin(t), 0);

		// Tangent vector is first derivative
		R[i].T = Vector3f(-sin(t), cos(t), 0);

		// Normal vector is second derivative
		R[i].N = Vector3f(-cos(t), -sin(t), 0);

		// Finally, binormal is facing up.
		R[i].B = Vector3f(0, 0, 1);
	}

	return R;
}

void recordCurve(const Curve &curve, VertexRecorder *recorder)
{
	const Vector3f WHITE(1, 1, 1);
	for (int i = 0; i < (int)curve.size() - 1; ++i)
	{
		recorder->record_poscolor(curve[i].V, WHITE);
		recorder->record_poscolor(curve[i + 1].V, WHITE);
	}
}
void recordCurveFrames(const Curve &curve, VertexRecorder *recorder, float framesize)
{
	Matrix4f T;
	const Vector3f RED(1, 0, 0);
	const Vector3f GREEN(0, 1, 0);
	const Vector3f BLUE(0, 0, 1);

	const Vector4f ORGN(0, 0, 0, 1);
	const Vector4f AXISX(framesize, 0, 0, 1);
	const Vector4f AXISY(0, framesize, 0, 1);
	const Vector4f AXISZ(0, 0, framesize, 1);

	for (int i = 0; i < (int)curve.size(); ++i)
	{
		T.setCol(0, Vector4f(curve[i].N, 0));
		T.setCol(1, Vector4f(curve[i].B, 0));
		T.setCol(2, Vector4f(curve[i].T, 0));
		T.setCol(3, Vector4f(curve[i].V, 1));

		// Transform orthogonal frames into model space
		Vector4f MORGN = T * ORGN;
		Vector4f MAXISX = T * AXISX;
		Vector4f MAXISY = T * AXISY;
		Vector4f MAXISZ = T * AXISZ;

		// Record in model space
		recorder->record_poscolor(MORGN.xyz(), RED);
		recorder->record_poscolor(MAXISX.xyz(), RED);

		recorder->record_poscolor(MORGN.xyz(), GREEN);
		recorder->record_poscolor(MAXISY.xyz(), GREEN);

		recorder->record_poscolor(MORGN.xyz(), BLUE);
		recorder->record_poscolor(MAXISZ.xyz(), BLUE);
	}
}
