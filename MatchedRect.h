#ifndef MATCHED_RECT_H
#define MATCHED_RECT_H

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
#include <vector>
#include <iterator>
#include <algorithm>

/**
   A simple class that encapsulates the 4-coordinates of matched regions
   of an image to avoid re-scanning the images.
*/
class MatchedRect {
    friend std::ostream& operator<<(std::ostream& os, const MatchedRect& rect) {
        return os << "sub-image matched at: " << rect.row1 << ", " 
                  << rect.col1 << ", " << rect.row2 << ", " << rect.col2;
    }

public:
    /**
     * Default constructor that initializes to an empty rectangle.
     */
    MatchedRect() { row1 = col1 = row2 = col2 = 0; }

    /**
     * Convenience constructor to create a rectangle at a given location.
     * 
     * \param[in] row The top-left corner row for the rectangle.
     * \param[in] col The top-left corner column for the rectangle.
     * \param[in] width The width of the rectangle.
     * \param[in] height The height of the rectangle.
     */
    MatchedRect(int row, int col, int width, int height) :
        row1(row), col1(col), row2(row + height), col2(col + width) {}
    
    /**
     * Convenience method to determine if two rectangular regions instersect
     * each other.
     * 
     * \param[in] other The other rectuangular region to check for intersection.
     */
    inline bool intersects(const MatchedRect& other) const {
        return !((row1 > other.row2) || (row2 < other.row1) ||
                 (col1 > other.col2) || (col2 < other.col1));
    }

    /**
     * Convience overload to enable is class to operate as a Functor. This
     * operator just calls the intersects method.
     * 
     * \param[in] other The other rectuangular region to check for intersection.
     */
    inline bool operator()(const MatchedRect& other) const {
        return intersects(other);
    }
    
    /**
     * Conveience operator to faciltate sorting of rectangles such that the
     * top-most and left-most rectangles are first in the list.
     * 
     * \param[in] other The other rectuangular region for comparison.
     */
    inline bool operator<(const MatchedRect& other) const {
        return ((row1 == other.row1) ? (col1 < other.col1) : 
            (row1 < other.row1));
    }

    // The four corners of the matched region.
    int row1, col1, row2, col2;
};

/**
   A convenience wrapper class around std::vector to encapsulate the
   list of matched regions in the image. 
*/
class MatchedRectList : public std::vector<MatchedRect> {
    /**
     * Convenience stream-insertion operator to print a list of matched
     * rectangular regions.
     */
    friend std::ostream& operator<<(std::ostream& os,
                                    const MatchedRectList& list) {
        std::copy(list.begin(), list.end(),
                  std::ostream_iterator<MatchedRect>(os, "\n"));
        return os;
    }

public:
    inline bool isMatched(const MatchedRect& other) const {
        return find_if(begin(), end(), other) != end();
    }
};

#endif