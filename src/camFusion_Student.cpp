
#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <assert.h>

#include "camFusion.hpp"
#include "dataStructures.h"

using namespace std;


// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1) {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        pt.x = Y.at<double>(0, 0) / Y.at<double>(0, 2); // pixel coordinates
        pt.y = Y.at<double>(1, 0) / Y.at<double>(0, 2);

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2) {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt)) {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check whether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1) {
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}


void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size worldSize, cv::Size imageSize, bool bWait)
{
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0;
        float xwmin=1e8, ywmin=1e8, ywmax=-1e8;
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            // draw individual point
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
        }

        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom),cv::Scalar(0,0,0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left-250, bottom+50), cv::FONT_ITALIC, 2, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax-ywmin);
        putText(topviewImg, str2, cv::Point2f(left-250, bottom+125), cv::FONT_ITALIC, 2, currColor);
    }

    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    // display image
    string windowName = "3D Objects";
    cv::namedWindow(windowName, 1);
    cv::imshow(windowName, topviewImg);

    if(bWait)
    {
        cv::waitKey(0); // wait for key to be pressed
    }
}


// associate a given bounding box with the keypoints it contains
void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev,
                              std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches) {
    // ...
}


// Compute time-to-collision (TTC) based on keypoint correspondences in successive images
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr,
                      std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg) {
    // ...
}

bool sort_lidar(LidarPoint pt1, LidarPoint pt2) {
    return (pt1.x < pt2.x);
}

void computeTTCLidar(std::vector<LidarPoint> &lidarPointsPrev,
                     std::vector<LidarPoint> &lidarPointsCurr, double frameRate, double &TTC) {
    // Compute TTC using just LIDAR pts from two frames
    std::cout << "lidarPointsPrev size " << lidarPointsPrev.size() << std::endl;
    std::cout << "lidarPointsCurr size " << lidarPointsCurr.size() << std::endl;



    sort(lidarPointsPrev.begin(), lidarPointsPrev.end(), sort_lidar);
    sort(lidarPointsCurr.begin(), lidarPointsCurr.end(), sort_lidar);

    // get 10th percentile pt
    LidarPoint prevpt = lidarPointsPrev[((int)lidarPointsPrev.size() * 0.1)];
    LidarPoint currpt = lidarPointsCurr[((int)lidarPointsCurr.size() * 0.1)];

    double prev_x = prevpt.x;
    double curr_x = currpt.x;

    std::cout << "Prev x=" << prev_x << " Current x=" << curr_x << std::endl;
    assert(prev_x > curr_x); // prev point should be farther away than current pt!

    double delta_x = prev_x - curr_x;
    float speed = delta_x / frameRate;
    TTC = curr_x / speed;
}


/*
-> TERMINOLOGY LOOKUP TABLE <-
*----------------------------*
| query = source = prevframe |
| train = ref    = currframe |
*----------------------------*
*/
void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches,
                        DataFrame &prevFrame, DataFrame &currFrame) {
    // currFrame->kptMatches == matches
    // std::cout << "Matches size " << matches.size() << " currFrames size " << currFrame.kptMatches.size() << std::endl;
    // std::cout << "prevFrame size " << prevFrame.kptMatches.size() << std::endl;
    // std::cout << "prevFrame keypt size " << prevFrame.keypoints.size() << std::endl;
    // std::cout << "currFrame keypt size " << currFrame.keypoints.size() << std::endl;

    // std::cout << "prevFrame boundingBoxes size " << prevFrame.boundingBoxes.size() << std::endl;
    // std::cout << "currFrame boundingBoxes size " << currFrame.boundingBoxes.size() << std::endl;

    std::vector<int> _row(currFrame.boundingBoxes.size(), 0);
    std::vector<vector<int>> counter(prevFrame.boundingBoxes.size(), _row);


    for (auto match : matches) {
        int _queryIdx = match.queryIdx;
        int _trainIdx = match.trainIdx;

        cv::KeyPoint prevkeypt = prevFrame.keypoints[_queryIdx];
        cv::KeyPoint curkeypt = currFrame.keypoints[_trainIdx];

        // for each point, figure out what bounding box (if any) contains it
        BoundingBox prev_bbox;
        for (auto bbox : prevFrame.boundingBoxes) {
            if (bbox.roi.contains(prevkeypt.pt)) { // Should I handle multiple bboxes overlapping?
                prev_bbox = bbox;
                break;
            }
        }

        if (!prev_bbox.valid()) {
            break; // point not inside any boxes!
        }

        BoundingBox curr_bbox;
        for (auto bbox : currFrame.boundingBoxes) {
            if (bbox.roi.contains(curkeypt.pt)) {
                curr_bbox = bbox;
                break;
            }
        }

        if (!curr_bbox.valid()) {
            break; // point not inside any boxes!
        }

        // if we've made it this far, it means that the last frame's pt has a
        // bbox, and the current frame's pt has a bbox
        counter[prev_bbox.boxID][curr_bbox.boxID]++;
    }

    for (auto bbox : prevFrame.boundingBoxes) {
        vector<int> frequencies = counter[bbox.boxID];
        vector<int>::iterator max_elem = max_element(frequencies.begin(), frequencies.end());
        int arg_max = std::distance(frequencies.begin(), max_elem);
        if (frequencies[arg_max] > 1) { // ensure multiple keypts link the two boxes
            bbBestMatches.insert(std::make_pair(bbox.boxID, arg_max));
        }
    }
}
