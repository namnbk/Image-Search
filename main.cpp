//--------------------------------------------------------------------
//
// Copyright (C) 2023 raodm@miamiOH.edu
//
// Miami University makes no representations or warranties about the
// suitability of the software, either express or implied, including
// but not limited to the implied warranties of merchantability,
// fitness for a particular purpose, or non-infringement.  Miami
// University shall not be liable for any damages suffered by licensee
// as a result of using, result of using, modifying or distributing
// this software or its derivatives.
//
// By using or copying this Software, Licensee agrees to abide by the
// intellectual property laws, and all other applicable laws of the
// U.S., and the terms of GNU General Public License (version 3).
//
// Authors:   Dhananjai M. Rao          raodm@miamioh.edu
//
//---------------------------------------------------------------------

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
#include "MatchedRect.h"

// It is ok to use the following namespace delarations in C++ source
// files only. They must never be used in header files.
using namespace std;
using namespace std::string_literals;

/**
 * Helper method to compute the average background pixel color for a given 
 * region of the image based on a max. 
 * 
 * \param[in] img1 The image whose region is used to be used to compute the
 * average pixel color.
 * 
 * \param[in] mask The mask to be used to determine the pixels that logically
 * constitute the background.
 * 
 * \param[in] startRow The starting row in img1
 * 
 * \param[in] endRow The starting column in img1 
 * 
 * \param[in] maxRow The maximum number of rows from the starting row to be used
 * to compute the background. This is zero-based to ensure that the computation
 * does not exceed the image size.
 * 
 * \param[in] maxCol The maximum number of columns from the starting column to be 
 * used to compute the background. This is zero-based to ensure that the 
 * computation does not exceed the image size.
 * 
 * \return Returns the average pixel color for the given region.
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
                red   += pix.color.red;
                green += pix.color.green;
                blue  += pix.color.blue;
                count++;
            }
        }
    }
    // Compute the average color for each of the channels. 
    const unsigned char avgRed  = (red / count), avgGreen = (green / count),
                        avgBlue = (blue / count);
    return {.color = {avgRed, avgGreen, avgBlue, 255}};
}

/**
 * Helper method to compute the average background pixel color for a given 
 * region of the image based on a max. 
 * 
 * \param[in] img1 The image whose region is used to be used to compute the
 * average pixel color.
 * 
 * \param[in] mask The mask to be used to determine the pixels that logically
 * constitute the background.
 * 
 * \param[in] startRow The starting row in img1
 * 
 * \param[in] endRow The starting column in img1 
 * 
 * \param[in] maxRow The maximum number of rows from the starting row to be used
 * to compute the background. This is zero-based to ensure that the computation
 * does not exceed the image size.
 * 
 * \param[in] maxCol The maximum number of columns from the starting column to be 
 * used to compute the background. This is zero-based to ensure that the 
 * computation does not exceed the image size.
 * 
 * \param[in] tolerance The acceptable tolerance on the red, green, or blue
 * channels for each pixel.
 * 
 * \return Returns the average pixel color for the given region.
 */
