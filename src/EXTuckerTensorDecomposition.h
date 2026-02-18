

#include <Eigen/Dense>
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Core/util/Constants.h>
#include <Eigen/src/Core/util/Meta.h>
#include <Eigen/src/SVD/JacobiSVD.h>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <unsupported/Eigen/CXX11/Tensor>

struct TensorRank3
{
    std::vector<double> data;
    size_t xDim;
    size_t yDim;
    size_t zDim;

    public:
        TensorRank3() : xDim(0), yDim(0), zDim(0) {}
        TensorRank3(const std::vector<double>& data_, const std::array<size_t, 3> size)
            : data(data_)
            , xDim(size[0])
            , yDim(size[1])
            , zDim(size[2])
        {
            if (data.size() != xDim*yDim*zDim)
                throw std::runtime_error("Constructing TensorRank3 from Vector: Dimensions do not match data size!");
        }

        static TensorRank3 FromMatrix(const Eigen::MatrixXd& data, std::array<size_t, 3> size,  std::array<size_t, 3> permutation = {0, 1, 2})
        {
            if (data.size() != static_cast<Eigen::Index>(size[0]*size[1]*size[2]))
                throw std::runtime_error("Constructing TensorRank3 from Matrix: Dimensions do not match data size!");

            std::vector<double> buffer(data.size());
            if (permutation[0] == 0 && permutation[1] == 1 && permutation[2] == 2)
            {
                for (Eigen::Index k=0; k<data.size(); ++k)
                    buffer[k] = data.data()[k];
                return TensorRank3(buffer,size);
            }


            for (size_t z=0; z<size[2]; ++z)
                for (size_t y=0; y<size[1]; ++y)
                    for (size_t x=0; x<size[0]; ++x)
                    {
                        std::array<size_t, 3> pinds = {x,y,z};
                        size_t xp = pinds[permutation[0]];
                        size_t yp = pinds[permutation[1]];
                        size_t zp = pinds[permutation[2]];
                        size_t sx = size[permutation[0]];
                        size_t sy = size[permutation[1]];
                        buffer[xp + sx*(yp + sy*zp)] = data.data()[x+size[0]*(y+size[1]*z)];
                    }

            return TensorRank3(buffer, size);
        }

        size_t size() const
        {
            return data.size();
        }

        std::array<const size_t, 3> dims() const
        {
            return {xDim, yDim, zDim};
        }

        double at(const size_t x, const size_t y, const size_t z) const
        {
            if (x>=xDim || y>=yDim || z>=zDim)
                throw std::out_of_range("Indexing TensorRank3: Out of Bounds");
            return data[x + xDim * (y + yDim * z)];
        }

        double operator()(const size_t x, const size_t y, const size_t z) const
        {
            return data[x + xDim * (y + yDim * z)];
        }
};

class Tucker3D
{
    private:
        Eigen::MatrixXd U;              // I × r1
        Eigen::MatrixXd V;              // J × r2
        Eigen::MatrixXd W;              // K × r3
        TensorRank3 G;          // r1 × r2 × r3 core

        Eigen::MatrixXd unfoldDim1(const TensorRank3& data) const
        {
            auto dims = data.dims();
            // copy (safe). If you want zero-copy, don't return a Matrix.
            Eigen::MatrixXd M(dims[0], dims[1]*dims[2]);
            // Because of your storage, this is already in the correct order for mode-1:
            // column = y + J*z
            for (size_t z=0; z<dims[2]; ++z)
                for (size_t y=0; y<dims[1]; ++y)
                    for (size_t x=0; x<dims[0]; ++x)
                        M(x, y + dims[1]*z) = data(x,y,z);
            return M;
        }

        Eigen::MatrixXd unfoldDim2(const TensorRank3& data) const
        {
            auto dims = data.dims();
            Eigen::MatrixXd M(dims[1], dims[0]*dims[2]);
            for (size_t z=0; z<dims[2]; ++z)
                for (size_t y=0; y<dims[1]; ++y)
                    for (size_t x=0; x<dims[0]; ++x)
                        M(y, x + dims[0]*z) = data(x,y,z);
            return M;
        }

        Eigen::MatrixXd unfoldDim3(const TensorRank3& data) const
        {
            auto dims = data.dims();
            Eigen::MatrixXd M(dims[2], dims[0]*dims[1]);
            for (size_t z=0; z<dims[2]; ++z)
                for (size_t y=0; y<dims[1]; ++y)
                    for (size_t x=0; x<dims[0]; ++x)
                        M(z, x + dims[0]*y) = data(x,y,z);
            return M;
        }

