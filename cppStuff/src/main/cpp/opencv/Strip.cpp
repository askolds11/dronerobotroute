#include <fstream>
#include <nlohmann/json.hpp>

#include "../jni/org_askolds_cmptest_opencv_OpenCVStuff.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <unordered_set>

#define INCLUDE_NOISE

using namespace cv;
using namespace std;
using json = nlohmann::json;

struct SimplePointsResult
{
    std::vector<int> realPoints;
    std::vector<int> interpolatedPoints;
};

struct AdaptivePatternResult
{
    std::vector<int> realPoints;
    std::vector<int> interpolatedPoints;
    std::vector<int> noisePoints;
    std::vector<int> previousPoints;
    int offset = 0;
};

struct ResultWithOffsetSpacing
{
    AdaptivePatternResult result;
    int prevOffset;
    int avgSpacing;
};

int findClosestPoint(
    const std::vector<int>& prevPoints,
    const int point,
    const int prevOffset
)
{
    int closestPrev = prevPoints[0];
    int closestDist = std::abs(point - (closestPrev + prevOffset));

    if (prevPoints.size() > 1)
    {
        for (int j = 1; j < prevPoints.size(); ++j)
        {
            const int dist = std::abs(point - (prevPoints[j] + prevOffset));
            if (dist < closestDist)
            {
                closestDist = dist;
                closestPrev = prevPoints[j];
            }
            // once distance starts increasing, we're going away
            else
            {
                break;
            }
        }
    }

    return closestPrev;
}

SimplePointsResult extractAdaptivePattern3(
    const std::vector<int>& xPointsUnsorted,
    const int spacing,
    const int tolerance,
    const bool forwardsDirection
)
{
    if (xPointsUnsorted.empty()) return {};

    SimplePointsResult result;
    if (xPointsUnsorted.size() == 1)
    {
        result.realPoints.push_back(xPointsUnsorted[0]);
        return result;
    }
    // TODO: It kā var būt galīgi garām?
    if (xPointsUnsorted.size() == 2)
    {
        result.realPoints = xPointsUnsorted;
        return result;
    }
    vector<int> xPoints = xPointsUnsorted;
    ranges::sort(xPoints);

    if (!forwardsDirection)
    {
        ranges::reverse(xPoints);
    }

    vector<int> forwardPoints;
    vector<int> backwardPoints;

    result.realPoints.push_back(xPoints[0]);
    int currentPoint = xPoints[0];
    int currentPointIdx = 0;
    int idx = 0;
    while (idx < xPoints.size() - 1)
    // for (int i = goodStartIdx; i < xPoints.size() - 1; ++i)
    {
        const int nextPoint = xPoints[idx + 1];

        // see if there's a better point
        const int minDiff = std::abs(std::abs(nextPoint - currentPoint) - spacing);
        if (idx + 2 < xPoints.size())
        {
            const int nextNextPoint = xPoints[idx + 2];
            // next point is better
            if (std::abs(std::abs(nextNextPoint - currentPoint) - spacing) < minDiff)
            {
                idx++;
                continue;
            }
        }
        // If one apart, great
        if (tolerance >= minDiff)
        {
            result.realPoints.push_back(nextPoint);
            currentPoint = nextPoint;
            currentPointIdx = idx + 1;
            idx++;
            continue;
        }
        // less than one space, go to next
        else if (std::abs(nextPoint - currentPoint) < (spacing - tolerance))
        {
            idx++;
            continue;
        }
        // more than one row, see if spacing is like rows or noise
        else if (std::abs(nextPoint - currentPoint) > (spacing + tolerance))
        {
            // row count with comma
            const float rowsRaw = static_cast<float>(std::abs(nextPoint - currentPoint)) / static_cast<float>(spacing);
            // rounded (up/down), prefer a little bit to round down
            const long roundedRows = std::lrint(rowsRaw - 0.1);
            const int noise = static_cast<int>(std::abs(spacing - std::abs(nextPoint - currentPoint) / roundedRows));

            // check if next point isn't better for spacing
            if (idx + 2 < xPoints.size())
            {
                const int nextNextPoint = xPoints[idx + 2];
                // row count with comma
                const float nextRowsRaw = static_cast<float>(std::abs(nextNextPoint - currentPoint)) / static_cast<
                    float>(
                    spacing);
                // rounded (up/down), prefer a little bit to round down
                long nextRoundedRows = std::lrint(nextRowsRaw - 0.1);
                const int nextNoise = static_cast<int>(std::abs(spacing - std::abs(nextNextPoint - currentPoint) / nextRoundedRows));
                // next point is better, if noise is smaller, and difference is x rows
                if (nextRowsRaw - rowsRaw <= 0.5 && nextNoise < noise)
                {
                    idx++;
                    continue;
                }
            }

            // TODO: Recalculate previous interpolated points
            // TODO: Exponential or smth
            const int customTolerance = std::min(static_cast<int>(roundedRows * tolerance), spacing / 3);
            // Within tolerance, is real point
            if (noise < roundedRows * customTolerance /*|| noise < flooredRows * tolerance*/)
            {
                const int rows = /*noise < flooredRows * tolerance ? flooredRows : */static_cast<int>(roundedRows);
                // spacing for interpolated rows
                const int interpSpacing = (nextPoint - currentPoint) / rows;
                bool skip = false;
                for (int j = 1; j <= rows - 1; ++j)
                {
                    int interpolatedX = currentPoint + interpSpacing * j;
                    int minDiffLocal = INT32_MAX;
                    int minIndex = 0;
                    // try to use real points if interpolated are close
                    for (int curr = currentPointIdx + 1; curr <= idx; ++curr)
                    {
                        if (std::abs(xPoints[curr] - interpolatedX) <= minDiffLocal)
                        {
                            minDiffLocal = std::abs(xPoints[curr] - interpolatedX);
                            minIndex = curr;
                        }
                    }

                    int calculatedX = minDiffLocal <= tolerance ? xPoints[minIndex] : interpolatedX;
                    // ReSharper disable once CppTooWideScope
                    const int realPoint = minDiffLocal <= tolerance;

                    if (realPoint)
                    {
                        result.realPoints.push_back(calculatedX);
                        // set current point to the real point
                        currentPoint = calculatedX;
                        currentPointIdx = minIndex;
                        // look at the point after the real point
                        idx = minIndex + 1;
                        skip = true;
                        break;
                    }
                    else
                    {
                        result.interpolatedPoints.push_back(calculatedX);
                    }
                }

                if (skip)
                {
                    continue;
                }

                result.realPoints.push_back(nextPoint);
                currentPoint = nextPoint;
                currentPointIdx = idx + 1;
            }
            idx++;
            continue;
        }
    }

    return result;
}

/**
 * Returns with interpolated points from real points
 * @param xPointsUnsorted
 * @param realPointsSorted
 * @param spacing
 * @param tolerance
 * @param prevOffset
 * @param maxOffset
 * @param prevPointsUnsorted
 * @return
 */
