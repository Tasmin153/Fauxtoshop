
#include <iostream>
#include "console.h"
#include "gwindow.h"
#include "grid.h"
#include "simpio.h"
#include "strlib.h"
#include "gbufferedimage.h"
#include "gevents.h"
#include "math.h" //for sqrt and exp in the optional Gaussian kernel
#include "random.h"

using namespace std;

static const int WHITE = 0xFFFFFF;
static const int BLACK = 0x000000;
static const int GREEN = 0x00FF00;
static const double PI = 3.14159265;

void doFauxtoshop(GWindow &gw, GBufferedImage &img);
bool getImage(GBufferedImage &img, GWindow &gw);
void pickFilter(GBufferedImage& img);
void doFilter(GBufferedImage &img, int n);
bool openImage(GWindow &gw, GBufferedImage &img);

Grid<int> doScatter(Grid<int> &original);
Grid<int> doEdgeDetection(Grid<int> &original);
Grid<int> doGreenScreen(Grid<int> &original);
void doCompare(GBufferedImage &img);
void getSecondImg(GBufferedImage &img);
void getStickerLocation(Grid<int> &original, int &row, int &col);
bool isRowOrColWithinStickerBounds(int stickerLength, int start, int curr);
void overlaySticker(Grid<int> &background, Grid<int> &greenscreened, Grid<int> &sticker, int threshold, int stickerOriginX, int stickerOriginY);
bool isOutsideGreenThreshold(int pixel, int threshold);
int assignEdgeDetectionColors(int threshold, Grid<int> &original, int r, int c);
int diffBtwnPixels(int pixelA, int pixelB);
int getThreshold(string prompt);
int getRandCoord(int radius, int max, int current);
int	setLow(int radius, int n);
int	setHigh(int radius, int n, int max);

bool convertStringToInts(Grid<int> &original, string str, int &row, int &col);

bool openImageFromFilename(GBufferedImage &img, string filename);
bool saveImageToFilename(const GBufferedImage &img, string filename);
void getMouseClickLocation(int &row, int &col);

/* 
 * This main declares a GWindow and a GBufferedImage for use
 * throughout the program and calls doFauxtoShop function.
 */
int main() {
    GWindow gw;
    gw.setTitle("Fauxtoshop");
    gw.setVisible(true);
    GBufferedImage img;
    doFauxtoshop(gw, img);
    return 0;
}

/*
 * Kicks off Fauxtoshop program
 * with prompts to user.
 */
void doFauxtoshop(GWindow &gw, GBufferedImage &img) {

    while (true) {
        cout << "Welcome to Fauxtoshop!" << endl;
        if (!openImage(gw, img)) { // Opens and displays image file. If user enters blank string, quits app.
            return; 
        }

        pickFilter(img); // Prompts user to pick a filter

        while (true) { // Asks user if they would like to save image
            string filename = getLine("Enter filename to save image (or blank to skip saving): ");
            if (filename == "" || saveImageToFilename(img, filename.c_str())) {
                   break;
           }
        }

        gw.clear(); // Clears GWindow
        cout << "\n" <<endl;
        

    }
}

/* 
 * Opens image and adds to GWindow.
 * Returns false if user enters blank line when prompted for filename.
 */
bool openImage(GWindow &gw, GBufferedImage &img) {

        if (!getImage(img, gw)) {
            return false;
        }

        gw.setCanvasSize(img.getWidth(), img.getHeight()); // Resize GWindow to be same size as image
     
        gw.add(&img,0,0); // Add image to GWindow
        return true;
}

/*
 * Prompt user for image filename and open image. Closes application if blank string is entered.
 * Return true if image is opened, return false if blank string entered.
 */
bool getImage(GBufferedImage &img, GWindow &gw) {

    while (true) {
        string filename = getLine("Enter name of image file to edit (or blank to quit):");
        // Attempt to open file. Breaks out of the loop if filename is valid. 
        if (openImageFromFilename(img, filename.c_str())) {

            break;
        }
        
	if (filename == "") {
	    cout << "Quitting the application. You may close the console window." << endl;
	    gw.close();
	    return false;
	}
        cout << "Couldn't open that file. Please try again." << endl;
    }
    return true;
}

