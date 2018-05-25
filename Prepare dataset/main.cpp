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
        fname.insert(fname.find_last_of('.'), "_MASS-" + std::to_string(massId) + "_" + std::to_string(i+1));
        //cv::normalize(img,img,0,255,CV_MINMAX,CV_8U);
        cv::imwrite(outPath+"pos/"+fname, img);
//                cv::imshow("Mass ROI - " + std::to_string(ind), img);
//                cv::waitKey(0);
    }
//        cv::destroyAllWindows();
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
    std::string maskName = filename.substr(0,filename.find_last_of(".")).append(".mask.png");
    // load image, mask and ground truth
    cv::Mat image = cv::imread(inPath+"images/"+filename,CV_LOAD_IMAGE_UNCHANGED);
    cv::Mat mask = cv::imread(inPath+"masks/"+maskName,CV_LOAD_IMAGE_GRAYSCALE);
    cv::Mat truth = cv::imread(inPath+"groundtruth/"+filename,CV_LOAD_IMAGE_GRAYSCALE);

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

    // define multiple windows arroud a mass and crop
    for (int i = 0; i < massRect.size(); i++) {
        // getting coordinates of the mass
        cv::Point rectTl = massRect[i].tl(); // top left corner
        cv::Point rectBr = massRect[i].br(); // bottom right corner
        cv::Point rectTr(rectBr.x,rectTl.y); // top right corner
        cv::Point rectBl(rectTl.x,rectBr.y); // bottom left corner
        cv::Point rectCenter = (rectTl + rectBr) * 0.5; // center

        // building the top-left coordinate of 5 different cropping windows
        int x, y;
        std::vector<cv::Point> pt;

        x = rectCenter.x-(winW/2); if (x<0) { x = 0; }
        y = rectCenter.y-(winW/2); if (y<0) { y = 0; }
        pt.push_back(cv::Point(x,y));

        x = rectTl.x; if (x<0) { x = 0; }
        y = rectTl.y; if (y<0) { y = 0; }
        pt.push_back(cv::Point(x,y));

        x = rectTr.x-winW+1; if (x<0) { x = 0; }
        y = rectTr.y; if (y<0) { y = 0; }
        pt.push_back(cv::Point(x,y));

        x = rectBl.x; if (x<0) { x = 0; }
        y = rectBl.y-winW+1; if (y<0) { y = 0; }
        pt.push_back(cv::Point(x,y));

        x = rectBr.x-winW+1; if (x<0) { x = 0; }
        y = rectBr.y-winW+1; if (y<0) { y = 0; }
        pt.push_back(cv::Point(x,y));

        // add extra for big masses
        if (massRect[i].height > 2*winW) {
            x = rectTl.x; if (x<0) { x = 0; }
            y = int((rectTl.y+rectBr.y)/2) - int(winW/2) ; if (y<0) { y = 0; }
            pt.push_back(cv::Point(x,y));

            x = rectTr.x-winW+1; if (x<0) { x = 0; }
            y = int((rectTl.y+rectBr.y)/2) - int(winW/2) ; if (y<0) { y = 0; }
            pt.push_back(cv::Point(x,y));
        }
        if (massRect[i].width > 2*winW) {
            x = int((rectTl.x+rectTr.x)/2) - int(winW/2) ; if (x<0) { x = 0; }
            y = rectTl.y; if (y<0) { y = 0; }
            pt.push_back(cv::Point(x,y));

            x = int((rectTl.x+rectTr.x)/2) - int(winW/2) ;
            y = rectBl.y-winW+1; if (y<0) { y = 0; }
            pt.push_back(cv::Point(x,y));
        }

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
        for (int j=1; j<pt.size(); j++) {
            int ptX = pt[j].x, ptY = pt[j].y;
            for (int k=0; k<j; k++) {
                if (pt[k].x == ptX && pt[k].y == ptY) {
                    pt.erase(pt.begin()+j);
                    j--;
                    break;
                }
            }
        }
        crop(imageRoi, winW, pt, filename, i+1);
    }
}