AdaptivePatternResult getWithInterpolatedPoints(
    const std::vector<int>& xPointsUnsorted,
    const std::vector<int>& realPointsSorted,
    const int spacing,
    const int tolerance,
    const int prevOffset,
    const int maxOffset,
    const std::vector<int>& prevPointsUnsorted
)
{
    AdaptivePatternResult result;

    vector<int> xPoints = xPointsUnsorted;
    vector<int> prevPoints = prevPointsUnsorted;
    ranges::sort(xPoints);
    ranges::sort(prevPoints);

#pragma region Best starting point ( TODO: Find one that matches previous + offset )
    // TODO: Best point by finding one that matches previous point + offset

    int currentPoint;
    int currentPointIdx;
    int idx = 0;
    if (realPointsSorted.empty() || prevPoints.empty())
    {
        currentPoint = xPoints[0];
    }
    // Find a point that is close to a previous point and use that as first point
    else
    {
        bool found = false;
        int realPointIdx = 0;
        while (!found && realPointIdx < realPointsSorted.size())
        {
            currentPoint = realPointsSorted[realPointIdx];
            int closestPrevPoint = findClosestPoint(prevPoints, currentPoint, prevOffset);
            if (std::abs(std::abs(currentPoint - (closestPrevPoint + prevOffset))) <= tolerance)
            {
                found = true;
            }
            realPointIdx++;
        }
        if (!found)
        {
            currentPoint = realPointsSorted[0];
        }
    }
    result.realPoints.push_back(currentPoint);
    while (xPoints[idx] != currentPoint)
    {
        idx++;
    }
    currentPointIdx = idx;
#pragma endregion Best starting point

    auto nextIdx = +[](int lIdx) { return lIdx + 1; };
    int firstIdx = idx;
    int firstCurrentPoint = currentPoint;
    bool forwardsDir = true;

loopBeginning:
    while (idx <= xPoints.size() - 1 && idx >= 0)
    // for (int i = goodStartIdx; i < xPoints.size() - 1; ++i)
    {
        if (nextIdx(idx) >= xPoints.size() || nextIdx(idx) < 0)
        {
            break;
        }
        const int nextPoint = xPoints[nextIdx(idx)];

#pragma region Maybe better point (TODO: Check previous + offset points)
        // See if there's a better point (closer to one spacing apart).
        // even if the point is noise, it will be calculated as such in further processing
        int minDiff = std::abs(std::abs(nextPoint - currentPoint) - spacing);
        if (nextIdx(nextIdx(idx)) < xPoints.size() && nextIdx(nextIdx(idx)) > 0)
        {
            const int nextNextPoint = xPoints[nextIdx(nextIdx(idx))];
            // next point is better
            if (std::abs(std::abs(nextNextPoint - currentPoint) - spacing) < minDiff)
            {
                idx = nextIdx(idx);
                continue;
            }
        }
#pragma endregion Maybe better point
        // If one apart, great
        if (tolerance >= minDiff)
        {
#pragma region Point with one spacing
            // Check if the real point matches a previous point
            // if not, use previous point + offset, and real point is noise
            if (!prevPoints.empty())
            {
                int closestPrev = findClosestPoint(prevPoints, nextPoint, prevOffset);
                int plannedPoint = closestPrev + prevOffset;
                int plannedDist = std::abs(nextPoint - plannedPoint);

                // More than MaxOffset from planned point, but less than a row away (not one of next points)
                // and planned point is roughly a space away => real point is noise, use prev + offset instead
                // TODO: Custom tolerance instead of spacing - tolerance * 2
                if (plannedDist > maxOffset &&
                    plannedDist < spacing - tolerance * 2 &&
                    std::abs(currentPoint - plannedPoint) >= spacing - tolerance
                )
                {
                    // if real point near prevpoint, use that instead
                    int minDiffLocal2 = INT32_MAX;
                    int minIndex2 = 0;
                    // try to use real points if prevpoints are close
                    for (int curr = nextIdx(currentPointIdx); forwardsDir ? curr < xPoints.size() : curr >= 0; curr =
                         nextIdx(curr))
                    {
                        if (std::abs(xPoints[curr] - plannedPoint) <= minDiffLocal2)
                        {
                            minDiffLocal2 = std::abs(xPoints[curr] - plannedPoint);
                            minIndex2 = curr;
                        }
                    }

                    if (minDiffLocal2 <= tolerance)
                    {
                        result.realPoints.push_back(xPoints[minIndex2]);
                        currentPoint = xPoints[minIndex2];
                        currentPointIdx = minIndex2;
                        idx = nextIdx(minIndex2);
                    }
                    else
                    {
                        result.previousPoints.push_back(plannedPoint);
                        // Increase current index as well, as the latest point is actually no longer one from the array
                        currentPoint = closestPrev;
                        currentPointIdx = nextIdx(idx);
                        idx = nextIdx(idx);
                    }
                    continue;
                }
            }
            // if no prev points / less or equal than MaxOffset, then current point is ok
            result.realPoints.push_back(nextPoint);
            currentPoint = nextPoint;
            currentPointIdx = nextIdx(idx);
            idx = nextIdx(idx);
            continue;
            // result.realPoints.push_back(nextPoint);
            // currentPoint = nextPoint;
            // currentPointIdx = idx + 1;
            // idx++;
            // continue;
#pragma endregion Point with one spacing
        }
        // less than one space, go to next
        else if (std::abs(nextPoint - currentPoint) < (spacing - tolerance))
        {
#pragma region Too close to current point, skip
            idx = nextIdx(idx);
            continue;
#pragma endregion Too close to current point, skip
        }

        // more than one row, see if spacing is like rows or noise
        else if (std::abs(nextPoint - currentPoint) > (spacing + tolerance))
        {
#pragma region More than one row gap
            // interrow count with comma
            float rowsRaw = static_cast<float>(std::abs(nextPoint - currentPoint)) / static_cast<float>(spacing);
            // rounded (up/down), prefer a little bit to round down
            long roundedRows = std::lrint(rowsRaw - 0.1);
            /* amount of pixels difference for perfect spacing between the points */
            int noise = static_cast<int>(std::abs(spacing - std::abs(nextPoint - currentPoint) / roundedRows));

            if (!prevPoints.empty())
            {
                int closestPrevNext = findClosestPoint(prevPoints, nextPoint, prevOffset);
                int closestDist = std::abs(nextPoint - (closestPrevNext + prevOffset));

                // further than max offset, but less than a row away - noise
                // TODO: Custom tolerance
                if (closestDist > maxOffset && closestDist < (spacing - tolerance * 2))
                {
                    idx = nextIdx(idx);
                    continue;
                }
            }

            // check if next point isn't better for spacing
            if (nextIdx(nextIdx(idx)) < xPoints.size() && nextIdx(nextIdx(idx)) > 0)
            {
                const int nextNextPoint = xPoints[nextIdx(nextIdx(idx))];
                // row count with comma
                float nextRowsRaw = static_cast<float>(std::abs(nextNextPoint - currentPoint)) / static_cast<float>(
                    spacing);
                // rounded (up/down), prefer a little bit to round down
                long nextRoundedRows = std::max(1l, std::lrint(nextRowsRaw - 0.1));
                int nextNoise = static_cast<int>(std::abs(
                    spacing - std::abs(nextNextPoint - currentPoint) / nextRoundedRows));

                // next point is better, if noise is smaller, and is for the same row
                if (nextRowsRaw - rowsRaw <= 0.5 && nextNoise < noise)
                {
                    idx = nextIdx(idx);
                    continue;
                }
            }

            // TODO: Recalculate previous interpolated points
            // TODO: Maybe a custom parameter?
            // Tolerance can be higher for multiple row difference, as the error can stack
            if (noise < std::min(static_cast<int>(roundedRows), 3) * tolerance /*|| noise < flooredRows * tolerance*/)
            {
                bool skip = false;
                bool didAllPoints = false;
                bool wasPrevPoint = false;
                bool wasRealPoint = false;
                int setCurrentPointTo = currentPoint;
                int setCurrentIdxTo = idx;
                int interpolateToPoint = nextPoint;
                vector<int> interpolatedPoints;
                while (!didAllPoints)
                {
                    // spacing for interpolated rows
                    int interpSpacing = static_cast<int>((interpolateToPoint - currentPoint) / roundedRows);
                    int lastPoint = currentPoint;
                    for (int j = 1; j <= roundedRows - 1; ++j)
                    {
                        int interpolatedX = currentPoint + interpSpacing * j;
                        int minDiffLocal = INT32_MAX;
                        int minIndex = 0;
                        // try to use real points if interpolated are close
                        // TODO: (performance) Stop looking when starts increasing
                        // Find closest real point
                        for (int curr = nextIdx(currentPointIdx); forwardsDir ? curr <= idx : curr >= idx; curr =
                             nextIdx(curr))
                        {
                            if (std::abs(xPoints[curr] - interpolatedX) <= minDiffLocal)
                            {
                                minDiffLocal = std::abs(xPoints[curr] - interpolatedX);
                                minIndex = curr;
                            }
                        }

                        // TODO: Custom tolerance?
                        int calculatedX = minDiffLocal <= tolerance ? xPoints[minIndex] : interpolatedX;
                        bool realPoint = minDiffLocal <= tolerance;

                        // Check if is close to previous point, if exists
                        bool prevPoint = false;
                        int closestPrev = -1;
                        if (!prevPoints.empty())
                        {
                            closestPrev = findClosestPoint(prevPoints, calculatedX, prevOffset);
                            int plannedPoint = closestPrev + prevOffset;
                            int plannedDist = std::abs(calculatedX - plannedPoint);

                            // More than MaxOffset from planned point, but less than a row away (not one of next points)
                            // and planned point is at least a row away from previous and next points => point is noise, use prev + offset instead
                            // TODO: Custom tolerance
                            if (plannedDist > maxOffset &&
                                plannedDist < spacing - tolerance * 2 &&
                                std::abs(lastPoint - plannedPoint) >= spacing - tolerance &&
                                std::abs(interpolateToPoint - plannedPoint) >= spacing - tolerance
                            )
                            {
                                realPoint = false;
                                prevPoint = true;
                                calculatedX = plannedPoint;
                            }
                        }

                        // if real point near prevpoint, use that instead
                        if (prevPoint)
                        {
                            int minDiffLocal2 = INT32_MAX;
                            int minIndex2 = 0;
                            // try to use real points if prevpoints are close
                            for (int curr = nextIdx(currentPointIdx); forwardsDir ? curr <= idx : curr >= idx; curr =
                                 nextIdx(curr))
                            {
                                if (std::abs(xPoints[curr] - calculatedX) <= minDiffLocal2)
                                {
                                    minDiffLocal2 = std::abs(xPoints[curr] - calculatedX);
                                    minIndex2 = curr;
                                }
                            }

                            if (minDiffLocal2 <= tolerance)
                            {
                                calculatedX = xPoints[minIndex2];
                                realPoint = true;
                                prevPoint = false;
                            }
                        }

                        // if the interpolated point is less than a row from the planned previous point
                        // if a real point within a range is closer to the planned previous point than the interpolated one, use that instead
                        // todo: spacing - tolerance, or maybe offset?
                        if (!prevPoint && !realPoint && closestPrev != -1 && std::abs(
                            calculatedX - (closestPrev + prevOffset)) < spacing - tolerance)
                        {
                            int minDiffLocal2 = std::abs(calculatedX - (closestPrev + prevOffset));
                            int minIndex2 = -1;

                            for (int curr = nextIdx(currentPointIdx); forwardsDir ? curr <= idx : curr >= idx; curr =
                                 nextIdx(curr))
                            {
                                if (std::abs(xPoints[curr] - closestPrev - prevOffset) <= minDiffLocal2)
                                {
                                    minDiffLocal2 = std::abs(xPoints[curr] - closestPrev - prevOffset);
                                    minIndex2 = curr;
                                }
                            }

                            if (minIndex2 != -1 && minDiffLocal2 <= tolerance)
                            {
                                calculatedX = xPoints[minIndex2];
                                realPoint = true;
                                prevPoint = false;
                            }
                        }

                        if (prevPoint)
                        {
                            // If not first point, redo the previous points
                            if (j > 1)
                            {
                                interpolatedPoints.clear();
                                interpolateToPoint = calculatedX;
                                roundedRows = j;
                                wasPrevPoint = true;
                                wasRealPoint = false;
                                setCurrentPointTo = calculatedX;
                                skip = true;
                                break;
                            }
                            // no need to redo
                            wasPrevPoint = false;
                            wasRealPoint = false;
                            result.previousPoints.push_back(calculatedX);
                            currentPoint = calculatedX;
                            // Don't go to the next point
                            // currentPointIdx = idx;
                            skip = true;
                            didAllPoints = true;
                            break;
                        }
                        else if (realPoint)
                        {
                            // If not first point, redo the previous points
                            if (j > 1)
                            {
                                interpolatedPoints.clear();
                                interpolateToPoint = calculatedX;
                                roundedRows = j;
                                wasPrevPoint = false;
                                wasRealPoint = true;
                                setCurrentPointTo = calculatedX;
                                setCurrentIdxTo = minIndex;
                                skip = true;
                                break;
                            }
                            // no need to redo
                            wasPrevPoint = false;
                            wasRealPoint = false;
                            result.realPoints.push_back(calculatedX);
                            // set current point to the real point
                            currentPoint = calculatedX;
                            currentPointIdx = minIndex;
                            // look at the point after the real point
                            idx = nextIdx(minIndex);
                            skip = true;
                            didAllPoints = true;
                            break;
                        }
                        else
                        {
                            // result.interpolatedPoints.push_back(calculatedX);
                            interpolatedPoints.push_back(calculatedX);
                        }

                        if (j == roundedRows - 1)
                        {
                            didAllPoints = true;
                        }
                        lastPoint = calculatedX;
                    }
                }

                for (auto& interPoint : interpolatedPoints)
                {
                    result.interpolatedPoints.push_back(interPoint);
                }

                if (wasPrevPoint)
                {
                    result.previousPoints.push_back(setCurrentPointTo);
                    currentPoint = setCurrentPointTo;
                    // Don't go to the next point
                    // currentPointIdx = idx;
                    skip = true;
                }
                else if (wasRealPoint)
                {
                    result.realPoints.push_back(setCurrentPointTo);
                    // set current point to the real point
                    currentPoint = setCurrentPointTo;
                    currentPointIdx = setCurrentIdxTo;
                    // look at the point after the real point
                    idx = nextIdx(setCurrentIdxTo);
                    skip = true;
                }

                if (skip)
                {
                    continue;
                }

                result.realPoints.push_back(nextPoint);
                currentPoint = nextPoint;
                currentPointIdx = nextIdx(idx);
            }
            idx = nextIdx(idx);
            continue;
#pragma endregion More than one row gap
        }
    }

    // Repeat loop but backwards
    if (forwardsDir)
    {
        nextIdx = +[](int lIdx) { return lIdx - 1; };
        idx = firstIdx;
        currentPointIdx = firstIdx;
        currentPoint = firstCurrentPoint;
        forwardsDir = false;
        goto loopBeginning;
    }

    vector<int> resultingPoints;
    resultingPoints.insert(resultingPoints.end(), result.realPoints.begin(), result.realPoints.end());
    resultingPoints.insert(resultingPoints.end(), result.interpolatedPoints.begin(), result.interpolatedPoints.end());
    resultingPoints.insert(resultingPoints.end(), result.previousPoints.begin(), result.previousPoints.end());
    ranges::sort(resultingPoints);
    int offsetSum = 0;
    int offsetPoints = 0;
    vector<int> usedPrevPoints;
    if (!prevPoints.empty())
    {
        for (auto& p : resultingPoints)
        {
            auto prevPoint = findClosestPoint(prevPoints, p, prevOffset);
            // TODO: Custom spacing
            if (std::abs(p - (prevPoint + prevOffset)) <= spacing / 2)
            {
                usedPrevPoints.push_back(prevPoint);
                offsetSum += p - prevPoint;
                offsetPoints++;
            }
        }
    }
    vector<int> unusedPrevPoints;
    if (!prevPoints.empty() && !usedPrevPoints.empty())
        ranges::set_difference(prevPoints, usedPrevPoints, std::back_inserter(unusedPrevPoints));
    if (!resultingPoints.empty())
        ranges::set_difference(xPoints, resultingPoints, std::back_inserter(result.noisePoints));

    auto prevIter = unusedPrevPoints.begin();
    // Check if there isn't a unused real point nearby, if so, use that instead and this prevpoint is actually used
    while (prevIter != unusedPrevPoints.end())
    {
        auto noiseIter = result.noisePoints.begin();
        while (noiseIter != result.noisePoints.end())
        {
            // found real point
            if (std::abs(*noiseIter - (*prevIter + prevOffset)) <= tolerance)
            {
                result.realPoints.push_back(*noiseIter);
                prevIter = unusedPrevPoints.erase(prevIter);
                result.noisePoints.erase(noiseIter);
                break;
            }
            ++noiseIter;
        }
        if (prevIter != unusedPrevPoints.end())
            ++prevIter;
    }

    for (auto& p : unusedPrevPoints)
    {
        result.previousPoints.push_back(p + prevOffset);
    }

    // Need at least 3 points for a reliable-ish average
    if (offsetPoints > 5)
    {
        result.offset = offsetSum / offsetPoints;
    }
    else
    {
        result.offset = prevOffset;
    }
    return result;
}

