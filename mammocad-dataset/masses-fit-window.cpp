#include <iostream>
#include <math.h>
#include <cstdlib>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/photo/photo.hpp>

int winW = 224;

int ind = 0;
std::string inPath, outPath;

std::vector <std::string> getFilenamesInFolder(std::string folder, std::string ext = ".tif", bool force_gray = false)
{
    std::vector <std::string> files;
    cv::glob(folder, files);
    for(int i=0; i<files.size(); i++) {
        files[i] = files[i].erase(0,folder.length()+1);
        if(files[i].find(ext) == std::string::npos) {
            files.erase(files.begin()+i);
            i--;
        }
    }
    return files;
}

bool sortByArea(const std::vector<cv::Point> &c1, const std::vector<cv::Point> &c2) {
    return cv::contourArea(c1) > cv::contourArea(c2);
}

std::vector<cv::Rect> getMasses(const cv::Mat &truth, size_t totalMass=-1) {
    std::vector<cv::Rect> boundRect;
    cv::Mat im = truth.clone();
    int rows = im.rows;
    int cols = im.cols;

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(im, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

    if (contours.size() != totalMass) {
        std::sort(contours.begin(), contours.end(), sortByArea);
    }

    if (totalMass < 0) totalMass = contours.size();

    int foundContours = contours.size();

    std::vector<std::vector<cv::Point> > contours_poly( contours.size() );
    for (int i = 0; i < totalMass; i++ ) {
        cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true );
        cv::Rect r = cv::boundingRect(cv::Mat(contours_poly[i]));
        boundRect.push_back(r);
    }
    return boundRect;
}

void crop (const cv::Mat im, int offset, cv::Point pt, std::string filename, int massId) {
    std::string fname;
    fname = filename;
    int row = pt.y, col = pt.x;
    cv::Mat cropped = im.rowRange(row,row+offset).colRange(col, col+offset);
    fname.insert(fname.find_last_of('.'), "_MASS-" + std::to_string(massId));

    cv::Mat dst;
    cv::resize(cropped,dst,cv::Size(winW,winW));
    cv::imwrite(outPath+"1/"+fname, dst);

    // cv::imshow("Mass ROI - " + std::to_string(ind), img); cv::waitKey(0);
    //cv::destroyAllWindows();
}

cv::Rect getBreastBoundary(const cv::Mat& image) {
    cv::Mat im = image.clone();
    cv::Rect boundRect;

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(im, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<cv::Point>> contours_poly(1);
    if (contours.size()>0) {
        cv::approxPolyDP(cv::Mat(contours[0]), contours_poly[0], 3, true );
        boundRect = cv::boundingRect(cv::Mat(contours_poly[0]));
    }

//    cv::Mat drawing = image.rowRange(boundRect.tl().y,boundRect.br().y)
//                        .colRange(boundRect.tl().x,boundRect.br().x);

    return boundRect;
}

void getPositive(std::string filename, int totalMass) {
    std::string maskName = filename.substr(0,filename.find_last_of(".")).append(".png");
    // load image, mask and ground truth
    cv::Mat image = cv::imread(inPath+"images/"+filename,CV_LOAD_IMAGE_UNCHANGED);
    cv::Mat mask = cv::imread(inPath+"masks/"+maskName,CV_LOAD_IMAGE_GRAYSCALE);
    cv::Mat truth = cv::imread(inPath+"groundtruths/"+filename,CV_LOAD_IMAGE_GRAYSCALE);


    cv::Rect bound = getBreastBoundary(mask);

    cv::Mat imageRoi = image.rowRange(bound.tl().y,bound.br().y).colRange(bound.tl().x,bound.br().x);
    cv::Mat truthRoi = truth.rowRange(bound.tl().y,bound.br().y).colRange(bound.tl().x,bound.br().x);

    cv::normalize(imageRoi,imageRoi,0,255,CV_MINMAX,CV_8U);
    cv::Mat enhanced;
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(5,5));
    clahe->apply(imageRoi, imageRoi);

    // find mass locations
    std::vector<cv::Rect> massRect = getMasses(truthRoi, totalMass);

    // size of cropping window
    cv::Size winsize(winW,winW);

    int imWidth = imageRoi.cols;
    int imHeight = imageRoi.rows;


    // define a window centered on mass, then crop
    for (int i = 0; i < massRect.size(); i++) {
        int newWinW = winW;

        // getting coordinates of the mass
        cv::Point rectTl = massRect[i].tl(); // top left corner
        cv::Point rectBr = massRect[i].br(); // bottom right corner
        cv::Point rectCenter = (rectTl + rectBr) * 0.5; // center

        // building the top-left coordinate
        int x, y;
        while(true) {
            x = rectCenter.x-(newWinW/2); if (x<0) { x = 0; }
            y = rectCenter.y-(newWinW/2); if (y<0) { y = 0; }

            // redefine the top-left coordinates if the window won't fully fit

            int offsetX = x + newWinW - imWidth;
            int offsetY = y + newWinW - imHeight;
            if (offsetX > 0) {
                x -= offsetX;
            }
            if (offsetY > 0) {
                y -= offsetY;
            }

            // if the mass contour does not fully fit in window, increase window size
            int maxSide = (massRect[i].height>massRect[i].width)?massRect[i].height:massRect[i].width;

            // add some extra padding to the mass contour
            int x_ = rectTl.x-25, y_ = rectTl.y-25;
            // check limits
            if (x_ < 0) {x_ = 0;}
            if (y_ < 0) {y_ = 0;}

            if (x > x_ || y > y_) {
                newWinW += 1;
            } else {
                break;
            }
        }

        // finally, the top-left point of the window
        cv::Point pt(x,y);

//        cv::Mat draw = imageRoi.clone();
//        cv::rectangle(draw,rectTl,rectBr,cv::Scalar(125),4);
//        cv::rectangle(draw,pt, cv::Point(pt.x+newWinW, pt.y+newWinW),cv::Scalar(255),7);
//        std::string t =  "draw "+std::to_string(ixx++);
//        cv::namedWindow(t, cv::WINDOW_NORMAL);
//        cv::resizeWindow(t,draw.cols/5,draw.rows/5);
//        cv::imshow(t, draw); cv::waitKey(0);
//        cv::destroyAllWindows();

       crop(imageRoi, newWinW, pt, filename, i+1);
    }
}