/* Attempts to open the image file 'filename'.
 *
 * This function returns true when the image file was successfully
 * opened and the 'img' object now contains that image, otherwise it
 * returns false.
 */
bool openImageFromFilename(GBufferedImage& img, string filename) {
    cout << "Opening image file, may take a minute..." << endl;
    try { img.load(filename); }
    catch (...) { return false; }
    return true;
}

/* Asks the user which filter they would like to apply to the image file */
void pickFilter(GBufferedImage& img) {
    int n;
	while (true) {
        n = getInteger("Which image filter would you like to apply?\n\t1 - Scatter\n\t2 - Edge Detection\n\t3 - \"Green screen\" with another image\n\t4 - Compare image with another image\nYour choice: ");
        if (n > 0 && n <= 4) {
                break; // Break out of loop when user enters a valid number
        }
        cout << "You entered an invalid number. Let's try this again." << endl;
    }
    doFilter(img, n);
}

/* Starts the correct filter function */
void doFilter(GBufferedImage& img, int n) {
    Grid<int> original = img.toGrid();
    Grid<int> filtered;
    switch(n) {
        case 1: filtered = doScatter(original);
                img.fromGrid(filtered);
                break;
        case 2: filtered = doEdgeDetection(original);
                img.fromGrid(filtered);
                break;
        case 3: filtered = doGreenScreen(original);
                img.fromGrid(filtered);
                break;
        case 4: doCompare(img);
                break;
        default: cout << "You entered an invalid number" << endl;
                 break;
    }
}

/* Applies the scatter filter to the image.
 * Prompts user for scatter radius. Iterates through Grid. 
 */
Grid<int> doScatter(Grid<int>& original) {
    int radius = getInteger("Enter degree of scatter [1 - 100]: ");
    Grid<int> scattered(original.numRows(), original.numCols());
    for (int r = 0; r < scattered.numRows(); r++) {
        for (int c = 0; c < scattered.numCols(); c++) {
            scattered[r][c] = original[getRandCoord(radius, scattered.numRows(), r)][getRandCoord(radius, scattered.numCols(), c)];
	}
    }
    return scattered;
}

/*
 * Returns a random column or row within the radius of the current column or row.
 * Will not return a coordinate outside the bounds of the grid.
 */
int getRandCoord(int radius, int max, int current) {
    int low = setLow(radius, current);
    int high = setHigh(radius, current, max);
    return randomInteger(low, high);
}

/* Sets the lower radius boundary so that it stays inbounds */
int setLow(int radius, int n) {
    if ((n - radius) < 0) {
        return 0;
    }
    return n - radius;
}

/* Sets upper radius boundary so that it stays inbounds */
int setHigh(int radius, int n, int max) {
    if ((n + radius) >= max) {
        return max - 1;
    }
        return n + radius;
}

/* Returns a Grid<int> with the edge detection filter applied to the Grid<int> argument passed in. */
Grid<int> doEdgeDetection(Grid<int>& original) {
    int threshold = getThreshold("Enter threshold for edge detection: ");
    Grid<int> edged(original.numRows(), original.numCols());
    // Loop through each pixel in the grid
    for (int r = 0; r < edged.numRows(); r++) {
        for (int c = 0; c < edged.numCols(); c++) {
            edged[r][c] = assignEdgeDetectionColors(threshold, original, r, c);
        }
    }
    return edged;
}

/* 
 * Returns black if the difference between the pixel and its neighbors
 * is greater than the threshold. If not, returns white.
 */
int assignEdgeDetectionColors(int threshold, Grid<int>& original, int r, int c) {
    int pixel = original[r][c];
    int neighborPixel; 
    for (int row = r - 1; row <= r + 1; row++) {
        for (int col = c - 1; col <= c + 1; col++) {
            if (original.inBounds(row, col)) {
                neighborPixel = original[row][col];
                if (diffBtwnPixels(pixel, neighborPixel) > threshold) {
                    return BLACK;
                }
            }
        }
    }
    return WHITE;
}


// Returns the max difference between pairs of RGB values.
int diffBtwnPixels(int pixelA, int pixelB) {
    int redA, greenA, blueA, redB, greenB, blueB;
    GBufferedImage::getRedGreenBlue(pixelA, redA, greenA, blueA);
    GBufferedImage::getRedGreenBlue(pixelB, redB, greenB, blueB);

    int diffA = abs(redA - redB);
    int diffB = abs(greenA - greenB); 
    int diffC = abs(blueA - blueB);

    return max(max(diffA, diffB), diffC);
}

