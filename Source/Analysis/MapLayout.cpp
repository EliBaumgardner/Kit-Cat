#include "Analysis/MapLayout.h"

#include <flucoma/algorithms/public/KDTree.hpp>
#include <flucoma/algorithms/util/AlgorithmUtils.hpp>
#include <flucoma/algorithms/util/SpectralEmbedding.hpp>

#include <unsupported/Eigen/NonLinearOptimization>
#include <unsupported/Eigen/NumericalDiff>

#include <algorithm>
#include <cmath>

fluid::RealMatrix MapLayout::train(const DataSet& descriptors, const std::function<bool()>& shouldStop) const
{
    using AbDifference = Eigen::NumericalDiff<AbCurve>;

    const fluid::index neighbours = std::min(static_cast<fluid::index>(neighbourCount), descriptors.size() - 1);

    std::random_device device;
    std::mt19937_64    random(seed.value_or(device()));

    AbCurve                                 abCurve(minimumDistance);
    AbDifference                            abDifference(abCurve);

    Eigen::LevenbergMarquardt<AbDifference> abFit(abDifference);
    Eigen::VectorXd                         ab = Eigen::VectorXd::Ones(2);

    fluid::algorithm::SpectralEmbedding spectralEmbedding;

    Eigen::SparseMatrix<double>         graph = buildGraph(descriptors, neighbours, shouldStop);
    Eigen::ArrayXXd                     embedding;

    Eigen::ArrayXi heads;
    Eigen::ArrayXi tails;
    Eigen::ArrayXd epochsPerSample;

    double         maximumWeight = 0.0;
    fluid::index   entry         = 0;

    if (shouldStop()) {
        return {};
    }

    abFit.minimize(ab);

    embedding = normalizeEmbedding(spectralEmbedding.train(graph, dimensionCount), random);

    graph.makeCompressed();

    heads.resize(graph.nonZeros());
    tails.resize(graph.nonZeros());
    epochsPerSample.resize(graph.nonZeros());

    maximumWeight = graph.coeffs().maxCoeff();

    for (fluid::index outer = 0; outer < graph.outerSize(); ++outer) {
        for (Eigen::SparseMatrix<double>::InnerIterator edge(graph, outer); edge; ++edge) {
            heads(entry)           = static_cast<int>(edge.row());
            tails(entry)           = static_cast<int>(edge.col());
            epochsPerSample(entry) = 1.0 / (edge.value() / maximumWeight);
            entry                 += 1;
        }
    }

    epochsPerSample = (epochsPerSample == 0).select(-1, epochsPerSample);

    optimizeLayout(embedding, heads, tails, epochsPerSample, ab, random, shouldStop);

    if (shouldStop()) {
        return {};
    }

    return fluid::RealMatrix(fluid::algorithm::_impl::asFluid(embedding));
}

Eigen::SparseMatrix<double> MapLayout::buildGraph(const DataSet& descriptors, fluid::index neighbours, const std::function<bool()>& shouldStop) const
{
    const fluid::index pointCount = descriptors.size();
    const auto         data       = descriptors.getData();

    fluid::FluidTensor<std::string, 1> pointIds(pointCount);
    fluid::algorithm::KDTree           tree;

    Eigen::SparseMatrix<double> graph(pointCount, pointCount);
    Eigen::SparseMatrix<double> transposed;
    Eigen::ArrayXXd             distances = Eigen::ArrayXXd::Zero(pointCount, neighbours);
    Eigen::ArrayXd              sigma;

    for (fluid::index point = 0; point < pointCount; ++point) {
        pointIds(point) = std::to_string(point);
    }

    tree = fluid::algorithm::KDTree(DataSet(pointIds, data));

    graph.reserve(pointCount * neighbours);

    for (fluid::index point = 0; point < pointCount && ! shouldStop(); ++point) {
        auto [nearestDistances, nearestIds] = tree.kNearest(data.row(point), neighbours + 1);

        for (fluid::index neighbour = 0; neighbour < neighbours; ++neighbour) {
            const size_t       position = static_cast<size_t>(neighbour + 1);
            const fluid::index other    = std::stoi(*nearestIds[position]);

            distances(point, neighbour) = nearestDistances[position];
            graph.insert(point, other)  = nearestDistances[position];
        }
    }

    if (shouldStop()) {
        return graph;
    }

    sigma = findSigma(distances, neighbours);

    for (fluid::index outer = 0; outer < graph.outerSize(); ++outer) {
        for (Eigen::SparseMatrix<double>::InnerIterator edge(graph, outer); edge; ++edge) {
            edge.valueRef() = std::exp(-(edge.value() - distances(edge.row(), 0)) / sigma(edge.row()));
        }
    }

    transposed = graph.transpose();
    graph      = (graph + transposed) - graph.cwiseProduct(transposed);

    return graph;
}

