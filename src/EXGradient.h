
#pragma once

#include <QVector>
#include <QVector3D>
#include <qvector.h>

struct EXGradientColor
{
    public:
        EXGradientColor()
        : m_color(0, 0, 0)
        , m_positionOnGradient(-1)
        {}
        EXGradientColor(QVector3D color, float positionOnGradient)
        : m_color(color)
        , m_positionOnGradient(positionOnGradient)
        {}

        QVector3D m_color;
        float m_positionOnGradient; // between 0 and 1
};

struct EXColorGradient
{
    public:
        EXColorGradient() { m_colors = new QVector<EXGradientColor>; };
        EXColorGradient(std::vector<QVector3D> colors, std::vector<float> positionsOnGradient) {
            if (colors.size() != positionsOnGradient.size()) {
                throw std::length_error("In EXColorGradient, length of colors did not match length of positionsOnGradient");
            }

            size_t l = colors.size();
            m_colors = new QVector<EXGradientColor>(); m_colors->reserve(l);
            for (size_t k=0; k<l; ++k) { m_colors->push_back(EXGradientColor(colors[k], positionsOnGradient[k])); }
        }
        ~EXColorGradient() {}
    QVector<EXGradientColor>* m_colors;

    void insert(EXGradientColor new_color) {
        if (m_colors->empty()) { m_colors->push_back(new_color); }
        else {
            for (int k=0; k<m_colors->size(); ++k) {
                if (m_colors->at(k).m_positionOnGradient > new_color.m_positionOnGradient) {
                    m_colors->insert(k, new_color); break;
                }
            }
        }
    }

    void remove(int pos) {
        if (pos<m_colors->size()) {
            m_colors->remove(pos);
        }
    }

    void remove(EXGradientColor color) {
        for (int k=0; k<m_colors->size(); ++k) {
            if (color.m_positionOnGradient == m_colors->at(k).m_positionOnGradient) {
                if (color.m_color == m_colors->at(k).m_color) { m_colors->remove(k); break; }
            }
            if (color.m_positionOnGradient > m_colors->at(k).m_positionOnGradient) {
                break;
            }
        }
    }
};

