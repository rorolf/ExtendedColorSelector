


// KBSpline.h
#ifndef EXKBSPLINE_H
#define EXKBSPLINE_K

#include <QVector3D>
#include <vector>
#include <array>
#include <algorithm>
//#include <stdexcept>
//#include <cmath>

#include "EXUtils.h"

class EXKBSpline
{
public:
    // Per segment coefficients for C(u) = a*u^3 + b*u^2 + c*u + d, u in [0,1]
    using Coeff4 = std::array<QVector3D, 4>;

    EXKBSpline(const std::vector<QVector3D>& points,
             const std::vector<float>& times,
             float tension = -0.3f,
             float continuity = 0.0f,
             float bias = 0.0f)
    {
        init(points, times, tension, continuity, bias);
    }

    void init(const std::vector<QVector3D>& points,
              const std::vector<float>& times,
              float tension = -0.3f,
              float continuity = 0.0f,
              float bias = 0.0f)
    {
        const int n = static_cast<int>(points.size());
        exAssert(n>=2, "EXKBSpline: need at least 2 points.");
        exAssert(static_cast<int>(times.size()) == n, "EXKBSpline: points and times must have same length.");

        // Validate strictly increasing times
        for (int i = 1; i < n; ++i) {
            exAssert((times[i - 1] < times[i]), "EXKBSpline: times must be strictly increasing.");
        }

        m_points = points;
        m_times  = times;
        m_T = tension;
        m_C = continuity;
        m_B = bias;

        // Precompute per-keyframe tangents (time derivatives): outgoing mPlus[i], incoming mMinus[i]
        m_mPlus.resize(n);
        m_mMinus.resize(n);
        for (int i = 0; i < n; ++i) {
            QVector3D mp, mm;
            kbTangentsAt(i, mp, mm);
            m_mPlus[i]  = mp;
            m_mMinus[i] = mm;
        }

        // Hermite basis matrix
        // rows correspond to a,b,c,d
        m_hermiteH = {{
            {{  2.f, -2.f,  1.f,  1.f }},
            {{ -3.f,  3.f, -2.f, -1.f }},
            {{  0.f,  0.f,  1.f,  0.f }},
            {{  1.f,  0.f,  0.f,  0.f }}
        }};

        // Precompute segment coefficients
        m_coeffs.clear();
        m_coeffs.resize(n - 1);

        for (int i = 0; i < n - 1; ++i) {
            const float t0 = m_times[i];
            const float t1 = m_times[i + 1];
            const float dt = t1 - t0;

            // Convert d/dt -> d/du by multiplying by dt (u = (t-t0)/dt)
            const QVector3D M0 = m_mPlus[i]    * dt;      // outgoing at Pi
            const QVector3D M1 = m_mMinus[i+1] * dt;      // incoming at P_{i+1}

            m_coeffs[i] = hermiteCoeffs(m_points[i], m_points[i + 1], M0, M1);
        }
    }

    // Evaluate at time tquery. Clamps outside range to endpoints.
    QVector3D operator()(float tquery) const
    {
        if (m_times.empty()) {
            return QVector3D();
        }
        if (m_times.size() == 1) {
            return m_points[0];
        }

        int seg = 0;
        float u = 0.f;
        segmentIndexAndU(tquery, seg, u);

        const Coeff4& c = m_coeffs[seg];
        const QVector3D& a = c[0];
        const QVector3D& b = c[1];
        const QVector3D& cc = c[2];
        const QVector3D& d = c[3];

        // Horner: (((a*u)+b)*u + c)*u + d
        return (((a * u) + b) * u + cc) * u + d;
    }

    const std::vector<float>&     times()  const { return m_times; }
    const std::vector<QVector3D>& points() const { return m_points; }
    const std::vector<Coeff4>&    coeffs() const { return m_coeffs; }

private:
    // Mirror endpoint accessor: i in [0..n-1] is real, i=-1 or i=n is mirrored
    void mirroredPointTime(int i, QVector3D& P, float& t) const
    {
        const int n = static_cast<int>(m_points.size());
        if (i >= 0 && i < n) {
            P = m_points[i];
            t = m_times[i];
            return;
        }

        exAssert(-1<=i && i<=n, "EXKBSpline: mirroredPointTime only supports i=-1 and i=n for mirroring.");
        if (i <= -1) {
            // P[-1] = 2P0 - P1, t[-1] = 2t0 - t1
            P = (m_points[0] * 2.f) - m_points[1];
            t = (m_times[0]  * 2.f) - m_times[1];
            return;
        }
        if (i <= n) {
            // P[n] = 2P_{n-1} - P_{n-2}, t[n] = 2t_{n-1} - t_{n-2}
            P = (m_points[n - 1] * 2.f) - m_points[n - 2];
            t = (m_times[n - 1]  * 2.f) - m_times[n - 2];
            return;
        }
    }

