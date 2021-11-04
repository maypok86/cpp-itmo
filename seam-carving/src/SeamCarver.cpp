#include "SeamCarver.h"

#include <cmath>

using Pixel = Image::Pixel;

namespace {
size_t SubtractColor(const size_t first, const size_t second)
{
    return std::max(first, second) - std::min(first, second);
}

size_t CalcGradient(const Pixel & first, const Pixel & second)
{
    const size_t r = SubtractColor(first.m_red, second.m_red);
    const size_t g = SubtractColor(first.m_green, second.m_green);
    const size_t b = SubtractColor(first.m_blue, second.m_blue);
    return (r * r) + (g * g) + (b * b);
}

} // anonymous namespace

SeamCarver::SeamCarver(Image image)
    : m_image(std::move(image))
{
}

const Image & SeamCarver::GetImage() const
{
    return m_image;
}

size_t SeamCarver::GetImageWidth() const
{
    return m_image.GetWidth();
}

size_t SeamCarver::GetImageHeight() const
{
    return m_image.GetHeight();
}

size_t SeamCarver::CalcHorizontalIndex(size_t index) const
{
    return (index + GetImageWidth()) % GetImageWidth();
}

size_t SeamCarver::CalcVerticalIndex(size_t index) const
{
    return (index + GetImageHeight()) % GetImageHeight();
}

double SeamCarver::GetPixelEnergy(const size_t columnId, const size_t rowId) const
{
    Pixel left = m_image.GetPixel(CalcHorizontalIndex(columnId - 1), rowId);
    Pixel right = m_image.GetPixel(CalcHorizontalIndex(columnId + 1), rowId);
    Pixel up = m_image.GetPixel(columnId, CalcVerticalIndex(rowId + 1));
    Pixel down = m_image.GetPixel(columnId, CalcVerticalIndex(rowId - 1));
    return sqrt(CalcGradient(right, left) + CalcGradient(up, down));
}

SeamCarver::Seam SeamCarver::FindHorizontalSeam() const
{
    return FindSeam(true);
}

SeamCarver::Seam SeamCarver::FindVerticalSeam() const
{
    return FindSeam(false);
}

void SeamCarver::RemoveHorizontalSeam(const Seam & seam)
{
    RemoveSeam(
            GetImageWidth(), GetImageHeight() - 1, [&seam](const size_t columnId, const size_t rowId) { return rowId >= seam[columnId]; }, [this](const size_t columnId, const size_t rowId) { return m_image.GetPixel(columnId, rowId + 1); });
}

void SeamCarver::RemoveVerticalSeam(const Seam & seam)
{
    RemoveSeam(
            GetImageWidth() - 1, GetImageHeight(), [&seam](const size_t columnId, const size_t rowId) { return columnId >= seam[rowId]; }, [this](const size_t columnId, const size_t rowId) { return m_image.GetPixel(columnId + 1, rowId); });
}

SeamCarver::Seam SeamCarver::FindSeam(const bool isTranspose) const
{
    const size_t width = isTranspose ? GetImageHeight() : GetImageWidth();
    const size_t height = isTranspose ? GetImageWidth() : GetImageHeight();
    std::vector<std::vector<double>> energyTable(width, std::vector<double>(height));
    for (size_t columnId = 0; columnId < width; ++columnId) {
        for (size_t rowId = 0; rowId < height; ++rowId) {
            if (isTranspose) {
                energyTable[columnId][rowId] = GetPixelEnergy(rowId, columnId);
            }
            else {
                energyTable[columnId][rowId] = GetPixelEnergy(columnId, rowId);
            }
        }
    }
    std::vector<std::vector<double>> distanceTo(width, std::vector<double>(height, INFINITY));
    std::vector<std::vector<size_t>> pathTo(width, std::vector<size_t>(height));
    for (size_t columnId = 0; columnId < width; ++columnId) {
        distanceTo[columnId][0] = energyTable[columnId][0];
        pathTo[columnId][0] = columnId;
    }
    for (size_t rowId = 0; rowId < height - 1; ++rowId) {
        for (size_t columnId = 0; columnId < width; ++columnId) {
            if (columnId > 0) {
                const double neighbour = energyTable[columnId - 1][rowId + 1];
                if (distanceTo[columnId][rowId] + neighbour < distanceTo[columnId - 1][rowId + 1]) {
                    distanceTo[columnId - 1][rowId + 1] = distanceTo[columnId][rowId] + neighbour;
                    pathTo[columnId - 1][rowId + 1] = columnId;
                }
            }
            if (columnId < width - 1) {
                const double neighbour = energyTable[columnId + 1][rowId + 1];
                if (distanceTo[columnId][rowId] + neighbour < distanceTo[columnId + 1][rowId + 1]) {
                    distanceTo[columnId + 1][rowId + 1] = distanceTo[columnId][rowId] + neighbour;
                    pathTo[columnId + 1][rowId + 1] = columnId;
                }
            }
            const double neighbour = energyTable[columnId][rowId + 1];
            if (distanceTo[columnId][rowId] + neighbour < distanceTo[columnId][rowId + 1]) {
                distanceTo[columnId][rowId + 1] = distanceTo[columnId][rowId] + neighbour;
                pathTo[columnId][rowId + 1] = columnId;
            }
        }
    }
    double minEnergy = INFINITY;
    size_t minPoint = 0;
    for (size_t columnId = 0; columnId < width; ++columnId) {
        if (distanceTo[columnId][height - 1] < minEnergy) {
            minEnergy = distanceTo[columnId][height - 1];
            minPoint = columnId;
        }
    }
    Seam seam(height);
    size_t columnId = minPoint;
    for (size_t rowId = 0; rowId < height; ++rowId) {
        seam[height - 1 - rowId] = columnId;
        columnId = pathTo[columnId][height - 1 - rowId];
    }
    return seam;
}

template <class T, class F>
void SeamCarver::RemoveSeam(const size_t width, const size_t height, const T & test, const F & getPixel)
{
    std::vector<std::vector<Pixel>> table(width, std::vector<Pixel>(height));
    for (size_t columnId = 0; columnId < width; ++columnId) {
        for (size_t rowId = 0; rowId < height; ++rowId) {
            if (test(columnId, rowId)) {
                table[columnId][rowId] = getPixel(columnId, rowId);
            }
            else {
                table[columnId][rowId] = m_image.GetPixel(columnId, rowId);
            }
        }
    }
    m_image.m_table = table;
}