// Prompts the user for a positive, nonzero integer until it is input. Returns the integer.
int getThreshold(string prompt) {
    int threshold;
    while (true) {
        threshold = getInteger(prompt);
        if (threshold >= 0) {
            break;
        }
    }

    return threshold;
} 

// Implements green screen filter on grid<int> argument.
Grid<int> doGreenScreen(Grid<int>& original) {
    Grid<int> greenscreened(original.numRows(), original.numCols());
    GBufferedImage sticker;
    int stickerRow;
    int stickerCol;
    
    cout << "Now choose another file to add to your background image" << endl;

    getSecondImg(sticker); // Open the file input by the user
    Grid<int> stickerGrid = sticker.toGrid(); // Convert sticker image to Grid<int>
    int threshold = getThreshold("Now choose a tolerance threshold: ");
    getStickerLocation(original, stickerRow, stickerCol);
    overlaySticker(original, greenscreened, stickerGrid, threshold, stickerRow, stickerCol);

    return greenscreened; 
}

/* Convert image to Grid<int> */


/* Prompts the user to enter an image filename. If valid, assigns image to img.
 * Continues to prompt in a loop until valid filename entered. */
void getSecondImg(GBufferedImage &img) {
    while (true) {
        string filename = getLine("Enter name of image file to open: ");
        // Attempt to open file. Breaks out of the loop if filename is valid. 
        if (openImageFromFilename(img, filename.c_str())) {
            break;
        }
        cout << "Couldn't open that file. Please try again." << endl;
    }
}

/* Prompts user to enter the desired location for the sticker image.
 * If blank string is entered, allows the user to set the location with the mouse.
 */
void getStickerLocation(Grid<int> &original, int &row, int &col) {
    while (true) {
        string location = getLine("Enter location to place image as \"(row,col)\" (or blank to use mouse): ");
        if (location == "") {
            cout << "Now click the background image to place new image:" << endl;
            getMouseClickLocation(row, col);
            cout << "You chose (" << row << "," << col << ")" << endl;
            break;
        } else {
            if (convertStringToInts(original, location, row, col)) {
                break;
            }
            cout << "Invalid entry. Make sure your entry is in bounds and in the correct format: \"(row,col)\"." << endl;
        }
    }
}

/* Converts the location string input "(col,row)" into two ints, if valid.
 * Returns true if valid, assigning row and col the values. Else returns false.
 */
bool convertStringToInts(Grid<int> &original, string str, int &row, int &col) {
   int indexOfComma = stringIndexOf(str, ","); // Find index of the comma
   int rowLen = indexOfComma - stringIndexOf(str, "(") -1; 
   int colLen = stringIndexOf(str, ")") - indexOfComma - 1;
   string rowStr = str.substr(1, rowLen); 
   string colStr = str.substr(indexOfComma + 1, colLen);

   if (stringIsInteger(rowStr) && stringIsInteger(colStr)) { // If substrings valid, convert to integers
       row = stringToInteger(rowStr);
       col = stringToInteger(colStr);
           if (original.inBounds(row, col)) {
               return true;
           }
   }
   
   return false;
}

/* Overlays the sticker image on the original background image.
 * Assigns the filtered Grid<int> greenscreened the pixels of the new hybrid image. 
 * Ignores pixels on the sticker that fall within the green threshold.
 */

void overlaySticker(Grid<int> &background, Grid<int> &greenscreened, Grid<int> &sticker, int threshold, int stickerOriginRow, int stickerOriginCol) {
    int sRow = 0; // Sticker row

    for (int bgRow = 0; bgRow < background.numRows(); bgRow++) {

        int sCol = 0; // Sticker column
        
        for (int bgCol = 0; bgCol < background.numCols(); bgCol++) {

            if (isRowOrColWithinStickerBounds(sticker.numRows(), stickerOriginRow, bgRow) && isRowOrColWithinStickerBounds(sticker.numCols(), stickerOriginCol, bgCol)) {
                
                if (isOutsideGreenThreshold(sticker[sRow][sCol], threshold)) { // if sticker pixel falls outside threshold, add sticker img pixel
                        greenscreened[bgRow][bgCol] = sticker[sRow][sCol];
                } else {
                    greenscreened[bgRow][bgCol] = background[bgRow][bgCol]; // if sticker pixel is within green threshold, add background img pixel
                }
                sCol++;
            } else {
                greenscreened[bgRow][bgCol] = background[bgRow][bgCol]; // if outside bounds of sticker position, add background img pixel
            }
        }

        if (isRowOrColWithinStickerBounds(sticker.numRows(), stickerOriginRow, bgRow)) { // If adding pixels from the sticker grid, increment sRow to loop
                sRow++;
        }
    } 
}