/**
 * Calculates spacings and possible points per row
 * @param input Input image (grayscale, with only POIs left)
 * @param spacingMap Row spacing in format {rowSpacing, numberOfOccurences}
 * @param pointsByRow Output vector for detected points per row
 * @param stripHeight Chosen strip height in pixels
 * @param whiteThreshold White threshold for a POI to be considered a point
 */
void CalculateStrips(
    const UMat& input,
    map<int, int>& spacingMap,
    std::map<int, std::vector<int>>& pointsByRow,
    const int stripHeight,
    const int whiteThreshold
)
{
    // Caching
    // {
    //     std::ifstream inFile1("/home/askolds/Downloads/DroniOtsu/spacingMap.json");
    //     if (!inFile1)
    //     {
    //         std::cerr << "Failed to open spacingMap.json\n";
    //         throw;
    //     }
    //     json j1;
    //     inFile1 >> j1;
    //     for (auto& [k, v] : j1.items())
    //     {
    //         spacingMap[std::stoi(k)] = v;
    //     }
    //
    //     std::ifstream inFile2("/home/askolds/Downloads/DroniOtsu/pointsByRow.json");
    //     if (!inFile2)
    //     {
    //         std::cerr << "Failed to open pointsByRow.json\n";
    //         throw;
    //     }
    //
    //     json j2;
    //     inFile2 >> j2;
    //     // Deserialize json back to map<int, vector<int>>
    //     for (auto& [k, v] : j2.items())
    //     {
    //         pointsByRow[std::stoi(k)] = v.get<std::vector<int>>();
    //     }
    //
    //     inFile1.close();
    //     inFile2.close();
    //     return;
    // }

    // std::ifstream f("spacingMap.json");
    // std::ifstream f2("pointsByRow.json");
    const int cols = input.cols;
    const int rows = input.rows;

    // Create output image (black background)
    cv::UMat output(rows, cols, CV_8UC1, cv::Scalar(0));
    auto tempOutputMat = cv::imread(R"(/home/askolds/Downloads/DroniOtsu/woOutlier/011_A_45m_Contour_otsu_4.jpg)", cv::IMREAD_COLOR);
    UMat tempOutput;
    tempOutputMat.copyTo(tempOutput);

    UMat prevHistogram;
    UMat currHistogram;

    // Go strip by height
    // TODO: Only do this in range min/max, ignoring empty rows at top/bottom of image
    for (int y = 0; y < rows; y += stripHeight)
    {
        // Actual strip height, in case not enough pixels left over
        const int currentStripHeight = std::min(stripHeight, rows - y);
        UMat strip = input(Range(y, y + currentStripHeight), Range(0, cols));

        Point startPoint = Point(0, y);
        Point endPoint = Point(input.cols, y);
        line(tempOutput, startPoint, endPoint, cv::Scalar(0, 0, 255), 2);

        bool isFirst = y == 0;
        bool isLast = y + stripHeight >= rows;

        // Calculate first histogram
        if (isFirst)
        {
            UMat histogram;
            // Count (white) pixels in columns
            cv::reduce(strip, histogram, 0, REDUCE_SUM, CV_64FC1);

            // Convert to count
            divide(histogram, 255, currHistogram, 1, CV_16UC1);
        }
        // If not last, calculate next
        UMat nextHistogram2;
        if (!isLast)
        {
            UMat nextHistogram;
            // Count (white) pixels in columns
            cv::reduce(strip, nextHistogram, 0, REDUCE_SUM, CV_64FC1);

            // Convert to count
            divide(nextHistogram, 255, nextHistogram2, 1, CV_16UC1);
        }

        // combine three strips with less weight on previous and next
        UMat histogram3;
        if (!isFirst)
        {
            cv::addWeighted(currHistogram, 1.0, prevHistogram, 0.5, 0, histogram3);
        }

        if (!isLast)
        {
            if (!isFirst)
            {
                cv::addWeighted(histogram3, 1.0, nextHistogram2, 0.5, 0, histogram3);
            }
            else
            {
                cv::addWeighted(currHistogram, 1.0, nextHistogram2, 0.5, 0, histogram3);
            }
        }

        if (isFirst && isLast)
        {
            currHistogram.copyTo(histogram3);
        }

        // move over
        currHistogram.copyTo(prevHistogram);
        if (!isLast)
        {
            nextHistogram2.copyTo(currHistogram);
        }

        // TODO: Another parameter?
        int threshold = whiteThreshold;
        if (!isFirst) threshold += whiteThreshold / 2;
        if (!isLast) threshold += whiteThreshold / 2;


        // Threshold the count to binary mask: 255 if > whiteThreshold, else 0
        cv::UMat mask8u;
        cv::threshold(histogram3, mask8u, threshold, 255, cv::THRESH_BINARY);
        mask8u.convertTo(mask8u, CV_8UC1); // Ensure 8-bit

        // TODO: Parameters
        int noiseSize = 5; // Remove small isolated white dots (opening)
        int gapSize = 20; // Fill small black gaps between white pixels (closing)

        cv::Mat openKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noiseSize, 1));
        cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(gapSize + 1, 1));

        cv::UMat denoisedMask, cleanedMask;

        // Step 1: Opening — remove small noise
        cv::morphologyEx(mask8u, denoisedMask, cv::MORPH_OPEN, openKernel);

        // Step 2: Closing — fill small gaps
        cv::morphologyEx(denoisedMask, cleanedMask, cv::MORPH_CLOSE, closeKernel);

        // Copy result to output image
        cleanedMask.copyTo(output(cv::Range(y + currentStripHeight / 2, y + currentStripHeight / 2 + 1),
                                  cv::Range::all())); // Write one row directly

        // Get centroids
        cv::Mat labels, stats, centroids;
        int nLabels = cv::connectedComponentsWithStats(cleanedMask, labels, stats, centroids, 8, CV_32S);

        // Step 3: Create new image with white dots at centroids
        cv::Mat centerDotsMat = cv::Mat::zeros(output.rows, output.cols, CV_8UC1);
        int lastCol;

        // Go through each line
        for (int i = 1; i < nLabels; ++i)
        {
            int cx = static_cast<int>(centroids.at<double>(i, 0));

            if (cx >= 0 && cx < input.cols)
            {
                pointsByRow[y + currentStripHeight / 2].emplace_back(cx);
                // cv::circle(original, cv::Point(cx, cy), 20, cv::Scalar(0, 0, 255), -1); // red (BGR)
                // cv::circle(original, cv::Point(cx, y + currentStripHeight / 2), 20, cv::Scalar(0, 0, 255), -1);
                // red (BGR)
            }

            // Calculate spacing
            if (i > 1)
            {
                int dist = cx - lastCol;
                // cv::Point pt((cx - dist / 2 - 50), y + 40); // pixel coordinates
                // cv::putText(original, std::format("{}", dist), pt, cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0,
                //             cv::Scalar(0, 255, 255), 3);
                if (spacingMap.contains(dist))
                {
                    spacingMap[dist]++;
                }
                else
                {
                    spacingMap.insert({dist, 1});
                }
            }
            lastCol = cx;
        }

        // sort(xCoords, xCoords + nLabels - 1);

        // if (nLabels > 2)
        // {
        //     for (int i = 1; i < nLabels - 1; ++i)
        //     {
        //         if (numberMap.contains(xCoords[i] - xCoords[i - 1]))
        //         {
        //             numberMap[xCoords[i] - xCoords[i - 1]]++;
        //         }
        //         else
        //         {
        //             numberMap.insert({xCoords[i] - xCoords[i - 1], 1});
        //         }
        //     }
        // }

        // Debug view
        // cv::UMat output2;
        // cv::resize(strip, output2, cv::Size(), displayScale, displayScale, cv::INTER_AREA);
        // cv::resize(histogram2, output2, cv::Size(), displayScale, 1, cv::INTER_AREA);

        // cv::imshow("Visualization", output2);
        // cv::waitKey(0);
        // printf("Next\n");
    }

    imwrite(R"(/home/askolds/Downloads/DroniOtsu/image_split_lines_grayscale.jpg)", tempOutput);

    // Caching
    std::ofstream o("/home/askolds/Downloads/DroniOtsu/spacingMap.json");
    json j1 = json::object();
    for (const auto& [k, v] : spacingMap) {
        j1[std::to_string(k)] = v;
    }
    o << j1 << std::endl;
    std::ofstream o2("/home/askolds/Downloads/DroniOtsu/pointsByRow.json");
    json j2 = json::object();
    for (const auto& [k, v] : pointsByRow) {
        j2[std::to_string(k)] = v;
    }
    o2 << j2 << std::endl;
}

