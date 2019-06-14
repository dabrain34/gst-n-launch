/* GStreamer command line scalable application
 *
 * Copyright (C) 2019 Stéphane Cerveau <scerveau@collabora.com>
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


#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

GST_DEBUG_CATEGORY (scalable_transcoder_debug);
#define GST_CAT_DEFAULT scalable_transcoder_debug

typedef struct _GstScalableTranscoder
{
  GstElement *pipeline;
  GMainLoop *loop;
  GList *branches;
} GstScalableTranscoder;

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GstScalableTranscoder *thiz = (GstScalableTranscoder *) user_data;
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_error (message, &err, &debug);

      GST_ERROR_OBJECT (thiz, "ERROR: from element %s: %s\n", name,
          err->message);
      if (debug != NULL)
        GST_ERROR_OBJECT (thiz, "Additional debug info:%s", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);

      g_main_loop_quit (thiz->loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_warning (message, &err, &debug);

      GST_WARNING_OBJECT (thiz, "ERROR: from element %s: %s\n", name,
          err->message);
      if (debug != NULL)
        GST_WARNING_OBJECT (thiz, "Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);
      break;
    }
    case GST_MESSAGE_EOS:
      GST_DEBUG_OBJECT (thiz, "Got EOS\n");
      g_main_loop_quit (thiz->loop);
      break;
    default:
      break;
  }

  return TRUE;
}

gboolean
add_branch (GstScalableTranscoder * thiz, gchar * src_desc,
    gchar * branch_desc, gchar * sink_desc)
{
  GstElement *src, *transform, *sink;
  GError *err = NULL;
  GstPad *src_pad = NULL;
  GstPad *sink_pad = NULL;

  GST_DEBUG_OBJECT (thiz, "Add branch with src %s transform %s sink %s",
      src_desc, branch_desc, sink_desc);
  /* create source element and add it to the main pipeline */
  /* create transform bin element and add it to the main pipeline */

  if (!src_desc && !sink_desc)
    transform = gst_parse_launch_full (branch_desc, NULL, GST_PARSE_FLAG_NONE,
        &err);
  else
    transform = gst_parse_bin_from_description (branch_desc, TRUE, &err);

  if (!transform) {
    GST_ERROR_OBJECT (thiz, "Unable to instantiate the transform branch %s",
        branch_desc);
    return FALSE;
  }
  gst_bin_add (GST_BIN (thiz->pipeline), transform);

  if (src_desc) {
    src = gst_element_factory_make (src_desc, NULL);
    if (!src) {
      GST_ERROR_OBJECT (thiz, "Unable to create src element %s", src_desc);
      return FALSE;
    }
    /* retrieve the src pad which will be connected to the transform bin */
    src_pad = gst_element_get_static_pad (src, "src");
    if (!src_pad) {
      GST_ERROR_OBJECT (thiz,
          "Unable to retrieve the src pad of src element: %s", src_desc);
      gst_object_unref (src);
      return FALSE;
    }
    gst_bin_add (GST_BIN (thiz->pipeline), src);
    /* retrieve a compatible pad with the src pad */
    sink_pad = gst_element_get_compatible_pad (transform, src_pad, NULL);
    if (!sink_pad) {
      GST_ERROR_OBJECT (thiz, "Unable to retreive a sink pad ");
      return FALSE;
    }
    /* connect src element with transform bin */
    if (GST_PAD_LINK_FAILED (gst_pad_link (src_pad, sink_pad))) {
      GST_ERROR_OBJECT (thiz, "Unable to link src to transform");
      return FALSE;
    }
    gst_object_unref (src_pad);
    gst_object_unref (sink_pad);
  }

  if (sink_desc) {
    /* create sink element and add it to the main pipeline */
    sink = gst_element_factory_make (sink_desc, NULL);
    if (!sink) {
      GST_ERROR_OBJECT (thiz, "Unable to create sink element %s", sink_desc);
      return FALSE;
    }
    /* retrieve the sink pad which will be connected to the transform bin */
    sink_pad = gst_element_get_static_pad (sink, "sink");
    if (!sink_pad) {
      GST_ERROR_OBJECT (thiz,
          "Unable to retrieve the sink pad of sink element %s", sink_desc);
      gst_object_unref (src);
      return FALSE;
    }
    gst_bin_add (GST_BIN (thiz->pipeline), sink);
    /* retrieve a compatible pad with the sink pad */
    src_pad = gst_element_get_compatible_pad (transform, sink_pad, NULL);
    if (!src_pad) {
      GST_ERROR_OBJECT (thiz, "Unable to get a src pad from transform\n");
      return FALSE;
    }
    /* connect sink element with transform bin */
    if (GST_PAD_LINK_FAILED (gst_pad_link (src_pad, sink_pad))) {
      GST_ERROR_OBJECT (thiz, "Unable to link sink to transform");
      return FALSE;
    }

    gst_object_unref (src_pad);
    gst_object_unref (sink_pad);
  }

  return TRUE;
}

int
main (int argc, char **argv)
{
  int res = EXIT_SUCCESS;
  GError *err = NULL;
  GOptionContext *ctx;
  GstScalableTranscoder *thiz = NULL;
  gchar **full_branch_desc_array = NULL;
  gchar **branch;
  GstBus *bus;
  GOptionEntry options[] = {
    {"branch", 'b', 0, G_OPTION_ARG_STRING_ARRAY, &full_branch_desc_array,
      "Add a custom full branch with gst-launch style description", NULL}
    ,
    {NULL}
  };


  ctx = g_option_context_new ("[ADDITIONAL ARGUMENTS]");
  g_option_context_add_main_entries (ctx, options, NULL);
  g_option_context_add_group (ctx, gst_init_get_option_group ());

  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    GST_ERROR_OBJECT (thiz, "Error initializing: %s\n",
        GST_STR_NULL (err->message));
    res = -1;
    goto done;
  }
  g_option_context_free (ctx);

  GST_DEBUG_CATEGORY_INIT (scalable_transcoder_debug, "n-launch", 0,
      "gst-n-launch");

  if (!full_branch_desc_array) {
    g_printerr ("Usage: %s -b branch1 \n", argv[0]);
    goto done;
  }
  thiz = g_new0 (GstScalableTranscoder, 1);
  thiz->pipeline = gst_pipeline_new ("gst-n-launch");
  for (branch = full_branch_desc_array; branch != NULL && *branch != NULL;
      ++branch) {
    if (!add_branch (thiz, NULL, *branch, NULL)) {
      res = -2;
      goto done;
    }
  }

  thiz->loop = g_main_loop_new (NULL, FALSE);
  bus = gst_pipeline_get_bus (GST_PIPELINE (thiz->pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (message_cb), thiz);
  gst_object_unref (GST_OBJECT (bus));

  if (gst_element_set_state (thiz->pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    GST_ERROR_OBJECT (thiz, "Failed to go into PLAYING state");
    res = -3;
    goto done;
  }
  g_print ("PLAYING");
  g_main_loop_run (thiz->loop);
  g_print ("STOP");
  gst_element_set_state (thiz->pipeline, GST_STATE_NULL);

done:
  if (thiz->loop)
    g_main_loop_unref (thiz->loop);
  if (thiz->pipeline)
    gst_object_unref (thiz->pipeline);

  g_list_free_full (thiz->branches, g_free);
  g_strfreev (full_branch_desc_array);
  return res;
}
