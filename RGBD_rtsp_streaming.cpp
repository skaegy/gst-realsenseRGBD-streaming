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
 *
 * from threading import Thread
from time import clock

import cv2
import gi

gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')
from gi.repository import Gst, GstRtspServer, GObject


class SensorFactory(GstRtspServer.RTSPMediaFactory):
    def __init__(self, **properties):
        super(SensorFactory, self).__init__(**properties)
        self.launch_string = 'appsrc ! video/x-raw,width=320,height=240,framerate=30/1 ' \
                             '! videoconvert ! x264enc speed-preset=ultrafast tune=zerolatency ' \
                             '! rtph264pay config-interval=1 name=pay0 pt=96'
        self.pipeline = Gst.parse_launch(self.launch_string)
        self.appsrc = self.pipeline.get_child_by_index(4)

    def do_create_element(self, url):
        return self.pipeline


class GstServer(GstRtspServer.RTSPServer):
    def __init__(self, **properties):
        super(GstServer, self).__init__(**properties)
        self.factory = SensorFactory()
        self.factory.set_shared(True)
        self.get_mount_points().add_factory("/test", self.factory)
        self.attach(None)


GObject.threads_init()
Gst.init(None)

server = GstServer()

loop = GObject.MainLoop()
th = Thread(target=loop.run)
th.start()

print('Thread started')

cap = cv2.VideoCapture(0)

print(cap.isOpened())

frame_number = 0

fps = 30
duration = 1 / fps

timestamp = clock()

while cap.isOpened():
    ret, frame = cap.read()
    if ret:

        print('Writing buffer')

        data = frame.tostring()

        buf = Gst.Buffer.new_allocate(None, len(data), None)
        buf.fill(0, data)
        buf.duration = fps
        timestamp = clock() - timestamp
        buf.pts = buf.dts = int(timestamp)
        buf.offset = frame_number
        frame_number += 1
        retval = server.factory.appsrc.emit('push-buffer', buf)
        print(retval)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

cap.release()
 */

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

const int WIDTH = 640;
const int HEIGHT = 480;
const int SIZE = 640 * 480 * 3;
const int FRAME = 15;


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
    guint size;
    GstFlowReturn ret;
    rs2::pipeline* pPipe = (rs2::pipeline* ) user_data;

    std::cout << 1 << std::endl;

    //buffer = gst_buffer_new_allocate (NULL, SIZE, NULL);

    rs2::frameset rs_d415 = pPipe->wait_for_frames();
    //rs2::frameset aligned_frame = align.process(rs_d415);
    //rs2::frame depth = aligned_frame.get_depth_frame();
    rs2::frame color = rs_d415.get_color_frame();

    buffer = gst_buffer_new_wrapped_full( ( GstMemoryFlags )0, (void *)color.get_data(), SIZE, 0, SIZE, NULL, NULL );
    /*

    gst_buffer_memset (buffer, 0, ctx->white ? 0xff : 0x0, size);

    ctx->white = !ctx->white;


    GST_BUFFER_PTS (buffer) = ctx->timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
    ctx->timestamp += GST_BUFFER_DURATION (buffer);
    */

    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void
media_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media,
                 gpointer user_data)
{
    GstElement *element, *appsrc;
    MyContext *ctx;
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
                                       "height", G_TYPE_INT, HEIGHT,
                                       "framerate", GST_TYPE_FRACTION, 15, 1, NULL), NULL);
    /*
    ctx = g_new0 (MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    g_object_set_data_full (G_OBJECT (media), "my-extra-data", ctx,
                            (GDestroyNotify) g_free);
    */

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
    rs2::align align(RS2_STREAM_COLOR);
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
                                       "( appsrc name=mysrc ! videoconvert ! x264enc pass=qual quantizer=20 tune=zerolatency"
                                       " ! rtph264pay name=pay0 pt=96 )");

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