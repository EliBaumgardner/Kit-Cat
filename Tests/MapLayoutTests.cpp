#include <catch2/catch_test_macros.hpp>

#include "Analysis/MapLayout.h"

#include <flucoma/algorithms/public/UMAP.hpp>

#include <random>

MapLayout::DataSet makeDescriptors(fluid::index pointCount, fluid::index dimensionCount)
{
    std::mt19937                       random(7);
    std::normal_distribution<double>   value(0.0, 1.0);
    fluid::FluidTensor<std::string, 1> ids(pointCount);
    fluid::RealMatrix                  data(pointCount, dimensionCount);

    for (fluid::index point = 0; point < pointCount; ++point) {
        ids(point) = std::to_string(point);

        for (fluid::index dimension = 0; dimension < dimensionCount; ++dimension) {
            data(point, dimension) = value(random);
        }
    }

    return MapLayout::DataSet(ids, data);
}

TEST_CASE("the map layout places every point where FluCoMa's UMAP does for the same seed", "[map]")
{
    MapLayout::DataSet descriptors = makeDescriptors(60, 13);
    MapLayout          layout;
    fluid::RealMatrix  ported;

    fluid::algorithm::UMAP umap;
    MapLayout::DataSet     reference;

    layout.seed = 42;

    ported    = layout.train(descriptors, [] { return false; });
    reference = umap.train(descriptors, layout.neighbourCount, 2, layout.minimumDistance, layout.iterationCount, layout.learningRate, 42);

    REQUIRE(ported.rows() == reference.getData().rows());
    REQUIRE(ported.cols() == reference.getData().cols());

    for (fluid::index point = 0; point < ported.rows(); ++point) {
        CHECK(ported(point, 0) == reference.getData()(point, 0));
        CHECK(ported(point, 1) == reference.getData()(point, 1));
    }
}

TEST_CASE("the map layout returns no positions once it is asked to stop", "[map]")
{
    const fluid::index       pointCount  = 60;
    const MapLayout::DataSet descriptors = makeDescriptors(pointCount, 13);
    const MapLayout          layout;

    for (const int stopAfterCalls : { 0, static_cast<int>(pointCount / 2), static_cast<int>(pointCount + 10) }) {
        int                   calls = 0;
        fluid::RealMatrix     positions;
        std::function<bool()> shouldStop;

        shouldStop = [&calls, stopAfterCalls] {
            calls += 1;

            return calls > stopAfterCalls;
        };

        positions = layout.train(descriptors, shouldStop);

        CHECK(positions.rows() == 0);
    }
}