/* Returns true if the row or col is within the bounds of where the sticker is to be overlaid */
bool isRowOrColWithinStickerBounds(int stickerLength, int start, int curr) {
    int max = start + stickerLength - 1;

    if (curr >= start && curr < max) {
        return true;
    }
    return false;
}

/* Returns true if column is within bounds, false if not */
bool isColWithinStickerBounds(int numStickerRows, int startRow, int currRow) {
    int maxRow = startRow + numStickerRows - 1;

    if (currRow >= startRow && currRow < maxRow) {
        return true;
    }
    return false;
}
/* Returns true if the pixel's shade of green falls outside of the threshold
 *
 * Compares the pixel's green values to the const GREEN green value.
 * If difference is greater than threshold, returns true. Else returns false
 */
bool isOutsideGreenThreshold(int pixel, int threshold) {

    int redA, greenA, blueA, redB, greenB, blueB;
    GBufferedImage::getRedGreenBlue(pixel, redA, greenA, blueA);
    GBufferedImage::getRedGreenBlue(GREEN, redB, greenB, blueB);

    int diff = abs(greenA - greenB);
    if (diff > threshold) {
        return true;
    }
    return false;
}

/*  Attempts to save the image file to 'filename'.
 *
 * This function returns true when the image was successfully saved
 * to the file specified, otherwise it returns false.
 */
bool saveImageToFilename(const GBufferedImage &img, string filename) {
    try { img.save(filename); }
    catch (...) { return false; }
    cout << "Image saved." << endl;
    return true;
}

/* 
 * Waits for a mouse click in the GWindow and reports click location.
 *
 * When this function returns, row and col are set to the row and
 * column where a mouse click was detected.
 */
void getMouseClickLocation(int &row, int &col) {
    GMouseEvent me;
    do {
        me = getNextEvent(MOUSE_EVENT);
    } while (me.getEventType() != MOUSE_CLICKED);
    row = me.getY();
    col = me.getX();
}

/* Prints the number of pixels that differ between two images */
void doCompare(GBufferedImage &img) {

    GBufferedImage img2;
    getSecondImg(img2);
    cout << "These images differ in " << img.countDiffPixels(img2) << " pixel locations!" << endl;
}

/* 
 * Takes a radius and computes a 1-dimensional Gaussian blur kernel
 * with that radius. The 1-dimensional kernel can be applied to a
 * 2-dimensional image in two separate passes: first pass goes over
 * each row and does the horizontal convolutions, second pass goes
 * over each column and does the vertical convolutions. This is more
 * efficient than creating a 2-dimensional kernel and applying it in
 * one convolution pass.
 *
 * This code is based on the C# code posted by Stack Overflow user
 * "Cecil has a name" at this link:
 * http://stackoverflow.com/questions/1696113/how-do-i-gaussian-blur-an-image-without-using-any-in-built-gaussian-functions
 *
 */
Vector<double> gaussKernelForRadius(int radius) {
    if (radius < 1) {
        Vector<double> empty;
        return empty;
    }
    Vector<double> kernel(radius * 2 + 1);
    double magic1 = 1.0 / (2.0 * radius * radius);
    double magic2 = 1.0 / (sqrt(2.0 * PI) * radius);
    int r = -radius;
    double div = 0.0;
    for (int i = 0; i < kernel.size(); i++) {
        double x = r * r;
        kernel[i] = magic2 * exp(-x * magic1);
        r++;
        div += kernel[i];
    }
    for (int i = 0; i < kernel.size(); i++) {
        kernel[i] /= div;
    }
    return kernel;
}