void getNegative(std::string filename) {
    std::string fname(filename);
    std::string maskName = filename.substr(0,filename.find_last_of(".")).append(".mask.png");
    // load image, mask and ground truth
    cv::Mat image = cv::imread(inPath+"images/"+filename,CV_LOAD_IMAGE_UNCHANGED);
    cv::Mat mask = cv::imread(inPath+"masks/"+maskName,CV_LOAD_IMAGE_GRAYSCALE);
    cv::Mat truth = cv::imread(inPath+"groundtruth/"+filename,CV_LOAD_IMAGE_GRAYSCALE);

    if (!truth.data || truth.rows == 0) {
        truth = cv::Mat::zeros(image.size(),CV_8U);
    }

    int winW = 454; // cropping window size
    int imWidth = mask.cols;
    int imHeight = mask.rows;

    int id = 1;
    for (int y=0; y<imHeight-winW; y++) {
        for (int x=0; x<imWidth-winW; x++) {
            double min, max;
            cv::Mat winIm;

            // skip if there is zero pixel in mask
            winIm = mask.rowRange(y,y+winW).colRange(x,x+winW);
            cv::minMaxLoc(winIm, &min, &max); if (min == 0.0) { continue; }

            // skip if there is a non zero pixel in truth
            winIm = truth.rowRange(y,y+winW).colRange(x,x+winW);
            cv::minMaxLoc(winIm, &min, &max); if (max != 0.0) { continue; }

            winIm = image.rowRange(y,y+winW).colRange(x,x+winW);

            fname.insert(fname.find_last_of('.'), "_" + std::to_string(id++));
            cv::imwrite(outPath+"neg/"+fname, winIm);

            fname.assign(filename);
            x += winW;

            //            cv::imshow("roi", winIm);
            //            cv::waitKey(0);
        }
        y += winW;
    }
}

