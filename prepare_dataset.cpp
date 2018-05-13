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
        cv::imwrite(outPath+"pos/"+fname, img);
//        cv::imshow("Mass ROI - " + std::to_string(ind), img);
//        cv::waitKey(0);
    }
//    cv::destroyAllWindows();
}

void getPositive(std::string filename, int totalMass) {
    // load image and ground truth
    cv::Mat image = cv::imread(inPath+"images/"+filename,CV_LOAD_IMAGE_UNCHANGED);
    cv::Mat truth = cv::imread(inPath+"groundtruth/"+filename,CV_LOAD_IMAGE_GRAYSCALE);

    // find mass locations
    std::vector<cv::Rect> massRect = getMasses(truth, totalMass);

    // size of cropping window
    int winW = 454;
    cv::Size winsize(winW,winW);

    int imWidth = truth.cols;
    int imHeight = truth.rows;

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
        crop(image, winW, pt, filename, i+1);
    }
}

void getNegative(std::string filename) {

}

int main (int argc, char ** argv) {
    inPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Advanced Image Analysis/Projects/AIA-Mass-Segmentation-20180410T073508Z-002/AIA-Mass-Segmentation/dataset/";
    outPath = "C:/Users/hp4540/Documents/MAIA Courses/UNICAS/Distributed Programming and Networking/Project/dataset/";

    // starting with images having mass(es) (positive)
    std::vector<std::string> file = getFilenamesInFolder(inPath+"groundtruth",".tif");

    int totalMass[] = {2, 1, 2, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    for (int i=0; i<file.size(); i++) {
        ind = i+1;
        getPositive(file[i], totalMass[i]);
    }

    // processing all images for negative
    file = getFilenamesInFolder(inPath+"images",".tif");
    for (int i=0; i<file.size(); i++) {
        getNegative(file[i]);
    }

    return 0;
}