//=============================================================================
// AeroStreamDepth
// Demonstrates how to capture RGB and depth data from the RealSense camera,
// manipulate the data to create a RGB depth image, the put each individual
// frame into the GStreamer pipeline
//
// Unlike other pipelines where the source is a physical camera, the source
// to this pipeline is a appsrc element. This element gets its data
// frame-by-frame from a continuous pull from the R200 camera.
//
// Built on Ubuntu 16.04 and Eclipse Neon.
//
//	SOFTWARE DEPENDENCIES
//	* LibRealSense
//	* GStreamer
//
// Example
//   ./AeroStream 192.168.1.1
//=============================================================================

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <librealsense2/rs.hpp>
#include <librealsense2/rs_types.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <string>
#include <fstream>

const int WIDTH 	= 640;
const int HEIGHT 	= 480;
const int SIZE 		= 640 * 480 * 3;
const int FRAME     = 15;


// camera always returns this for 1 / get_depth_scale()
const uint16_t ONE_METER = 999;


// Holds the RGB data coming from R200
struct rgb
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};


// Function descriptions with the definitions
static void 	cb_need_data( GstAppSrc *appsrc, guint unused_size, gpointer user_data );

//=======================================================================================
// The main entry into the application. DUH!
// arg[] will contain the IP address of the machine running QGroundControl
//=======================================================================================
gint main(gint argc, gchar *argv[])
{
    // App requires a valid IP address to where QGroundControl is running.
    if( argc < 2 )
    {
        printf( "Inform address as first parameter.\n" );
        exit( EXIT_FAILURE );
    }
    // =================== Initialize Realsense ======================//
    rs2::pipeline pipe;
    rs2::config rs_cfg;
    rs2::align align(RS2_STREAM_COLOR);
    rs_cfg.enable_stream(RS2_STREAM_DEPTH, WIDTH, HEIGHT, RS2_FORMAT_Z16, FRAME);
    rs_cfg.enable_stream(RS2_STREAM_COLOR, WIDTH, HEIGHT, RS2_FORMAT_RGB8, FRAME);
    pipe.start(rs_cfg);
    gpointer pPipe = (gpointer) &pipe;

    char 		str_pipeline[ 200 ];	// Holds the pipeline
    GMainLoop 	*loop		= NULL;		// Main app loop keeps app alive
    GstElement	*pipeline	= NULL;		// GStreamers pipeline for data flow
    GstElement	*appsrc		= NULL;		// Used to inject buffers into a pipeline
    GstCaps		*app_caps	= NULL;		// Define the capabilities of the appsrc element
    GError 		*error 		= NULL;		// Holds error message if generated

    GstAppSrcCallbacks cbs;				// Callback functions/signals for appsrc

    // Initialize GStreamer
    gst_init( &argc, &argv );

    // Create the main application loop.
    loop = g_main_loop_new( NULL, FALSE );

    // Builds the following pipeline.
    snprintf( str_pipeline, sizeof( str_pipeline ), "appsrc name=mysource !"
                                                    "queue ! rtpvrawpay ! queue ! udpsink host=127.0.0.1 port=1234"); //UDP (y)

    // Instruct GStreamer to construct the pipeline and get the beginning element appsrc.
    pipeline   	= gst_parse_launch( str_pipeline, &error );
    if( !pipeline )
    {
        g_print( "Parse error: %s\n", error->message );
        return 1;
    }

    appsrc		= gst_bin_get_by_name( GST_BIN( pipeline ), "mysource" );

    // Create a caps (capabilities) struct that gets feed into the appsrc structure.
    app_caps = gst_caps_new_simple( "video/x-raw", "format", G_TYPE_STRING, "RGB",
                                    "width", G_TYPE_INT, WIDTH, "height", G_TYPE_INT, HEIGHT, NULL );

    // This is going to specify the capabilities of the appsrc.
    gst_app_src_set_caps( GST_APP_SRC( appsrc ), app_caps );

    // Don't need it anymore, un ref it so the memory can be removed.
    gst_caps_unref( app_caps );


    // Set a few properties on the appsrc Element
    g_object_set( G_OBJECT( appsrc ), "is-live", TRUE, "format", GST_FORMAT_TIME, NULL );

    // play
    gst_element_set_state( pipeline, GST_STATE_PLAYING );


    // ================== Connect GST signals with RS2 stream ================//
    cbs.need_data = cb_need_data;

    // Apply the callbacks to the appsrc Elemen / Connect the signals.
    // In other words, cb_need_data will constantly being called
    // to pull data from R200. Why? Because it needs data. =)
    gst_app_src_set_callbacks( GST_APP_SRC_CAST( appsrc ), &cbs, pPipe, NULL );


    // Launch the stream to keep the app running rather than falling out and existing
    g_main_loop_run( loop );

    // clean up
    gst_element_set_state( pipeline, GST_STATE_NULL );
    gst_object_unref( GST_OBJECT ( pipeline ) );
    gst_object_unref( GST_OBJECT ( appsrc ) );
    gst_object_unref( GST_OBJECT ( app_caps ) );
    g_main_loop_unref( loop );

    return 0;
}

//=======================================================================================
// Frame by frame, try to create our own depth image by taking the RGB data
// and modifying the red channels intensity value.
//=======================================================================================
static void cb_need_data( GstAppSrc *appsrc, guint unused_size, gpointer user_data )
{

    GstFlowReturn ret;
    rs2::pipeline* pipe = (rs2::pipeline* ) user_data;
    rs2::frameset rs_d415 = pipe->wait_for_frames();
    //auto aligned_frame = align.process(rs_d415);
    rs2::frame depth = rs_d415.get_depth_frame();
    rs2::frame color = rs_d415.get_color_frame();

    std::cout << 1 << std::endl;

    cv::Mat imRGB(cv::Size(WIDTH, HEIGHT), CV_8UC3, (void *) color.get_data(), cv::Mat::AUTO_STEP);
    cv::Mat imD(cv::Size(WIDTH, HEIGHT), CV_16UC1, (void *) depth.get_data(), cv::Mat::AUTO_STEP);
    cv::Mat imGray(cv::Size(WIDTH, HEIGHT), CV_8UC1, cv::Mat::AUTO_STEP);
    cv::cvtColor(imRGB, imGray, CV_RGB2GRAY);

    for (int i = 0; i < HEIGHT; i++){
        for (int j = 0; j < WIDTH; j++){
            imRGB.at<cv::Vec3b>(i,j)[0] = (uchar)imGray.at<uchar>(i,j);
            imRGB.at<cv::Vec3b>(i,j)[1] = (uchar)(imD.at<ushort>(i,j)/1000);
            imRGB.at<cv::Vec3b>(i,j)[2] = (uchar)((unsigned int)imD.at<ushort>(i,j)%1000);
            //imRGB.at<cv::Vec3b>(i,j)[2] = (uchar)(imD.at<ushort>(i,j) - (uchar)(imD.at<ushort>(i,j)/1000)*1000);
        }
    }
    cv::imwrite("1.jpg", imRGB);
    //std::cout << "IM RGB: TYPE-" << imRGB.type() << " SIZE-" << imRGB.rows*imRGB.cols*imRGB.channels() << std::endl;

    // Creating a new buffer to send to the gstreamer pipeline
    GstBuffer *buffer = gst_buffer_new_wrapped_full( ( GstMemoryFlags )0, (gpointer)imRGB.data, SIZE, 0, SIZE, NULL, NULL );

    // Push the buffer back out to GStreamer so it can send it down and out to wifi
    g_signal_emit_by_name( appsrc, "push-buffer", buffer, &ret );

}