        TensorRank3 mode_product(const TensorRank3& data, Eigen::MatrixXd& mat, const int mode) const
        {
            if (mode == 1)
            {
                Eigen::MatrixXd X = mat * unfoldDim1(data);
                size_t d1 = mat.rows();
                return TensorRank3::FromMatrix(X, {d1, data.yDim, data.zDim});
            }
            else if (mode == 2)
            {
                Eigen::MatrixXd X = mat * unfoldDim2(data);
                size_t d2 = mat.rows();
                return TensorRank3::FromMatrix(X, {d2, data.xDim, data.zDim}, {1,0,2});
            }
            else if (mode==3)
            {
                Eigen::MatrixXd X = mat * unfoldDim3(data);
                size_t d3 = mat.rows();
                return TensorRank3::FromMatrix(X, {d3, data.xDim, data.yDim}, {2,0,1});
            }
            else
            {
                throw std::runtime_error("Constructing Tucker TensorDecomp: Invalid dimension in mode_product");
            }
        }

        TensorRank3 mode_product(const TensorRank3& data, const Eigen::Transpose<Eigen::Matrix<double, -1, -1>>& mat, const int mode) const
        {
            if (mode == 1)
            {
                Eigen::MatrixXd X = mat * unfoldDim1(data);
                size_t d1 = mat.rows();
                return TensorRank3::FromMatrix(X, {d1, data.yDim, data.zDim});
            }
            else if (mode == 2)
            {
                Eigen::MatrixXd X = mat * unfoldDim2(data);
                size_t d2 = mat.rows();
                return TensorRank3::FromMatrix(X, {d2, data.xDim, data.zDim}, {1,0,2});
            }
            else if (mode==3)
            {
                Eigen::MatrixXd X = mat * unfoldDim3(data);
                size_t d3 = mat.rows();
                return TensorRank3::FromMatrix(X, {d3, data.xDim, data.yDim}, {2,0,1});
            }
            else
            {
                throw std::runtime_error("Constructing Tucker TensorDecomp: Invalid dimension in mode_product");
            }
        }

    public:

        Tucker3D(const std::vector<double>& data,
                 const std::array<size_t,3> dataSize,
                 const std::array<size_t, 3> decompositionRanks)
            : ranksXYZ(decompositionRanks)
            , originalTensorDims(dataSize)
        {

            const auto I = originalTensorDims[0];
            const auto J = originalTensorDims[1];
            const auto K = originalTensorDims[2];

            if (data.size() !=  I*J*K)
                throw std::runtime_error("Constructing Tucker TensorDecomp: data and given dimensions do not match!");

            TensorRank3 tData(data, dataSize);
            Eigen::JacobiSVD<Eigen::MatrixXd> svdD1_23(unfoldDim1(tData), Eigen::ComputeThinU);
            Eigen::JacobiSVD<Eigen::MatrixXd> svdD2_13(unfoldDim2(tData), Eigen::ComputeThinU);
            Eigen::JacobiSVD<Eigen::MatrixXd> svdD3_12(unfoldDim3(tData), Eigen::ComputeThinU);

            const  auto r1 = Eigen::Index(std::min(decompositionRanks[0], I));
            const  auto r2 = Eigen::Index(std::min(decompositionRanks[1], J));
            const  auto r3 = Eigen::Index(std::min(decompositionRanks[2], K));

            U = svdD1_23.matrixU().leftCols(r1);
            V = svdD2_13.matrixU().leftCols(r2);
            W = svdD3_12.matrixU().leftCols(r3);

            auto UT = U.transpose();
            auto VT = V.transpose();
            auto WT = W.transpose();

            auto G1 = mode_product(tData, UT, 1);
            auto G2 = mode_product(G1, VT, 2);
            this->G = mode_product(G2, WT, 3);
        }

        double interpolate(const size_t x, const size_t y, const size_t z) const
        {
            throw std::runtime_error("Not Implemented!");
        }

        double operator()(const size_t x, const size_t y, const size_t z) const
        {
            return this->interpolate(x, y, z);
        }

        const std::array<size_t, 3> ranksXYZ;
        const std::array<size_t, 3> originalTensorDims;
};