int getMatchingPixCount(const PNG& img1, const PNG& mask,
        const int startRow, const int startCol,
        const int maxRow, const int maxCol, const int tolerance) {
    const auto inTolerance = [&tolerance](int c1, int c2) 
        { return std::abs(c1 - c2) < tolerance; };
    const Pixel Black{ .rgba = 0xff'00'00'00U };

    // First compute the average background pixel color.
    const Pixel bgPix = computeBackgroundPixel(img1, mask, startRow, startCol, 
        maxRow, maxCol);
    int matchingPixelCount = 0;
    for (int row = 0; (row < maxRow); row++) {
        for (int col = 0; (col < maxCol); col++) {
            const auto imgPix  = img1.getPixel(row + startRow, col + startCol);
            const auto maskPix = mask.getPixel(row, col);
            const bool isPixDiff =  
                (inTolerance(imgPix.color.red,   bgPix.color.red)   &&
                 inTolerance(imgPix.color.green, bgPix.color.green) &&
                 inTolerance(imgPix.color.blue,  bgPix.color.blue));
            const int addSub  = (maskPix.rgba == Black.rgba) ? -1 : 1;
            matchingPixelCount += addSub * (isPixDiff ? -1 : 1);
            /*
            std::cout << row << '\t' << col << '\t'
                      << "(" << (int) imgPix.color.red << ',' << (int) imgPix.color.green
                      << ',' << (int) imgPix.color.blue << ")\t("
                      << (int) maskPix.color.red << ',' << (int) maskPix.color.green
                      << ',' << (int) maskPix.color.blue << '\t' << matchingPixelCount
                      << std::endl;
            */
        }
    }
    return matchingPixelCount;
}

/**
 * This helper method is given to draw a rectangular box around a matching 
 * region.
 * 
 * \param[in] img The image in which the red box is to be drawn.
 * 
 * \param[in] box The region of the box where the red box is to be drawn.
 */
void drawRedBox(PNG& img, const MatchedRect& box) {
    // Draw horizontal lines for the box.
    for (int col = box.col1; (col < box.col2); col++) {
        img.setRed(box.row1, col);
        img.setRed(box.row2 - 1, col);
    }
    // Draw vertical lines for the box.
    for (int row = box.row1; (row < box.row2); row++) {
        img.setRed(row, box.col1);
        img.setRed(row, box.col2);
    }    
}

/**
 * Helper method to check if a given region in an image matches the mask.
 * 
 * \param[in] img The main image for checking. A box is drawn in this image if
 * the given srchRgn matches.
 * 
 * \param[in] mask The mask image to be used.
 * 
 * \param[in] mrl The list of previous matched rectangular regions. These 
 * regions are to be ignored. 
 * 
 * \param[in] srchRgn The new rectangular region in the main img to be checked 
 * for a match.
 * 
 * \param[in] pixMatchNeeded The number of matching pixels needed to determine
 * if the specified region is a match.
 * 
 * \param[in] tolerance The absolute acceptable difference between each color
 * channel when comparing  
 */
bool checkMatchRegion(PNG& img, const PNG& mask, MatchedRectList& mrl, 
    const MatchedRect& srchRgn, const int pixMatchNeeded, const int tolerance) {
    // Check for matching regions
    bool matched;
#pragma omp critical(resultVector) 
{
    matched = mrl.isMatched(srchRgn);
}
    if (matched) {
        // Current search rgion is already part of a region
        // matched earlier in this method (same as thread).
        return false;  // not matched
    }

    // Next compute the pixels that match based on tolerance
    const int matchingPixs = getMatchingPixCount(img, mask, srchRgn.row1,
        srchRgn.col1, srchRgn.row2 - srchRgn.row1, 
        srchRgn.col2 - srchRgn.col1, tolerance);
    if (matchingPixs > pixMatchNeeded) {
        // Found a matching region.
        // std::cout << srchRgn << std::endl;
#pragma omp critical(drawing)
        drawRedBox(img, srchRgn);  // hope this won't cause a race condition
#pragma omp critical(resultVector)
    {
        mrl.push_back(srchRgn);  // add matched region to list of matches
    }
        return true;  // found a matching region!
    }
    return false;  // no match
}

void processResult(MatchedRectList& mrl, PNG& img) {
    // Sort the result
    std::sort(mrl.begin(), mrl.end());
    // For each rectangular in a sorted order
    for (const auto& srchRgn : mrl) {
        // Process each matched region by drawing and printing
        std::cout << srchRgn << std::endl;
        // drawRedBox(img, srchRgn);
    }
}

/**
 * This is the top-level method that is called from the main method to 
 * perform the necessary image search operation. 
 * 
 * \param[in] mainImageFile The PNG image in which the specified searchImage 
 * is to be found and marked (for example, this will be "Flag_of_the_US.png")
 * 
 * \param[in] maskImageFile The PNG sub-image for which we will be searching
 * in the main image (for example, this will be "star_mask.png") 
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
                const std::string& maskImageFile, 
                const std::string& outImageFile, const bool isMask = true, 
                const int matchPercent = 75, const int tolerance = 32) {
    // Load the main image and the mask to be used.
    PNG img, mask;
    img.load(mainImageFile);
    mask.load(maskImageFile);
    // The following matched-rectangle-list holds the list of rectangular
    // regions in the image that have already been matched.
    MatchedRectList mrl;
    const int maxRow = img.getHeight() - mask.getHeight();
    const int maxCol = img.getWidth()  - mask.getWidth();
    const int pixMatchNeeded = mask.getBufferSize() * matchPercent / 400;
    // Multi-threaded searching image row-by-row and column-by-column 
    // boxing out matching regions
#pragma omp parallel for default(shared)
    for (int row = 0; (row <= maxRow); row++) {
        for (int col = 0; (col <= maxCol); col++) {
            // Create a rectangle representing the region we are going to 
            // check for a matching image.
            const MatchedRect srchRegion(row, col,
                std::min(img.getWidth()  - col, mask.getWidth()),
                std::min(img.getHeight() - row, mask.getHeight()));
            // Use an helper method to perform the check.
            checkMatchRegion(img, mask, mrl, srchRegion, pixMatchNeeded, 
                            tolerance);
        }
    }
    // Finally, print some result and write out result image
    processResult(mrl, img);
    std::cout << "Number of matches: " << mrl.size() << std::endl;
    img.write(outImageFile);
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