void processImage(
    const cv::UMat& input,
    int stripHeight,
    int whiteThreshold,
    float tolerancePercentage,
    float maxOffsetPercentage,
    double displayScale = 0.05
)
{
    auto original = cv::imread(R"(/home/askolds/Downloads/DroniOtsu/011_A_45m_Contour.jpg)", cv::IMREAD_COLOR);
    auto original2 = cv::imread(R"(/home/askolds/Downloads/DroniOtsu/011_A_45m_Contour.jpg)", cv::IMREAD_COLOR);

    map<int, int> spacingMap;
    /* y, [x] */
    std::map<int, std::vector<int>> pointsByRow;
    CalculateStrips(
        input,
        spacingMap,
        pointsByRow,
        stripHeight,
        whiteThreshold
    );

    // Get the most common spacing for initial results
    int mostCommon;
    {
        auto mostCommonSpacing = pair(0, 0);

        for (auto& [fst, snd] : spacingMap)
        {
            if (mostCommonSpacing.second < snd)
            {
                mostCommonSpacing.first = fst;
                mostCommonSpacing.second = snd;
            }
            // if (snd > 10)
            // {
            // printf("%d,%d,\n", fst, snd);
            // }
        }

        mostCommon = mostCommonSpacing.first;
    }

    // TODO: Parameters
    // int tolerance = 0.1 * mostCommon;
    // int maxOffset = 0.3 * mostCommon;

    // printf("Spacing: %d\n", mostCommon);
    // printf("Tolerance: %d\n", tolerance);

    /** Rows ranked by how many points there are, how much noise etc. */
    map<int, float> rowScore;

    for (auto& [fst, snd] : pointsByRow)
    {
        // Find best (or close to best) starting line
        auto points = extractAdaptivePattern3(snd, mostCommon,
                                              static_cast<int>(tolerancePercentage * static_cast<float>(mostCommon)),
                                              true);
        auto points2 = extractAdaptivePattern3(snd, mostCommon,
                                               static_cast<int>(tolerancePercentage * static_cast<float>(mostCommon)),
                                               false);

        // Get overlapping real points - these have a high probability of being right
        ranges::sort(points.realPoints);
        ranges::sort(points2.realPoints);
        vector<int> realPoints;
        ranges::set_intersection(points.realPoints, points2.realPoints, back_inserter(realPoints));

        // Get all interpolated points (to calculate row score)
        vector<int> interpolatedPoints = points.interpolatedPoints;
        interpolatedPoints.insert(interpolatedPoints.end(), points2.interpolatedPoints.begin(),
                                  points2.interpolatedPoints.end());

        // Get all noise points (could also be real, but not in the overlapping set)
        vector<int> noisePoints;
        ranges::set_difference(snd, realPoints, std::back_inserter(noisePoints));

        // for (auto & p : noisePoints)
        // {
        //     cv::circle(original2, cv::Point(p, fst), 20, cv::Scalar(0, 0, 0), -1); // red (BGR)
        // }
        // for (auto & p : points.interpolatedPoints)
        // {
        //     cv::circle(original2, cv::Point(p, fst), 18, cv::Scalar(255, 0, 0), -1); // red (BGR)
        // }
        // for (auto & p : points2.interpolatedPoints)
        // {
        //     cv::circle(original2, cv::Point(p, fst), 18, cv::Scalar(0, 255, 0), -1); // red (BGR)
        // }
        // // for (auto & p : points.previousPoints)
        // // {
        // //     cv::circle(original2, cv::Point(p, fst), 16, cv::Scalar(0, 255, 0), -1); // red (BGR)
        // // }
        // for (auto & p : realPoints)
        // {
        //     cv::circle(original2, cv::Point(p, fst), 14, cv::Scalar(0, 0, 255), -1); // red (BGR)
        // }


        // prev.clear();
        // prev.insert(prev.end(), points.realPoints.begin(), points.realPoints.end());
        // prev.insert(prev.end(), points.interpolatedPoints.begin(), points.interpolatedPoints.end());
        // prev.insert(prev.end(), points.previousPoints.begin(), points.previousPoints.end());
        rowScore[fst] = static_cast<float>(
            static_cast<float>(realPoints.size()) / static_cast<float>(snd.size())
            - 0.5 * static_cast<float>(interpolatedPoints.size()) / static_cast<float>(snd.size())
            + 0.05 * static_cast<float>(realPoints.size())
        );
        if (snd.size() <= 2)
        {
            rowScore[fst] = 0;
        }
        rowScore[fst] *= 100;
        // cv::Point pt(7000 + 10 * static_cast<int>(rowScore[fst]), fst + stripHeight / 2); // pixel coordinates
        // cv::putText( original2, std::format("{:.2f}", rowScore[fst]), pt, cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0, cv::Scalar(0,255,255), 3);
        // printf("%d \t\t %f\n", fst, rowScore[fst]);

        // prevOffset = points.offset;
        // if (prev.size() >= 2)
        // {
        //     vector<int> sorted = prev;
        //     std::sort(sorted.begin(), sorted.end());
        //     int sum = 0;
        //     auto first = *sorted.begin();
        //     auto prev = first;
        //     for (auto & p : sorted)
        //     {
        //         if (p != first)
        //         {
        //             sum += (p - prev);
        //             prev = p;
        //         }
        //     }
        //     avgSpacing = sum / static_cast<int>(sorted.size() - 1);
        //     // printf("Spacing: %d\n", avgSpacing);
        // }
    }

    /* Best row index */
    int bestRow = 0;
    /* All row y coords (keys for points map) */
    vector<int> allRowKeys;
    {
        float bestRowScore = FLT_MIN;

        for (auto& [fst, snd] : rowScore)
        {
            allRowKeys.push_back(fst);
            if (snd > bestRowScore)
            {
                bestRow = fst;
                bestRowScore = snd;
            }
        }
    }


    ranges::sort(allRowKeys);

    vector<int> bestPrev;
    int prevOffset = 0;
    int avgSpacing = 0;
    /* result, used prevoffset, used spacing*/
    std::map<int, ResultWithOffsetSpacing> resultingPoints;


    AdaptivePatternResult bestRowResult;
    // Do calculations for best row
    // get average spacing for best result
    {
        auto rowPoints = pointsByRow[bestRow];
        // First get initial real points, calculate expected spacing
        vector<int> realPoints;
        {
            // Get points both ways
            auto points = extractAdaptivePattern3(rowPoints, mostCommon,
                                                  static_cast<int>(tolerancePercentage * static_cast<float>(
                                                      mostCommon)), true);
            auto points2 = extractAdaptivePattern3(rowPoints, mostCommon,
                                                   static_cast<int>(tolerancePercentage * static_cast<float>(
                                                       mostCommon)), false);

            // Get real points (intersection of real points from both ways)
            ranges::sort(points.realPoints);
            ranges::sort(points2.realPoints);
            ranges::set_intersection(points.realPoints, points2.realPoints, back_inserter(realPoints));

            // Calculate average spacing
            vector<int> allPoints;
            allPoints.insert(allPoints.end(), points.realPoints.begin(), points.realPoints.end());
            allPoints.insert(allPoints.end(), points.interpolatedPoints.begin(), points.interpolatedPoints.end());

            ranges::sort(allPoints);
            int sum = 0;
            auto firstPoint = *allPoints.begin();
            auto prevPoint = firstPoint;
            for (auto& p : allPoints)
            {
                if (p != firstPoint)
                {
                    sum += (p - prevPoint);
                    prevPoint = p;
                }
            }
            avgSpacing = sum / static_cast<int>(allPoints.size() - 1);
        }

        bestRowResult = getWithInterpolatedPoints(
            rowPoints,
            realPoints,
            avgSpacing,
            static_cast<int>(tolerancePercentage * static_cast<float>(avgSpacing)),
            0,
            static_cast<int>(maxOffsetPercentage * static_cast<float>(avgSpacing)),
            bestPrev
        );


        // Fill previous row
        bestPrev.insert(bestPrev.end(), bestRowResult.realPoints.begin(), bestRowResult.realPoints.end());
        bestPrev.insert(bestPrev.end(), bestRowResult.interpolatedPoints.begin(), bestRowResult.interpolatedPoints.end());
        ranges::sort(bestPrev);
        // First row has no previous points
        // prev.insert(prev.end(), bestRowResult.previousPoints.begin(), bestRowResult.previousPoints.end());
        // Calculate new average spacing
        {
            auto firstPoint = *bestPrev.begin();
            auto prevPoint = firstPoint;
            int sum = 0;
            for (auto& p : bestPrev)
            {
                if (p != firstPoint)
                {
                    sum += (p - prevPoint);
                    prevPoint = p;
                }
            }
            avgSpacing = sum / static_cast<int>(bestPrev.size() - 1);
        }

        // Draw
        for (auto& p : bestRowResult.noisePoints)
        {
            cv::circle(original2, cv::Point(p, bestRow), 20, cv::Scalar(0, 0, 0), -1);
        }
        for (auto& p : bestRowResult.realPoints)
        {
            cv::circle(original2, cv::Point(p, bestRow), 18, cv::Scalar(0, 0, 255), -1);
        }
        for (auto& p : bestRowResult.interpolatedPoints)
        {
            cv::circle(original2, cv::Point(p, bestRow), 18, cv::Scalar(0, 255, 255), -1);
        }
        for (auto& p : bestRowResult.previousPoints)
        {
            cv::circle(original2, cv::Point(p, bestRow), 16, cv::Scalar(0, 255, 0), -1);
        }
    }

    resultingPoints[bestRow] = ResultWithOffsetSpacing(bestRowResult, 0, avgSpacing);

    // actually 0
    prevOffset = bestRowResult.offset;
    int bestRowAvgSpacing = avgSpacing;


    auto nextIdx = +[](int lIdx) { return lIdx + 1; };
    bool forwardsDir = true;
    int rowIdx = 0;

    while (allRowKeys[rowIdx] != bestRow)
    {
        rowIdx++;
    }
    int bestRowIdx = rowIdx;
    rowIdx++;
    vector<int> prev = bestPrev;

loopBeginningProcessImage:
    // Go to one side
    for (; forwardsDir ? rowIdx < allRowKeys.size() : rowIdx >= 0; rowIdx = nextIdx(rowIdx))
    // for (auto& row : allRowKeys)
    {
        int row = allRowKeys[rowIdx];

        auto rowPoints = pointsByRow[row];

        auto points = extractAdaptivePattern3(rowPoints, avgSpacing,
                                              static_cast<int>(tolerancePercentage * static_cast<float>(avgSpacing)),
                                              true);
        auto points2 = extractAdaptivePattern3(rowPoints, avgSpacing,
                                               static_cast<int>(tolerancePercentage * static_cast<float>(avgSpacing)),
                                               false);

        // Find the most possible real points, then interpolate

        ranges::sort(points.realPoints);
        ranges::sort(points2.realPoints);
        vector<int> realPoints;
        ranges::set_intersection(points.realPoints, points2.realPoints, back_inserter(realPoints));

        // TODO: Use initial row spacing, or use previous?

        // vector<int> allPoints;
        // allPoints.insert(allPoints.end(), points.realPoints.begin(), points.realPoints.end());
        // allPoints.insert(allPoints.end(), points.interpolatedPoints.begin(), points.interpolatedPoints.end());
        // // Initial search does not have previous points
        // // prev.insert(prev.end(), points.previousPoints.begin(), points.previousPoints.end());
        //
        // ranges::sort(allPoints);
        // int sum = 0;
        // auto firstPoint = *allPoints.begin();
        // auto prevPoint = firstPoint;
        // for (auto& p : allPoints)
        // {
        //     if (p != firstPoint)
        //     {
        //         sum += (p - prevPoint);
        //         prevPoint = p;
        //     }
        // }
        // avgSpacing = sum / static_cast<int>(allPoints.size() - 1);

        auto result = getWithInterpolatedPoints(
            rowPoints,
            realPoints,
            avgSpacing,
            static_cast<int>(tolerancePercentage * static_cast<float>(avgSpacing)),
            prevOffset,
            static_cast<int>(maxOffsetPercentage * static_cast<float>(avgSpacing)),
            prev
        );

        if (forwardsDir)
        {
            resultingPoints[row] = ResultWithOffsetSpacing(result, prevOffset, avgSpacing);
            if (rowIdx == bestRowIdx + 1)
            {
                resultingPoints[row] = ResultWithOffsetSpacing(result, result.offset, avgSpacing);
            }
        }
        else
        {
            resultingPoints[row] = ResultWithOffsetSpacing(result, 0, avgSpacing);
            if (rowIdx == bestRowIdx - 1)
            {
                int nextRowOpposite = allRowKeys[rowIdx + 1];
                // Going up, change so that all offsets are going down
                auto nextResult = resultingPoints[nextRowOpposite];
                nextResult.prevOffset = -result.offset;
                resultingPoints[nextRowOpposite] = nextResult;
            } else
            {
                int nextRowOpposite = allRowKeys[rowIdx + 1];
                // Going up, change so that all offsets are going down
                auto nextResult = resultingPoints[nextRowOpposite];
                nextResult.prevOffset = -prevOffset;
                resultingPoints[nextRowOpposite] = nextResult;
            }
        }

        prev.clear();
        prev.insert(prev.end(), result.realPoints.begin(), result.realPoints.end());
        prev.insert(prev.end(), result.interpolatedPoints.begin(), result.interpolatedPoints.end());
        prev.insert(prev.end(), result.previousPoints.begin(), result.previousPoints.end());
        ranges::sort(prev);
        if (prev.size() > 1)
        {
            auto firstPoint = *prev.begin();
            auto prevPoint = firstPoint;
            int sum = 0;
            for (auto& p : prev)
            {
                if (p != firstPoint)
                {
                    sum += (p - prevPoint);
                    prevPoint = p;
                }
            }
            avgSpacing = sum / static_cast<int>(prev.size() - 1);
        }

        prevOffset = result.offset;
        // printf("Spacing: %d\n", avgSpacing);

        // for (auto& p : result.noisePoints)
        // {
        //     cv::circle(original2, cv::Point(p, row), 20, cv::Scalar(0, 0, 0), -1); // red (BGR)
        // }
        // for (auto& p : result.realPoints)
        // {
        //     cv::circle(original2, cv::Point(p, row), 18, cv::Scalar(0, 0, 255), -1); // red (BGR)
        // }
        // for (auto& p : result.interpolatedPoints)
        // {
        //     cv::circle(original2, cv::Point(p, row), 18, cv::Scalar(0, 255, 255), -1); // red (BGR)
        // }
        // for (auto& p : result.previousPoints)
        // {
        //     cv::circle(original2, cv::Point(p, row), 16, cv::Scalar(0, 255, 0), -1); // red (BGR)
        // }
        // for (auto & p : realPoints)
        // {
        //     cv::circle(original2, cv::Point(p, fst), 14, cv::Scalar(0, 0, 255), -1); // red (BGR)
        // }
    }

    if (forwardsDir)
    {
        prev = bestPrev;
        nextIdx = +[](int lIdx) { return lIdx - 1; };
        rowIdx = bestRowIdx > 0 ? bestRowIdx - 1 : 0;
        forwardsDir = false;
        prevOffset = bestRowResult.offset;
        avgSpacing = bestRowAvgSpacing;
        goto loopBeginningProcessImage;
    }

    // Finally populate points
    for (int i = 0; i < allRowKeys.size() - 1; ++i)
    {
        int row = allRowKeys[i];
        int nextRow = allRowKeys[i + 1];

        vector<int> xPoints = pointsByRow[row];
        ranges::sort(xPoints);

        auto prevPointsRaw = resultingPoints[row];
        auto prevPointsResult = prevPointsRaw.result;
        vector<int> prevPoints;
        prevPoints.insert(prevPoints.end(), prevPointsResult.realPoints.begin(), prevPointsResult.realPoints.end());
        prevPoints.insert(prevPoints.end(), prevPointsResult.interpolatedPoints.begin(),
                               prevPointsResult.interpolatedPoints.end());
        prevPoints.insert(prevPoints.end(), prevPointsResult.previousPoints.begin(), prevPointsResult.previousPoints.end());
        ranges::sort(prevPoints);

        auto nextPointsRaw = resultingPoints[nextRow];
        auto nextPointsResult = nextPointsRaw.result;
        vector<int> nextPoints;
        nextPoints.insert(nextPoints.end(), nextPointsResult.realPoints.begin(), nextPointsResult.realPoints.end());
        nextPoints.insert(nextPoints.end(), nextPointsResult.interpolatedPoints.begin(),
                               nextPointsResult.interpolatedPoints.end());
        nextPoints.insert(nextPoints.end(), nextPointsResult.previousPoints.begin(), nextPointsResult.previousPoints.end());
        ranges::sort(nextPoints);

        vector<int> usedPrevPoints;
        if (!prevPoints.empty())
        {
            for (auto& p : nextPoints)
            {
                auto prevPoint = findClosestPoint(prevPoints, p, nextPointsRaw.prevOffset);
                // TODO: Custom spacing
                if (std::abs(p - (prevPoint + nextPointsRaw.prevOffset)) <= nextPointsRaw.avgSpacing / 2)
                {
                    usedPrevPoints.push_back(prevPoint);
                }
            }
        }
        vector<int> unusedPrevPoints;

        if (!prevPoints.empty() && !usedPrevPoints.empty())
            ranges::set_difference(prevPoints, usedPrevPoints, std::back_inserter(unusedPrevPoints));

        auto prevIter = unusedPrevPoints.begin();
        // Check if there isn't a unused real point nearby, if so, use that instead and this prevpoint is actually used
        while (prevIter != unusedPrevPoints.end())
        {
            auto noiseIter = nextPointsResult.noisePoints.begin();
            while (noiseIter != nextPointsResult.noisePoints.end())
            {
                // found real point
                if (std::abs(*noiseIter - (*prevIter + nextPointsRaw.prevOffset)) <= tolerancePercentage * nextPointsRaw.avgSpacing / 2)
                {
                    nextPointsResult.realPoints.push_back(*noiseIter);
                    prevIter = unusedPrevPoints.erase(prevIter);
                    nextPointsResult.noisePoints.erase(noiseIter);
                    break;
                }
                ++noiseIter;
            }
            if (prevIter != unusedPrevPoints.end())
                ++prevIter;
        }

        for (auto& p : unusedPrevPoints)
        {
            nextPointsResult.previousPoints.push_back(p + nextPointsRaw.prevOffset);
        }

        nextPointsRaw.result = nextPointsResult;
        resultingPoints[nextRow] = nextPointsRaw;
    }

    // TODO: Same but reverse

    for (int i = 0; i < allRowKeys.size() - 1; ++i)
    {
        int row = allRowKeys[i];
        AdaptivePatternResult result = resultingPoints[row].result;
        for (auto& p : result.noisePoints)
        {
            cv::circle(original2, cv::Point(p, row), 20, cv::Scalar(0, 0, 0), -1); // red (BGR)
        }
        for (auto& p : result.realPoints)
        {
            cv::circle(original2, cv::Point(p, row), 18, cv::Scalar(0, 0, 255), -1); // red (BGR)
        }
        for (auto& p : result.interpolatedPoints)
        {
            cv::circle(original2, cv::Point(p, row), 18, cv::Scalar(0, 255, 255), -1); // red (BGR)
        }
        for (auto& p : result.previousPoints)
        {
            cv::circle(original2, cv::Point(p, row), 16, cv::Scalar(0, 255, 0), -1); // red (BGR)
        }
    }


    // imwrite(R"(/home/askolds/Downloads/DroniOtsu/3_FIND_LINES.jpg)", output);

    // Optional: Enlarge dots
    // cv::Mat dilated;
    // cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    // cv::dilate(centerDotsMat, dilated, kernel);

    // Convert result back to UMat
    // cv::UMat centerDots;
    // dilated.copyTo(centerDots);

    // imwrite(R"(/home/askolds/Downloads/DroniOtsu/3_FIND.jpg)", original);
    imwrite(R"(/home/askolds/Downloads/DroniOtsu/3_FIND_CLEAN.jpg)", original2);
    // Resize for display
    // cv::UMat resizedOutput;
    // cv::resize(output, resizedOutput, cv::Size(), displayScale, displayScale, cv::INTER_AREA);
    // cv::imshow("Visualization", resizedOutput);
    // cv::waitKey(0);

    // Resize for viewing
    // cv::UMat resizedOutput;
    // cv::resize(output, resizedOutput, cv::Size(), displayScale, displayScale, cv::INTER_AREA);
    //
    // cv::imshow("Visualization", resizedOutput);
    // cv::waitKey(0);
}