void getNegative(std::string filename) {
    std::string fname(filename);
    std::string maskName = filename.substr(0,filename.find_last_of(".")).append(".png");
    // load image, mask and ground truth
    cv::Mat image = cv::imread(inPath+"images/"+filename,CV_LOAD_IMAGE_UNCHANGED);
    cv::Mat mask = cv::imread(inPath+"masks/"+maskName,CV_LOAD_IMAGE_GRAYSCALE);
    cv::Mat truth = cv::imread(inPath+"groundtruths/"+filename,CV_LOAD_IMAGE_GRAYSCALE);

    cv::Rect bound = getBreastBoundary(mask);

    if (!truth.data || truth.rows == 0) {
        truth = cv::Mat::zeros(image.size(),CV_8U);
    }

    cv::Mat imageRoi = image.rowRange(bound.tl().y,bound.br().y).colRange(bound.tl().x,bound.br().x);
    cv::Mat truthRoi = truth.rowRange(bound.tl().y,bound.br().y).colRange(bound.tl().x,bound.br().x);
    cv::Mat maskRoi = mask.rowRange(bound.tl().y,bound.br().y).colRange(bound.tl().x,bound.br().x);

    cv::normalize(imageRoi,imageRoi,0,255,CV_MINMAX,CV_8U);
    cv::Mat enhanced;
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(5,5));
    clahe->apply(imageRoi, imageRoi);

    int imWidth = maskRoi.cols;
    int imHeight = maskRoi.rows;

    int id = 1;
    for (int y=0; y<imHeight-winW; y++) {
        for (int x=0; x<imWidth-winW; x++) {
            double min, max;
            cv::Mat winIm;

            // skip if there is zero pixel in mask
            winIm = maskRoi.rowRange(y,y+winW).colRange(x,x+winW);
            cv::minMaxLoc(winIm, &min, &max); if (min == 0.0) { continue; }

            // skip if there is a non zero pixel in truth (i.e. avoid mass)
            winIm = truthRoi.rowRange(y,y+winW).colRange(x,x+winW);
            cv::minMaxLoc(winIm, &min, &max); if (max != 0.0) { continue; }

            winIm = imageRoi.rowRange(y,y+winW).colRange(x,x+winW);

            fname.insert(fname.find_last_of('.'), "_" + std::to_string(id++));
            cv::imwrite(outPath+"0/"+fname, winIm);

            fname.assign(filename);
            x += winW;

            //cv::imshow("roi", winIm); cv::waitKey(0);
        }
        y += winW;
    }
}

bool doPos = false;
bool doNeg = false;

int main (int argc, char ** argv) {
    inPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Distributed Programming and Networking/Project/Resources/rawdata/";
    outPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Distributed Programming and Networking/Project/dataset-fit-clahe/";

    std::vector<std::string> file;

    file = getFilenamesInFolder(inPath+"groundtruths",".tif");
    int totalMass[] = {2, 1, 2, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    /***********************
     * Build positive data *
     ***********************/

    if (doPos) {
        for (int i=0; i<file.size(); i++) {
            ind = i+1;
            std::string f = std::to_string(ind).append(".tif");
            getPositive(f, totalMass[i]);
        }
    }

    /*************************
     * Methods negative data *
     *************************/

    file.clear();

    if (doNeg) {
        file = getFilenamesInFolder(inPath+"images",".tif");
        for (int i=0; i<file.size(); i++) {
            std::string f = std::to_string(i+1).append(".tif");
            getNegative(f);
        }
    }

    return 0;
}