Eigen::ArrayXd MapLayout::findSigma(const Eigen::ArrayXXd& distances, fluid::index neighbours) const
{
    const double   target = std::log2(neighbours);
    Eigen::ArrayXd sigma  = Eigen::ArrayXd::Zero(distances.rows());

    for (fluid::index point = 0; point < distances.rows(); ++point) {
        const double nearest = distances(point, 0);

        double low    = 0.0;
        double high   = fluid::algorithm::infinity;
        double middle = 1.0;

        for (int iteration = 0; iteration < sigmaIterationCount; ++iteration) {
            double probabilitySum = 0.0;

            for (fluid::index neighbour = 1; neighbour < distances.cols(); ++neighbour) {
                const double distance = distances(point, neighbour) - nearest;

                if (distance <= 0) {
                    probabilitySum += 1.0;
                } else {
                    probabilitySum += std::exp(-(distance / middle));
                }
            }

            if (std::abs(probabilitySum - target) < sigmaTolerance) {
                break;
            }

            if (probabilitySum > target) {
                high   = middle;
                middle = (low + high) / 2.0;
            } else if (high == fluid::algorithm::infinity) {
                low    = middle;
                middle = middle * 2;
            } else {
                low    = middle;
                middle = (low + high) / 2.0;
            }
        }

        sigma(point) = middle;
    }

    return sigma;
}

Eigen::ArrayXXd MapLayout::normalizeEmbedding(const Eigen::ArrayXXd& embedding, std::mt19937_64& random) const
{
    const double             expansion = 10.0 / embedding.abs().maxCoeff();
    std::normal_distribution noiseDistribution(0.0, 1e-4);

    const auto noise = Eigen::ArrayXXd::NullaryExpr(embedding.rows(), embedding.cols(), [&noiseDistribution, &random] { return noiseDistribution(random); });

    Eigen::ArrayXXd      result  = (embedding * expansion) + noise;
    const Eigen::ArrayXd minimum = result.colwise().minCoeff();
    const Eigen::ArrayXd maximum = result.colwise().maxCoeff();
    const Eigen::ArrayXd range   = (maximum - minimum).max(fluid::algorithm::epsilon);

    result = (result.rowwise() - minimum.transpose());
    result = result.rowwise() / range.transpose();

    return 10.0 * result;
}

void MapLayout::optimizeLayout(Eigen::ArrayXXd& embedding, const Eigen::ArrayXi& heads, const Eigen::ArrayXi& tails, const Eigen::ArrayXd& epochsPerSample, const Eigen::VectorXd& ab,
                               std::mt19937_64& random, const std::function<bool()>& shouldStop) const
{
    const double a = ab(0);
    const double b = ab(1);

    const Eigen::ArrayXd epochsPerNegativeSample = epochsPerSample / negativeSampleRate;
    Eigen::ArrayXd       nextEpoch               = epochsPerSample;
    Eigen::ArrayXd       nextNegativeEpoch       = epochsPerNegativeSample;

    const MapPoint                              bound = MapPoint::Constant(gradientBound);
    std::uniform_int_distribution<fluid::index> randomPoint(0, embedding.rows() - 1);
    double                                      alpha = learningRate;

    for (fluid::index epoch = 0; epoch < iterationCount; ++epoch) {
        if (shouldStop()) {
            return;
        }

        for (fluid::index edge = 0; edge < epochsPerSample.size(); ++edge) {
            MapPoint current;
            MapPoint other;
            MapPoint gradient;

            double       squaredDistance = 0.0;
            double       coefficient     = 0.0;
            fluid::index negativeCount   = 0;

            if (nextEpoch(edge) > epoch) {
                continue;
            }

            current         = embedding.row(heads(edge));
            other           = embedding.row(tails(edge));
            squaredDistance = (current - other).square().sum();

            if (squaredDistance > 0) {
                coefficient  = -2.0 * a * b * std::pow(squaredDistance, b - 1.0);
                coefficient /= a * std::pow(squaredDistance, b) + 1.0;
            }

            gradient  = (coefficient * (current - other)).cwiseMin(bound).cwiseMax(-bound);
            current  += gradient * alpha;
            other    += -gradient * alpha;

            nextEpoch(edge) += epochsPerSample(edge);
            negativeCount    = static_cast<fluid::index>((epoch - nextNegativeEpoch(edge)) / epochsPerNegativeSample(edge));

            for (fluid::index negative = 0; negative < negativeCount; ++negative) {
                const fluid::index negativeIndex = randomPoint(random);
                MapPoint           negativePoint;

                if (negativeIndex == heads(edge)) {
                    continue;
                }

                negativePoint   = embedding.row(negativeIndex);
                squaredDistance = (current - negativePoint).square().sum();
                gradient        = bound;

                if (squaredDistance > 0) {
                    coefficient  = 2.0 * b;
                    coefficient /= (0.001 + squaredDistance) * (a * std::pow(squaredDistance, b) + 1);
                    gradient     = (coefficient * (current - negativePoint)).cwiseMin(bound).cwiseMax(-bound);
                }

                current += gradient * alpha;
            }

            nextNegativeEpoch(edge) += static_cast<double>(negativeCount) * epochsPerNegativeSample(edge);

            embedding.row(heads(edge)) = current;
            embedding.row(tails(edge)) = other;
        }

        alpha = learningRate * (1.0 - (static_cast<double>(epoch) / static_cast<double>(iterationCount)));
    }
}

MapLayout::AbCurve::AbCurve(double minimumDistance)
{
    distances = Eigen::ArrayXd::LinSpaced(ValuesAtCompileTime, 0.0, 3.0);
    targets   = (distances <= minimumDistance).select(1, (-distances + minimumDistance).exp());
}

int MapLayout::AbCurve::operator()(const Eigen::VectorXd& ab, Eigen::VectorXd& residuals) const
{
    residuals = targets - (1 / (1 + ab(0) * distances.pow(2 * ab(1))));

    return 0;
}

int MapLayout::AbCurve::values() const
{
    return ValuesAtCompileTime;
}

int MapLayout::AbCurve::inputs() const
{
    return InputsAtCompileTime;
}
