#pragma once

#include <flucoma/data/FluidDataSet.hpp>
#include <flucoma/data/TensorTypes.hpp>

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <functional>
#include <optional>
#include <random>
#include <string>

/*
    The settings and steps that place points on a 2D map so that points with similar descriptors
    land near each other. train takes one row of descriptors per point and returns one row of two
    coordinates per point, in no fixed range; CorpusAnalysis scales them to 0 to 1.

    The method is UMAP, ported from FluCoMa's UMAP::train so that it can stop part-way. It links
    each point to its neighbourCount nearest points, weighting each link by how near they are,
    makes a starting layout from those links, and then over iterationCount rounds pulls linked
    points together and pushes randomly chosen points apart, in steps that shrink from
    learningRate toward zero. minimumDistance sets how tightly close points may pack. seed fixes
    the random choices; when it is empty, each run differs.

    CorpusAnalysis holds one as layout and runs it on CorpusLoader's worker thread. train asks
    shouldStop for every point while linking and on every round, and returns an empty matrix when
    it answers true. MapLayoutTests checks it against FluCoMa's UMAP::train with the same seed.
*/

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
