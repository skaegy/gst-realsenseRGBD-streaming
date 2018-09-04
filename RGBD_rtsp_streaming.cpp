//
// Created by root on 30/08/18.
//

/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <librealsense2/rs.hpp>
#include <librealsense2/rs_types.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

const int WIDTH = 424;
const int HEIGHT = 240;
const int SIZE = WIDTH * HEIGHT * 3;
const int FRAME = 15;
rs2::align align(RS2_STREAM_COLOR);

typedef struct
{
    gboolean white;
    GstClockTime timestamp;
} MyContext;

/* called when we need to give data to appsrc */
static void
need_data (GstElement * appsrc, guint unused, gpointer user_data)
{
    GstBuffer *buffer;
    guint size = SIZE * 2;
    GstFlowReturn ret;
    rs2::pipeline* pPipe = (rs2::pipeline* ) user_data;

    rs2::frameset rs_d415 = pPipe->wait_for_frames();
    rs2::frameset aligned_frame = align.process(rs_d415);

    rs2::spatial_filter spat_filter;    //
    spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.5);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20);
    rs2::disparity_transform depth_to_disparity(true);
    rs2::disparity_transform disparity_to_depth(false);


    rs2::frame depth = aligned_frame.get_depth_frame();
    depth = depth_to_disparity.process(depth);
    //depth = spat_filter.process(depth);
    depth = disparity_to_depth.process(depth);
    rs2::frame color = aligned_frame.get_color_frame();

    cv::Mat imRGB(cv::Size(WIDTH, HEIGHT), CV_8UC3, (void *) color.get_data(), cv::Mat::AUTO_STEP);
    cv::Mat imDep(cv::Size(WIDTH, HEIGHT), CV_16UC1, (void *) depth.get_data(), cv::Mat::AUTO_STEP);
    cv::Mat imCombine(cv::Size(WIDTH, HEIGHT*2), CV_8UC3);
    /* encode depth (Z_16) to CV_8UC3
     * 0: Z/32
     * 1: Z/32
     * 2: Z \in 0-32
     * */
    double scale_factor = 0.05;
    std::vector<cv::Mat> Depth_channel(3);
    imDep.copyTo(Depth_channel[0]);
    Depth_channel[0].convertTo(Depth_channel[0], CV_16U, 1.0/32.0);
    imDep.copyTo(Depth_channel[1]);
    Depth_channel[1].convertTo(Depth_channel[1], CV_16U, 1.0/32.0);
    imDep.copyTo(Depth_channel[2]);
    Depth_channel[2].convertTo(Depth_channel[2], CV_16U, 1.0/32.0);
    Depth_channel[2].convertTo(Depth_channel[2], CV_16U, 32.0);
    Depth_channel[2] = imDep - Depth_channel[2];

    cv::Mat imD_C3;
    cv::merge(Depth_channel, imD_C3);
    imD_C3.convertTo(imD_C3, CV_8UC3);
    cv::cvtColor(imD_C3, imD_C3, CV_BGR2RGB);

    cv::vconcat(imRGB,imD_C3,imCombine);

    GstMapInfo mapinfo;
    buffer = gst_buffer_new_allocate (NULL, size, NULL);
    gst_buffer_map (buffer, &mapinfo, GST_MAP_WRITE);
    // Connect address of Mat and Buffer !!!!
    memcpy(mapinfo.data, imCombine.data,  gst_buffer_get_size( buffer ) );

    // gst_buffer_new_wrapped_full: have error when direct use imOut
    //buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, (void *)imD_C3.data, size, 0, size, NULL, NULL );

    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void
media_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media,
                 gpointer user_data)
{
    GstElement *element, *appsrc;
    rs2::pipeline* pPipe = (rs2::pipeline* ) user_data;

    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element (media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up (GST_BIN (element), "mysrc");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg (G_OBJECT (appsrc), "format", "time");
    /* configure the caps of the video */
    g_object_set (G_OBJECT (appsrc), "caps",
                  gst_caps_new_simple ("video/x-raw",
                                       "format", G_TYPE_STRING, "RGB",
                                       "width", G_TYPE_INT, WIDTH,
                                       "height", G_TYPE_INT, HEIGHT*2,
                                       "framerate", GST_TYPE_FRACTION, FRAME, 1, NULL), NULL);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect (appsrc, "need-data", (GCallback) need_data, pPipe);
    gst_object_unref (appsrc);
    gst_object_unref (element);
}

int
main (int argc, char *argv[])
{
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    gst_init (&argc, &argv);

    /* Initialize realsense d415*/
    rs2::pipeline rs_pipe;
    rs2::config rs_cfg;
    rs_cfg.enable_stream(RS2_STREAM_DEPTH, WIDTH, HEIGHT, RS2_FORMAT_Z16, FRAME);
    rs_cfg.enable_stream(RS2_STREAM_COLOR, WIDTH, HEIGHT, RS2_FORMAT_RGB8, FRAME);
    rs_pipe.start(rs_cfg);
    gpointer pPipe = (gpointer) &rs_pipe;

    /* create loop*/
    loop = g_main_loop_new (NULL, FALSE);

    /* create a server instance */
    server = gst_rtsp_server_new ();

    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server);

    /* make a media factory for a test stream. The default media factory can use
     * gst-launch syntax to create pipelines.
     * any launch line works as long as it contains elements named pay%d. Each
     * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new ();
    gst_rtsp_media_factory_set_launch (factory,
                                       "( appsrc name=mysrc !  videoconvert ! "
                                       "x264enc " //pass=qual quantizer=20 tune=zerolatency "
                                       "! rtph264pay name=pay0 pt=96 )");

    /* notify when our media is ready, This is called whenever someone asks for
     * the media and a new pipeline with our appsrc is created */

    g_signal_connect (factory, "media-configure", (GCallback) media_configure,
                      pPipe);

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory (mounts, "/test", factory);

    /* don't need the ref to the mounts anymore */
    g_object_unref (mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach (server, NULL);

    /* start serving */
    g_print ("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run (loop);

    return 0;
}