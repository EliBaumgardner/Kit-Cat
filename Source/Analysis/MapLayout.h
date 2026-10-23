#pragma once

#include <flucoma/data/FluidDataSet.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <functional>
#include <optional>
#include <random>
#include <string>

class MapLayout
{
public:
    using DataSet = fluid::FluidDataSet<std::string, double, 1>;

    fluid::RealMatrix train (const DataSet& descriptors, const std::function<bool()>& shouldStop) const;

    int neighbourCount = 15;
    int iterationCount = 200;

    double minimumDistance = 0.1;
    double learningRate    = 1.0;

    std::optional<unsigned int> seed;

private:
    Eigen::SparseMatrix<double> buildGraph (const DataSet& descriptors, fluid::index neighbours, const std::function<bool()>& shouldStop) const;

    Eigen::ArrayXd  findSigma          (const Eigen::ArrayXXd& distances, fluid::index neighbours) const;
    Eigen::ArrayXXd normalizeEmbedding (const Eigen::ArrayXXd& embedding, std::mt19937_64& random) const;

    void optimizeLayout (Eigen::ArrayXXd& embedding, const Eigen::ArrayXi& heads, const Eigen::ArrayXi& tails, const Eigen::ArrayXd& epochsPerSample, const Eigen::VectorXd& ab,
                         std::mt19937_64& random, const std::function<bool()>& shouldStop) const;

    struct AbCurve
    {
        AbCurve (double minimumDistance);

        int operator() (const Eigen::VectorXd& ab, Eigen::VectorXd& residuals) const;
        int values     () const;
        int inputs     () const;

        using Scalar       = double;
        using InputType    = Eigen::VectorXd;
        using ValueType    = Eigen::VectorXd;
        using JacobianType = Eigen::MatrixXd;

        enum
        {
            InputsAtCompileTime = 2,
            ValuesAtCompileTime = 300
        };

        Eigen::ArrayXd distances;
        Eigen::ArrayXd targets;
    };

    using MapPoint = Eigen::Array<double, 1, 2>;

    static constexpr fluid::index dimensionCount = 2;

    static constexpr int sigmaIterationCount = 64;

    static constexpr double sigmaTolerance     = 1e-5;
    static constexpr double negativeSampleRate = 5.0;
    static constexpr double gradientBound      = 4.0;
};