int main (int argc, char ** argv) {
    inPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Advanced Image Analysis/Projects/AIA-Mass-Segmentation-20180410T073508Z-002/AIA-Mass-Segmentation/dataset/";
    outPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Distributed Programming and Networking/Project/dataset/";

    // starting with images having mass(es) (positive)
    std::vector<std::string> file = getFilenamesInFolder(inPath+"groundtruth",".tif");

    int totalMass[] = {2, 1, 2, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    for (int i=0; i<file.size(); i++) {
        ind = i+1;
        //getPositive(file[i], totalMass[i]);
    }

    file.clear();

    // processing all images for negative
    file = getFilenamesInFolder(inPath+"images",".tif");
    for (int i=0; i<file.size(); i++) {
//        getNegative(file[i]);
    }


    int rand [] = { 1234,    39,  1202,  2926,  3592,  2643,  3047,  2048,   631,  1594,   780,  1811,  3464,  2248,    87,  2765,  1062,  3709,  1479  , 1334,   355,   507,  1128,  3341,  3290,  2921,  3186,  1526,   621,   694,  3551,  2484,  2740,  3325,  1033,   246,  3520,  1531  ,  912,  4007,  2831,  3650,    70,  1237,  2819,   699,  1406,  4106,  2805,  2512,  1131,  1071,   506,  2647,  1953,  3095,  3371  ,  382,   349,  1572,  1068,  2344,    86,  2827,   743,  2406,  3500,  3299,  1575,  2888,  1990,  1626,  2267,  2317,  3887,  3965  , 1955,  1991,  2799,  3797,   702,  3802,   359,  2251,  3131,  1218,  1841,  2117,  2383,  1574,  1245,  3298,  3267,  2257,  3950  , 2409,   722,  3665,  3963,  1466,  1046,   118,  1820,   884,  1238,    91,  2980,  2948,  1899,  3496,  2379,   109,  2136,  2316  ,  284,  1452,  1023,  2820,  2229,  3026,  2174,   562,  3133,  2373,  2443,  2490,   250,  2929,  4064,  2559,  2569,  1124,   225  , 4110,   115,  2866,   166,   132,   256,  3022,  1295,  1439,  1290,  1780,  1582,   770,  2807,   170,  3212,  1423,  1039,  1000  , 3404,  2208,  1169,  2553,  3588,  3318,     1,  3007,  3961,  2782,   258,  1784,   168,  1335,  3995,  3067,  2705,  2398,  2768  , 2015,  3434,  3694,  3562,  1086,  4072,  3367,  2502,  3912,  3479,  2659,   251,  2641,  1947,   525,  2111,  1847,  1050,   582  , 4109,   797,  1010,  2713,  3365,  1019,   624,  3251,  3667,   564,  2405,   211,  4107,   640,    44,    18,  3287,  3827,  1164  , 2281,  1592,  2152,   806,   651,  2962,  3857,  1076,  1196,  3920,  4025,  1201,  2592,   418,  1726,   162,   885,  1599,  3311  , 1769,  1715,  3675,  1353,  3615,  3867,  2834,  3004,  2311,  2671,  1776,  3494,   515,  3785,  2150,  3135,  2503,  1228,  3757  , 3905,  3690,  1378,  2417,   533,  1074,  2567,   795,    73,   951,  2252,  1779,   893,  1552,   745,  3431,  3774,  2650,  2493  , 3586,  1998,  2934,  1044,  2172,  2200,  3874,  4063,  2115,  1159,   472,  3504,  1512,  3216,   652,   589,   938,  3070,  1856  , 2084,   965,  2378,  3401,  2017,  2513,  3861,  3885,  1802,   202,  3338,  1984,   297,  2173,  2404,  1040,  1733,    65,   655  , 1756,  2600,  3968,  1407,  3181,  3342,   347,  3419,  1099,  1091,    15,  3934,    83,   984,  3324,   862,  1149,  3126,  3606  , 1743,  3332,  1704,   838,   249,  2171,   867,  1350,  3224,  3490,  1035,  3936,  1304,  3779,  4023,   917,  2701,  1924,  3766  , 1891,  3710,  2750,  1393,  4099,  1064,  1644,  3166,  1191,  3524,  3040,  3152,  3533,  3674,  2861,  3706,  1048,  4050,  3653  , 2159,  1517,  1609,  3480,  3093,  2533,  3635,  3682,  2310,  3880,  1687,  3582,  2885,  3225,  2096,   549,  1225,  1823,  1070  , 3740,   865,  4061,  2900,  1493,   685,  4030,  2833,  1509,  2877,   510,  1184,  2032,  2726,  1510,  1857,  2220,  3391,   518  , 3981,  3432,  1968,  2243,  3661,  4001,   415,   661,  3486,  2309,   915,  3804,  2924,  2109,  3381,  1259,  2532,  4006,   922  , 2988,  2296,  1560,   155,  2959,  1981,  1695,   324,  3027,  2755,  1877,  1980,  1513,  2631,  1528,  3370,  2931,  2941,   252  ,  377,  2581,  2421,   176,  2586,   318,  2126,  3796,  2814,   401,  2860,  3568,  1418,  4077,  2960,  2554,  4040,  2162,  1612  , 1263,  3154,  1944,  3888,  2753,  1255,   595,   952,  3003,  1265,  3173,  2002,   568,  3711,   321,   833,   960,  3437,   111  , 2511,   948,   429,  3262,  3929,  2721,  2307,  4035,  1859,   947,   746,  1111,  3423,   299,  3201,  2362,  1460,  3307,  2055  , 1960,  3159,  3600,  2964,   878,  2047,  1122,  2301,  1478,  2303,  1869,  3077,  2286,   113,  2633,  1729,  1112,  2809,  1807  , 3893 };
    file.clear();
    file = getFilenamesInFolder(outPath+"neg",".tif");
    for (int i=0; i<514; i++) {
        cv::Mat image = cv::imread(outPath+"neg/"+file[rand [i]],CV_LOAD_IMAGE_UNCHANGED);
        cv::imwrite(outPath+"neg_min_bckup/"+file[rand [i]], image);
    }

    return 0;
}
