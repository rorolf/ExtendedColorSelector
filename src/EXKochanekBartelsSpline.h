


// KBSpline.h
#pragma once

#include <QVector3D>
#include <vector>
#include <array>
#include <algorithm>
//#include <stdexcept>
//#include <cmath>

#include "EXUtils.h"
//#include "EXGradient.h"

struct EXGradientColor;

class EXKBSpline
{
public:
    // Per segment coefficients for C(u) = a*u^3 + b*u^2 + c*u + d, u in [0,1]
    using Coeff4 = std::array<QVector3D, 4>;

    EXKBSpline(const QVector<EXGradientColor>& points,
             float tension = -0.3f,
             float continuity = 0.0f,
             float bias = 0.0f)
    {
        init(points, tension, continuity, bias);
    }

    void init(const QVector<EXGradientColor>& points,
              float tension = -0.3f,
              float continuity = 0.0f,
              float bias = 0.0f)
    {
        const int n = static_cast<int>(points.size());
        exAssert(n>=2, "EXKBSpline: need at least 2 points.");

        m_points = QVector<EXGradientColor>();
        m_points.reserve(n);
        for (EXGradientColor newClr : points) { m_points.push_back(newClr); }

        std::sort(m_points.begin(), m_points.end(), [](EXGradientColor& a, EXGradientColor& b) {
            return a.m_positionOnGradient < b.m_positionOnGradient; // default ascending sort uses '<'
        });

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
            const float t0 = m_points[i].m_positionOnGradient;
            const float t1 = m_points[i + 1].m_positionOnGradient;
            const float dt = t1 - t0;

            // Convert d/dt -> d/du by multiplying by dt (u = (t-t0)/dt)
            const QVector3D M0 = m_mPlus[i]    * dt;      // outgoing at Pi
            const QVector3D M1 = m_mMinus[i+1] * dt;      // incoming at P_{i+1}

            m_coeffs[i] = hermiteCoeffs(m_points[i].m_color, m_points[i + 1].m_color, M0, M1);
        }
    }

    // Evaluate at time tquery. Clamps outside range to endpoints.
    QVector3D operator()(float tquery) const
    {
        if (m_points.empty()) {
            return QVector3D();
        }
        if (m_points.size() == 1) {
            return m_points[0].m_color;
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

    const QVector<EXGradientColor>& points() const { return m_points; }
    const QVector<Coeff4>&    coeffs() const { return m_coeffs; }

private:
    // Mirror endpoint accessor: i in [0..n-1] is real, i=-1 or i=n is mirrored
    void mirroredPointTime(int i, QVector3D& P, float& t) const
    {
        const int n = static_cast<int>(m_points.size());
        if (i >= 0 && i < n) {
            P = m_points[i].m_color;
            t = m_points[i].m_positionOnGradient;
            return;
        }

        exAssert((-1<=i) && (i<=n), "EXKBSpline: mirroredPointTime only supports i=-1 and i=n for mirroring.");
        if (i <= -1) {
            // P[-1] = 2P0 - P1, t[-1] = 2t0 - t1
            P = (m_points[0].m_color * 2.f) - m_points[1].m_color;
            t = (m_points[0].m_positionOnGradient  * 2.f) - m_points[1].m_positionOnGradient;
            return;
        }
        if (i <= n) {
            // P[n] = 2P_{n-1} - P_{n-2}, t[n] = 2t_{n-1} - t_{n-2}
            P = (m_points[n - 1].m_color * 2.f) - m_points[n - 2].m_color;
            t = (m_points[n - 1].m_positionOnGradient  * 2.f) - m_points[n - 2].m_positionOnGradient;
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

        exAssert((dt0 != 0.f) && (dt1 != 0.f), "EXKBSpline: duplicate time detected (dt==0) in kbTangentsAt.");

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
        const int n = static_cast<int>(m_points.size());

        if (tquery <= m_points.front().m_positionOnGradient) {
            segOut = 0;
            uOut = 0.f;
            return;
        }
        if (tquery >= m_points.back().m_positionOnGradient) {
            segOut = n - 2;
            uOut = 1.f;
            return;
        }

        // Find last index i such that times[i] <= tquery
        auto it = std::upper_bound(m_points.begin(), m_points.end(), tquery, [](EXGradientColor& a, float b) {
            return a.m_positionOnGradient < b;
        });
        int i = static_cast<int>(std::distance(m_points.begin(), it)) - 1;
        if (i < 0) i = 0;
        if (i > n - 2) i = n - 2;

        const float t0 = m_points[i].m_positionOnGradient;
        const float t1 = m_points[i + 1].m_positionOnGradient;
        const float dt = t1 - t0;

        segOut = i;
        uOut = (dt > 0.f) ? (tquery - t0) / dt : 0.f;
        // numeric safety
        if (uOut < 0.f) uOut = 0.f;
        if (uOut > 1.f) uOut = 1.f;
    }

private:
    // Inputs / keyframes
    QVector<EXGradientColor> m_points;

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
    QVector<Coeff4> m_coeffs;
};


