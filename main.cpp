// Copyright 2023 Nam Hoang
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <omp.h>
#include "PNG.h"

// It is ok to use the following namespace delarations in C++ source
// files only. They must never be used in header files.
using namespace std;
using namespace std::string_literals;

/**
 * Compute the average color of the background region (black region)
 * 
*/
Pixel computeBackgroundPixel(const PNG& img1, const PNG& mask, 
                            const int startRow, const int startCol,
                            const int maxRow, const int maxCol) {
    const Pixel Black{ .rgba = 0xff'00'00'00U };
    int red = 0, blue = 0, green = 0, count = 0;
    for (int row = 0; (row < maxRow); row++) {
        for (int col = 0; (col < maxCol); col++) {
            if (mask.getPixel(row, col).rgba == Black.rgba) {
                const auto pix = img1.getPixel(row + startRow, col + startCol); 
                red += pix.color.red;
                green += pix.color.green;
                blue += pix.color.blue;
                count++;
            }
        }
    }
    const unsigned char avgRed = (red / count), 
                        avgGreen = (green / count), 
                        avgBlue = (blue / count); 
    return {.color = {avgRed, avgGreen, avgBlue, 0}};
}

/**
 * Determine if the two pixels are of the same shade 
 * (the rgb value difference between the two pixels are within certain range)
*/
bool sameShade(const Pixel& pixel1, const Pixel& pixel2, const int tolerance) {
    const int diffRed = abs(static_cast<int>(pixel1.color.red) 
        - static_cast<int>(pixel2.color.red));
    const int diffGreen = abs(static_cast<int>(pixel1.color.green) 
        - static_cast<int>(pixel2.color.green));
    const int diffBlue = abs(static_cast<int>(pixel1.color.blue) 
        - static_cast<int>(pixel2.color.blue));
    return (diffRed < tolerance && diffGreen < tolerance) 
        && (diffBlue < tolerance);
}

/**
 * Count the number of matching pixels in a given range
*/
int calcNetMatch(const PNG& img1, const PNG& mask, 
        const int startRow, const int startCol,
        const int maxRow, const int maxCol, 
        const Pixel& avgPixel, const int tolerance) {
    // Prepare
    int countMatch = 0;
    const Pixel Black{ .rgba = 0xff'00'00'00U };
    // Count matching pixels
    for (int row = 0; (row < maxRow); row++) {
        for (int col = 0; (col < maxCol); col++) {
            // get the current pixel
            const auto pix = img1.getPixel(startRow + row, startCol + col);
            // check same shade with the average pixel
            bool hasSameShade = sameShade(pix, avgPixel, tolerance);
            // check matching pixel
            if ((mask.getPixel(row, col).rgba == Black.rgba && hasSameShade) 
            || (mask.getPixel(row, col).rgba != Black.rgba && !hasSameShade)) {
                countMatch++;
            }
        }
    }
    // Result
    return countMatch - (maxRow * maxCol - countMatch);
}

void drawBox(PNG& png, int row, int col, int width, int height) {
    // Draw horizontal lines
    for (int i = 0; (i < width); i++) {
        png.setRed(row, col + i); 
        png.setRed(row + height - 1, col + i);
    }
    // Draw vertical lines
    for (int i = 0; (i < height); i++) { 
        png.setRed(row + i, col); 
        png.setRed(row + i, col + width - 1);
    }
}

bool pointInRegion(int pointRow, int pointCol, int regRow, int regCol, 
    int regWidth, int regHeight) {
    return (regRow <= pointRow && pointRow <= regRow + regHeight)
        && (regCol <= pointCol && pointCol <= regCol + regWidth);
}

void processPotentialRegion(PNG& png, int row, int col, int width, int height, 
    vector<vector<int>>& regions) {
    // prepare
    vector<vector<int>> offsets = 
        {{0, 0}, {height - 1, 0}, {0, width - 1}, {height - 1, width - 1}};

    // check if this new region is overlap by any existsing regions
    for (const auto& region : regions) {
        for (const auto& offset : offsets) {
            if (pointInRegion(row + offset[0], col + offset[1], 
                region[0], region[1], width, height)) {
                    // the point is in an existing region -> stop
                    return;
            }
        }
    }

    // if pass all check, then add to result
    regions.push_back({row, col});

    // draw a red line to cover
    drawBox(png, row, col, width, height);
}

void printResult(const vector<vector<int>>& regions, int width, int height) {
    for (const auto& region : regions) {
        const int row = region[0], col = region[1];
        cout << "sub-image matched at: " << row << ", " << col << ", " 
            << row + height << ", " << col + width << endl;
    }
    cout << "Number of matches: " << regions.size() << endl;
}

/**
 * This is the top-level method that is called from the main method to 
 * perform the necessary image search operation. 
 * 
 * \param[in] mainImageFile The PNG image in which the specified searchImage 
 * is to be found and marked (for example, this will be "Flag_of_the_US.png")
 * 
 * \param[in] srchImageFile The PNG sub-image for which we will be searching
 * in the main image (for example, this will be "star.png" or "start_mask.png") 
 * 
 * \param[in] outImageFile The output file to which the mainImageFile file is 
 * written with search image file highlighted.
 * 
 * \param[in] isMask If this flag is true then the searchImageFile should 
 * be deemed as a "mask". The default value is false.
 * 
 * \param[in] matchPercent The percentage of pixels in the mainImage and
 * searchImage that must match in order for a region in the mainImage to be
 * deemed a match.
 * 
 * \param[in] tolerance The absolute acceptable difference between each color
 * channel when comparing  
 */
void imageSearch(const std::string& mainImageFile,
                const std::string& srchImageFile, 
                const std::string& outImageFile, const bool isMask = true, 
                const int matchPercent = 75, const int tolerance = 32) {
    // Implement this method using various methods or even better
    // use an object-oriented approach.

    // Load the image files into memory
    PNG searchImg, subImg;
    searchImg.load(mainImageFile);
    subImg.load(srchImageFile);
    PNG resImg(searchImg);

    // sub image width and height
    int subWidth = subImg.getWidth(), subHeight = subImg.getHeight();
    // a vector containt all regions founded in format of [row, col]
    vector<vector<int>> regs;

    // Scan for region
    for (int row = 0; row < searchImg.getHeight() - subHeight + 1; row++) {
        for (int col = 0; col < searchImg.getWidth() - subWidth + 1; col++) {
            // average pixel in this region
            Pixel avgPixel = computeBackgroundPixel(searchImg, subImg, 
                row, col, subHeight, subWidth);
            // calculate netmatch by verifying match/mismatch pixels
            int netMatch = calcNetMatch(searchImg, subImg, row, 
                col, subHeight, subWidth, avgPixel, tolerance);
            // see if this is a match
            if (netMatch > subHeight * subWidth * matchPercent / 100.0) {
                // found a potential match
                processPotentialRegion(resImg, row, col, 
                    subWidth, subHeight, regs);
            }
        }
    }
    // Final stat
    printResult(regs, subWidth, subHeight);
    // Write out result image
    resImg.write(outImageFile);
}

/**
 * The main method simply checks for command-line arguments and then calls
 * the image search method in this file.
 * 
 * \param[in] argc The number of command-line arguments. This program
 * needs at least 3 command-line arguments.
 * 
 * \param[in] argv The actual command-line arguments in the following order:
 *    1. The main PNG file in which we will be searching for sub-images
 *    2. The sub-image or mask PNG file to be searched-for
 *    3. The file to which the resulting PNG image is to be written.
 *    4. Optional: Flag (True/False) to indicate if the sub-image is a mask 
 *       (deault: false)
 *    5. Optional: Number indicating required percentage of pixels to match
 *       (default is 75)
 *    6. Optiona: A tolerance value to be specified (default: 32)
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        // Insufficient number of required parameters.
        std::cout << "Usage: " << argv[0] << " <MainPNGfile> <SearchPNGfile> "
                  << "<OutputPNGfile> [isMaskFlag] [match-percentage] "
                  << "[tolerance]\n";
        return 1;
    }
    const std::string True("true");
    // Call the method that starts off the image search with the necessary
    // parameters.
    imageSearch(argv[1], argv[2], argv[3],       // The 3 required PNG files
        (argc > 4 ? (True == argv[4]) : true),   // Optional mask flag
        (argc > 5 ? std::stoi(argv[5]) : 75),    // Optional percentMatch
        (argc > 6 ? std::stoi(argv[6]) : 32));   // Optional tolerance

    return 0;
}

// End of source code
