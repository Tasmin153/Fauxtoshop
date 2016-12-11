
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

static const int    WHITE = 0xFFFFFF;
static const int    BLACK = 0x000000;
static const int    GREEN = 0x00FF00;
static const double PI    = 3.14159265;

void doFauxtoshop(GWindow &gw, GBufferedImage &img);
bool getImage(GBufferedImage& img, GWindow& gw);
void pickFilter(GBufferedImage& img);
void	doFilter(GBufferedImage& img, int n);

Grid<int> doScatter(Grid<int>& original);
Grid<int> doEdgeDetection(Grid<int>& original);
// void	doGreenScreen(Grid<int>& original);
// void	doCompare(Grid<int>& original);
int assignEdgeDetectionColors(int threshold, Grid<int>& original, int r, int c);
int diffBtwnPixels(int pixelA, int pixelB);
int getThreshold();
int getRandCoord(int radius, int max, int current);
int	setLow(int radius, int n);
int	setHigh(int radius, int n, int max);
bool openImageFromFilename(GBufferedImage& img, string filename);
bool saveImageToFilename(const GBufferedImage &img, string filename);
void getMouseClickLocation(int &row, int &col);

/* 
 * This main simply declares a GWindow and a GBufferedImage for use
 * throughout the program.  
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
 * TODO: rewrite this comment.
 */
void doFauxtoshop(GWindow &gw, GBufferedImage &img) {

    while (true) {
        cout << "Welcome to Fauxtoshop!" << endl;
        
        // Prompts user for a file name until a valid image file is entered, then opens image and returns true.
        // If blank string is entered, returns false.
        if (!getImage(img, gw)) {
            return;
        }

        cout << "Opening image file, may take a minute..." << endl;

        // Resize the GWindow to be the same size as the image
        gw.setCanvasSize(img.getWidth(), img.getHeight());

        // Add image to GWindow 
        gw.add(&img,0,0);
        
        // Prompt user to pick a filter 
        pickFilter(img);

        // Ask user if they would like to save the filtered image
        while (true) {
            string filename = getLine("Enter filename to save image (or blank to skip saving): ");
            if (filename == "" || saveImageToFilename(img, filename.c_str())) {
                   break;
           }
        }
        // Clear the screen
        gw.clear();
	cout << "\n" <<endl;
        
        // Compare to another image 
        // GBufferedImage img2;
        // openImageFromFilename(img2, "beyonce.jpg");
        // img.countDiffPixels(img2);

        // int row, col;
        // getMouseClickLocation(row, col);
        // gw.clear();
    }
}

bool getImage(GBufferedImage& img, GWindow &gw) {

    // Prompt user for image filename and open image. Closes application if blank string is entered.
    while (true) {
        string filename = getLine("Enter name of image file to edit (or blank to quit):");
        // Attempt to open file. Breaks out of the loop if filename is valid. 
        if (openImageFromFilename(img, filename.c_str())) {
            break;
        }
	// If blank, quit program 
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
    try { img.load(filename); }
    catch (...) { return false; }
    return true;
}

/* Asks the user which filter they would like to apply to the image file
 *
 */
void pickFilter(GBufferedImage& img) {
	int n = getInteger("Which image filter would you like to apply?\n\t1 - Scatter\n\t2 - Edge Detection\n\t3 - \"Green screen\" with another image\n\t4 - Compare image with another image\nYour choice: ");
    doFilter(img, n);
}

/* Starts the correct filter function
 *
 */
void doFilter(GBufferedImage& img, int n) {
    Grid<int> original = img.toGrid();
    Grid<int> filtered;
    switch(n) {
        case 1: filtered = doScatter(original);
                break;
        case 2: filtered = doEdgeDetection(original);
                break;
	//case 3: doGreenScreen(img);
	//case 4: doCompare(img);
	default: break;
    }
    img.fromGrid(filtered);
}

/* Applies the scatter filter to the image
 *
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

int setLow(int radius, int n) {
    if ((n - radius) < 0) {
        return 0;
    }
    return n - radius;
}

int setHigh(int radius, int n, int max) {
    if ((n + radius) >= max) {
        return max - 1;
    }
        return n + radius;
}

 /*
 * Returns a Grid<int> with the edge detection filter applied to the Grid<int> argument passed in.
 */
Grid<int> doEdgeDetection(Grid<int>& original) {
    int threshold = getThreshold();
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

    int maxDiff = abs(redA - redB);
    maxDiff = max(abs(greenA - greenB), maxDiff);
    maxDiff = max(abs(blueA - blueB), maxDiff);
    return maxDiff;
}

// Prompts the user for a positive, nonzero integer until it is input. Returns the integer.
int getThreshold() {
    int threshold;
    while (true) {
        threshold = getInteger("Enter threshold for edge detection: ");
        if (threshold >= 0) {
            break;
        }
    }
    return threshold;
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

/* OPTIONAL HELPER FUNCTION
 *
 * This is only here in in case you decide to impelment a Gaussian
 * blur as an OPTIONAL extension (see the suggested extensions part
 * of the spec handout).
 *
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