int main()
{
    // Load image into UMat directly for GPU-acceleration
    cv::UMat img;
    cv::imread(R"(/home/askolds/Downloads/DroniOtsu/woOutlier/011_A_45m_Contour_otsu_4.jpg)",
               cv::IMREAD_GRAYSCALE).copyTo(img);
    if (img.empty())
    {
        std::cerr << "Failed to load image\n";
        return -1;
    }

    int stripHeight = 60; // height of each horizontal strip
    int threshold = 3; //stripHeight / 2;     // white pixel count threshold

    processImage(img, stripHeight, threshold, 0.1, 0.16, 0.05);

    return 0;
}

// R"(/home/askolds/Downloads\DroniOtsu\2_Otsu_4.jpg)"
// R"(/home/askolds/Downloads\DroniOtsu\2_Otsu_4_TESTING.jpg)"

const char* ColorIndexx(
    const char* src,
    const char* mask,
    const char* workDir,
    const double blue,
    const double green,
    const double red,
    const int nr
)
{
    // Get extension
    const auto srcString = std::string(src);
    std::string extension;
    if (const std::size_t indexLastSeparator = srcString.find_last_of('.'); indexLastSeparator != std::string::npos)
    {
        extension = srcString.substr(indexLastSeparator + 1);
    }
    else
    {
        throw;
    }

    // Read images
    const auto srcMat = imread(src, IMREAD_COLOR);
    const auto maskMat = imread(mask, IMREAD_GRAYSCALE);

    // Get umats
    // auto srcUMat = srcMat.getUMat(ACCESS_RW);
    // auto maskUMat = maskMat.getUMat(ACCESS_RW);
    // auto resultUMat = UMat(srcUMat.size(), CV_32FC1);
    // auto tempUMat = UMat(srcUMat.size(), CV_32FC1);

    printf("Running color index\n");

    UMat srcUMat;
    srcMat.convertTo(srcUMat, CV_32FC3);
    UMat maskUMat;
    maskMat.copyTo(maskUMat);
    auto resultUMat = UMat(srcUMat.size(), CV_32FC1);
    auto tempUMat = UMat(srcUMat.size(), CV_32FC1);

    //Apply to channels
    transform(srcUMat, resultUMat, Matx13f(
                  static_cast<float>(blue),
                  static_cast<float>(green),
                  static_cast<float>(red)
              )
    );

    // Normalize (within the mask)
    normalize(resultUMat, tempUMat, 0, 255, NORM_MINMAX, -1, maskUMat);

    // Clear resultUMat
    resultUMat.setTo(Scalar::all(0));
    // Convert to CV_8UC1
    tempUMat.convertTo(resultUMat, CV_8UC1);

    imwrite(std::format("{}\\{}_ColorIndex.{}", workDir, nr, extension), resultUMat);

    return "Test";
}
