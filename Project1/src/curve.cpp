#include "curve.h"
#include "vertexrecorder.h"
using namespace std;

const float c_pi = 3.14159265358979323846f;

namespace
{
	// Approximately equal to.  We don't want to use == because of
	// precision issues with floating
	inline bool approx(const Vector3f &lhs, const Vector3f &rhs)
	{
		const float eps = 1e-8f;
		return (lhs - rhs).absSquared() < eps;
	}

}

Curve &fixClosed(Curve &curve)
{
	// If curve is not closed or need not fixing, return it as is.
	if (!approx(curve.front().V, curve.back().V) || approx(curve.front().N, curve.back().N))
		return curve;

	float t = acos(Vector3f::dot(curve.front().N, curve.back().N)) / curve.size();
	for (size_t i = 0; i < curve.size(); i++)
	{
		curve[i].N = cos(t * i) * curve[i].N - sin(t * i) * curve[i].B;
		curve[i].B = Vector3f::cross(curve[i].T, curve[i].N);
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

	Curve R;
	Vector3f B(0, 0, 1);

	for (size_t i = 0, numCurves = (P.size() - 1) / 3; i < numCurves; ++i)
		for (unsigned j = 0; j <= steps; ++j)
		{
			float t = float(j) / steps, t2 = t * t, t3 = t2 * t;

			Vector3f V = (1 - 3 * t + 3 * t2 - t3) * P[i * 3] +
						 (3 * t - 6 * t2 + 3 * t3) * P[i * 3 + 1] +
						 (3 * t2 - 3 * t3) * P[i * 3 + 2] +
						 t3 * P[i * 3 + 3],

					 T = ((-3 + 6 * t - 3 * t2) * P[i * 3] +
						  (3 - 12 * t + 9 * t2) * P[i * 3 + 1] +
						  (6 * t - 9 * t2) * P[i * 3 + 2] +
						  3 * t2 * P[i * 3 + 3])
							 .normalized(),

					 N = Vector3f::cross(B, T).normalized();

			B = Vector3f::cross(T, N);

			R.push_back({V, T, N, B});
		}

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
	Vector3f B(0, 0, 1);

	for (size_t i = 0, numCurves = P.size() - 3; i < numCurves; ++i)
		for (unsigned j = 0; j <= steps; ++j)
		{
			float t = float(j) / steps, t2 = t * t, t3 = t2 * t;

			Vector3f V = (1.0f / 6.0f) * ((-t3 + 3 * t2 - 3 * t + 1) * P[i] +
										  (3 * t3 - 6 * t2 + 4) * P[i + 1] +
										  (-3 * t3 + 3 * t2 + 3 * t + 1) * P[i + 2] +
										  t3 * P[i + 3]),

					 T = ((-t2 + 2 * t - 1) * P[i] +
						  (3 * t2 - 4 * t) * P[i + 1] +
						  (-3 * t2 + 2 * t + 1) * P[i + 2] +
						  t2 * P[i + 3])
							 .normalized(),

					 N = Vector3f::cross(B, T).normalized();

			B = Vector3f::cross(T, N);

			R.push_back({V, T, N, B});
		}

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