    // Compute KB outgoing/incoming tangents at keyframe i (0-based), derivative w.r.t time
    void kbTangentsAt(int i, QVector3D& mPlus, QVector3D& mMinus) const
    {
        QVector3D Pim1, Pi, Pip1;
        float tim1 = 0.f, ti = 0.f, tip1 = 0.f;

        mirroredPointTime(i - 1, Pim1, tim1);
        mirroredPointTime(i,     Pi,   ti);
        mirroredPointTime(i + 1, Pip1, tip1);

        const float dt0 = ti   - tim1;
        const float dt1 = tip1 - ti;

        exAssert(dt0 != 0.f && dt1 != 0.f, "EXKBSpline: duplicate time detected (dt==0) in kbTangentsAt.");

        const QVector3D d0 = (Pi   - Pim1) / dt0; // left secant
        const QVector3D d1 = (Pip1 - Pi)   / dt1; // right secant

        const float k = 1.f - m_T;

        // Classic Kochanek–Bartels weights
        const float w0p = k * (1.f + m_C) * (1.f + m_B) * 0.5f;
        const float w1p = k * (1.f - m_C) * (1.f - m_B) * 0.5f;
        mPlus = d0 * w0p + d1 * w1p;

        const float w0m = k * (1.f - m_C) * (1.f + m_B) * 0.5f;
        const float w1m = k * (1.f + m_C) * (1.f - m_B) * 0.5f;
        mMinus = d0 * w0m + d1 * w1m;
    }

    // Multiply Hermite basis matrix by geometry vector [P0, P1, M0, M1]
    // and return (a,b,c,d) such that C(u) = a u^3 + b u^2 + c u + d.
    Coeff4 hermiteCoeffs(const QVector3D& P0, const QVector3D& P1,
                         const QVector3D& M0, const QVector3D& M1) const
    {
        auto lincomb = [&](const std::array<float,4>& row) -> QVector3D {
            return P0 * row[0] + P1 * row[1] + M0 * row[2] + M1 * row[3];
        };

        Coeff4 out;
        out[0] = lincomb(m_hermiteH[0]); // a
        out[1] = lincomb(m_hermiteH[1]); // b
        out[2] = lincomb(m_hermiteH[2]); // c
        out[3] = lincomb(m_hermiteH[3]); // d
        return out;
    }

    void segmentIndexAndU(float tquery, int& segOut, float& uOut) const
    {
        const int n = static_cast<int>(m_times.size());

        if (tquery <= m_times.front()) {
            segOut = 0;
            uOut = 0.f;
            return;
        }
        if (tquery >= m_times.back()) {
            segOut = n - 2;
            uOut = 1.f;
            return;
        }

        // Find last index i such that times[i] <= tquery
        auto it = std::upper_bound(m_times.begin(), m_times.end(), tquery);
        int i = static_cast<int>(std::distance(m_times.begin(), it)) - 1;
        if (i < 0) i = 0;
        if (i > n - 2) i = n - 2;

        const float t0 = m_times[i];
        const float t1 = m_times[i + 1];
        const float dt = t1 - t0;

        segOut = i;
        uOut = (dt > 0.f) ? (tquery - t0) / dt : 0.f;
        // numeric safety
        if (uOut < 0.f) uOut = 0.f;
        if (uOut > 1.f) uOut = 1.f;
    }

private:
    // Inputs / keyframes
    std::vector<QVector3D> m_points;
    std::vector<float>     m_times;

    // KB parameters
    float m_T = 0.f;
    float m_C = 0.f;
    float m_B = 0.f;

    // Precomputed tangents (time derivatives) at keyframes
    std::vector<QVector3D> m_mPlus;   // outgoing at i
    std::vector<QVector3D> m_mMinus;  // incoming at i

    // Hermite basis matrix (4x4) stored as rows
    std::array<std::array<float,4>,4> m_hermiteH;

    // Per-segment cubic coefficients
    std::vector<Coeff4> m_coeffs;
};


#endif //EXKBSPLINE_H
