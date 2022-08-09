#pragma once

#include <array>
#include <random>

#include "Utils.h"

namespace inventory_changer::item_generator
{

template <typename RandomEngine>
class AttributeGenerator {
public:
    explicit AttributeGenerator(RandomEngine& randomEngine) : randomEngine{ randomEngine } {}

    [[nodiscard]] SkinCondition generateSkinCondition() const
    {
        if (const auto condition = std::uniform_int_distribution{ 1, 10'000 }(randomEngine); condition <= 1471)
            return FactoryNew;
        else if (condition <= 3939)
            return MinimalWear;
        else if (condition <= 8257)
            return FieldTested;
        else if (condition <= 9049)
            return WellWorn;
        return BattleScarred;
    }

    [[nodiscard]] float generatePaintKitWear(SkinCondition condition) const
    {
        static constexpr auto wearRanges = std::to_array<float>({ 0.0f, 0.07f, 0.15f, 0.38f, 0.45f, 1.0f });
        return std::uniform_real_distribution{ wearRanges[condition - 1], wearRanges[condition] }(randomEngine);
    }

    [[nodiscard]] int generatePaintKitSeed() const
    {
        return std::uniform_int_distribution{ 1, 1000 }(randomEngine);
    }

    [[nodiscard]] std::uint32_t generateServiceMedalIssueDate(std::uint16_t year) const
    {
        return getRandomDateTimestampOfYear(year);
    }

private:
    [[nodiscard]] static std::pair<std::time_t, std::time_t> clampTimespanToNow(std::time_t min, std::time_t max) noexcept
    {
        const auto now = std::time(nullptr);
        return std::make_pair((std::min)(min, now), (std::min)(max, now));
    }

    [[nodiscard]] std::uint32_t getRandomDateTimestampOfYear(std::uint16_t year) const noexcept
    {
        const auto [min, max] = clampTimespanToNow(getStartOfYearTimestamp(year), getEndOfYearTimestamp(year));
        return static_cast<std::uint32_t>(std::uniform_int_distribution{ min, max }(randomEngine));
    }

    RandomEngine& randomEngine;
};

}
