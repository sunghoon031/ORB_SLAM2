/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/


#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>

#include<ros/ros.h>
#include <cv_bridge/cv_bridge.h>

#include<opencv2/core/core.hpp>

#include"../../../include/System.h"

////////////////////////////////
/// Seong addition starts ... //
////////////////////////////////

#include "geometry_msgs/TransformStamped.h"
#include "tf/transform_datatypes.h"
#include <tf/transform_broadcaster.h>
#include"../../../include/Map.h"
#include "std_msgs/Float32.h"
////////////////////////////////
/// Seong addition ends...   ///
////////////////////////////////

using namespace std;

class ImageGrabber
{
public:
    ImageGrabber(ORB_SLAM2::System* pSLAM):mpSLAM(pSLAM){}

    void GrabImage(const sensor_msgs::ImageConstPtr& msg);

    ORB_SLAM2::System* mpSLAM;
};


int main(int argc, char **argv)
{
    ros::init(argc, argv, "Mono");
    ros::start();

    if(argc != 3)
    {
        cerr << endl << "Usage: rosrun ORB_SLAM2 Mono path_to_vocabulary path_to_settings" << endl;        
        ros::shutdown();
        return 1;
    }    

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);

    ImageGrabber igb(&SLAM);

    ros::NodeHandle nodeHandler;
    ros::Subscriber sub = nodeHandler.subscribe("/camera/image_raw", 1, &ImageGrabber::GrabImage,&igb);

    ///////////////////////////////
    // Seong Addition starts .. ///
    ///////////////////////////////

    ros::Publisher pub = nodeHandler.advertise<std_msgs::Float32>("/orb/depth",1);


    while (ros::ok())
    {
    	float SceneMedianDepth = igb.mpSLAM->GetSceneMedianDepth();
    	std_msgs::Float32 depth_msg;
		depth_msg.data = SceneMedianDepth;
    	pub.publish(depth_msg);

    	ros::spinOnce();
    }

    ///////////////////////////////
	// Seong Addition ends   .. ///
	///////////////////////////////

    //ros::spin();

    // Stop all threads
    SLAM.Shutdown();

    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");

    ros::shutdown();

    return 0;
}

void ImageGrabber::GrabImage(const sensor_msgs::ImageConstPtr& msg)
{
    // Copy the ros image message to cv::Mat.
    cv_bridge::CvImageConstPtr cv_ptr;
    try
    {
        cv_ptr = cv_bridge::toCvShare(msg);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
    }

    // Seong comment
    // mpSLAM->TrackMonocular(cv_ptr->image,cv_ptr->header.stamp.toSec());

    // Seong modification
    cv::Mat mTcw = mpSLAM->TrackMonocular(cv_ptr->image,cv_ptr->header.stamp.toSec());

    ////////////////////////////////
    /// Seong addition starts ... //
    ////////////////////////////////

    if (mTcw.empty())
    {
    	return;
    }

    cv::Mat mRcw = mTcw.rowRange(0,3).colRange(0,3);
    cv::Mat mRwc = mRcw.t(); // camera rotational pose
    cv::Mat mtcw = mTcw.rowRange(0,3).col(3);
    cv::Mat mOw = -mRcw.t()*mtcw; // camera translational pose

	static tf::TransformBroadcaster br;

	tf::Matrix3x3 R_pose( mRwc.at<float>(0,0), mRwc.at<float>(0,1), mRwc.at<float>(0,2),
						mRwc.at<float>(1,0), mRwc.at<float>(1,1), mRwc.at<float>(1,2),
						mRwc.at<float>(2,0), mRwc.at<float>(2,1), mRwc.at<float>(2,2));
	tf::Vector3 t_pose(mOw.at<float>(0,0), mOw.at<float>(1,0), mOw.at<float>(2,0));
	tf::Transform transform = tf::Transform(R_pose, t_pose);
    br.sendTransform(tf::StampedTransform(transform, ros::Time::now(), "camera_origin", "camera_pose"));

    ////////////////////////////////
    /// Seong addition ends   ... //
    ////////////////////////////////

}
