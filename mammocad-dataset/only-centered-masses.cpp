#include <iostream>
#include <math.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/photo/photo.hpp>

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

void crop (const cv::Mat im, int offset, std::vector<cv::Point> pt, std::string filename, int massId) {
    std::string fname;
    for (int i=0; i<pt.size(); i++) {
        fname = filename;
        int row = pt[i].y, col = pt[i].x;
        cv::Mat img = im.rowRange(row,row+offset).colRange(col, col+offset);
        fname.insert(fname.find_last_of('.'), "_MASS-" + std::to_string(massId));
        //cv::normalize(img,img,0,255,CV_MINMAX,CV_8U);
       cv::imwrite(outPath+"pos/"+fname, img);
      // cv::imshow("Mass ROI - " + std::to_string(ind), img); cv::waitKey(0);
    }
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

    // find mass locations
    std::vector<cv::Rect> massRect = getMasses(truthRoi, totalMass);

    // size of cropping window
    int winW = 454;
    cv::Size winsize(winW,winW);

    int imWidth = imageRoi.cols;
    int imHeight = imageRoi.rows;

    // define a window centered on mass, then crop
    for (int i = 0; i < massRect.size(); i++) {
        // getting coordinates of the mass
        cv::Point rectTl = massRect[i].tl(); // top left corner
        cv::Point rectBr = massRect[i].br(); // bottom right corner
        cv::Point rectCenter = (rectTl + rectBr) * 0.5; // center

        // building the top-left coordinate
        int x, y;
        std::vector<cv::Point> pt;

        x = rectCenter.x-(winW/2); if (x<0) { x = 0; }
        y = rectCenter.y-(winW/2); if (y<0) { y = 0; }
        pt.push_back(cv::Point(x,y));

        // redefine the top-left coordinates if the window won't fully fit
        for (int j=0; j<pt.size(); j++) {
            int ptX = pt[j].x, ptY = pt[j].y;
            int offsetX = ptX + winW - imWidth;
            int offsetY = ptY + winW - imHeight;
            if (offsetX > 0) {
                pt[j].x -= offsetX;
            }
            if (offsetY > 0) {
                pt[j].y -= offsetY;
            }
        }

        // remove duplicate windows
//        for (int j=1; j<pt.size(); j++) {
//            int ptX = pt[j].x, ptY = pt[j].y;
//            for (int k=0; k<j; k++) {
//                if (pt[k].x == ptX && pt[k].y == ptY) {
//                    pt.erase(pt.begin()+j);
//                    j--;
//                    break;
//                }
//            }
//        }
        crop(imageRoi, winW, pt, filename, i+1);
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

    int winW = 454; // cropping window size
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
            cv::imwrite(outPath+"neg/"+fname, winIm);

            fname.assign(filename);
            x += winW;

//            cv::imshow("roi", winIm); cv::waitKey(0);
        }
        y += winW;
    }
}

int main (int argc, char ** argv) {
    inPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Distributed Programming and Networking/Project/rawdata/";
    outPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Distributed Programming and Networking/Project/dataset/";

    std::vector<std::string> file;


    /***********************
     * Build positive data *
     ***********************/
    file = getFilenamesInFolder(inPath+"groundtruths",".tif");
    int totalMass[] = {2, 1, 2, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    for (int i=0; i<file.size(); i++) {
        ind = i+1;
        std::string f = std::to_string(ind).append(".tif");
        getPositive(f, totalMass[i]);
    }

    return 0;

    /*************************
     * Methods negative data *
     *************************/

    file.clear();
    file = getFilenamesInFolder(inPath+"images",".tif");
    for (int i=0; i<file.size(); i++) {
        std::string f = std::to_string(i+1).append(".tif");
        getNegative(f);
    }

    return 0;
}
