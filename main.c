#include <gst/gst.h>

#include <stdio.h>

#define OPUS_BITRATE 32000

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct CustomData {
	GstElement *pipeline;

	GstElement *source;
	GstElement *convert_audio;
	GstElement *resample_audio;
	GstElement *opusenc_codec;
	GstElement *convert_video;
	// GstElement *av1enc;
	GstElement *x264_codec;
	GstElement *muxer;
	GstElement *finalsink;
} CustomData;


/**
 * @brief Handler function for "pad-added"
 * 
 * @details Some objects don't have all pads created at the moment of initialization.
 * For example, demuxer like uridecodebin reveals its pads after the input was passed
 * and element detected what streams there are in input. Whenever a new pad is detected,
 * the signal "pad-added" is rised. This functions handles this signal by manually linking
 * a new detected pad to the appropriate next element in pipeline. For audio and video the 
 * function links it to convert_audio and convert_video elements. All other pads are ignored.
 * 
 * @param src element, which added new pad
 * @param pad pad, that was added
 * @param data custom data, that is passed by user for handling new pad
 */
static void pad_added_handler(GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: converter input_url output_file\n");
		return 1;
	}

	CustomData data;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	gboolean terminate = FALSE;

	gst_init(&argc, &argv);

	data.source = gst_element_factory_make("uridecodebin", "source");
	data.convert_audio =
		gst_element_factory_make("audioconvert", "convert");
	data.opusenc_codec =
		gst_element_factory_make("opusenc", "opus-encoder");
	data.resample_audio =
		gst_element_factory_make("audioresample", "resample");
	data.x264_codec = gst_element_factory_make("x264enc", "x264-encoder");
	// data.av1enc = gst_element_factory_make("av1enc", "av1-encoder");
	data.convert_video =
		gst_element_factory_make("videoconvert", "videoconv");
	data.muxer = gst_element_factory_make("matroskamux", "muxer");
	data.finalsink = gst_element_factory_make("filesink", "finalsink");

	data.pipeline = gst_pipeline_new("test-pipeline");

	// check initialization
	if (!data.pipeline || !data.source || !data.convert_audio ||
	    !data.resample_audio || !data.convert_video || !data.muxer ||
	    !data.finalsink) {
		g_printerr("Not all elements could be created.\n");
		return -1;
	}

	if (!data.x264_codec) {
		g_printerr("Not all elements could be created\n");
		g_printerr(
			"Make sure that you have GStreamer Ugly Plugins installed\n");
		return -1;
	}

	if (!data.opusenc_codec) {
		g_printerr("Not all elements could be created\n");
		g_printerr(
			"Make sure that you have GStreamer Base Plugins installed\n");
		return -1;
	}

	// add everything
	gst_bin_add_many(GST_BIN(data.pipeline), data.source,
			 data.convert_audio, data.resample_audio,
			 data.opusenc_codec, data.convert_video, data.muxer,
			 data.finalsink, data.x264_codec, NULL);

	// video pipeline
	if (!gst_element_link_many(data.convert_video, data.x264_codec,
				   data.muxer, NULL)) {
		g_printerr("Video elements could not be linked.\n");
		gst_object_unref(data.pipeline);
		return -1;
	}
	// audio pipeline
	if (!gst_element_link_many(data.convert_audio, data.resample_audio,
				   data.opusenc_codec, data.muxer, NULL)) {
		g_printerr("Elements could not be linked.\n");
		gst_object_unref(data.pipeline);
		return -1;
	}
	// muxer pipeline
	if (!gst_element_link(data.muxer, data.finalsink)) {
		g_printerr("Finalsink could not be linked.\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	g_object_set(data.source, "uri", argv[1], NULL);

	g_object_set(data.finalsink, "location", argv[2], NULL);

	g_object_set(data.opusenc_codec, "bitrate", OPUS_BITRATE, NULL);

	/* Connect to the pad-added signal */
	g_signal_connect(data.source, "pad-added",
			 G_CALLBACK(pad_added_handler), &data);

	/* Start playing */
	ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr(
			"Unable to set the pipeline to the playing state.\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	/* Listen to the bus */
	bus = gst_element_get_bus(data.pipeline);
	do {
		msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
						 GST_MESSAGE_STATE_CHANGED |
							 GST_MESSAGE_ERROR |
							 GST_MESSAGE_EOS);

		/* Parse message */
		if (msg != NULL) {
			GError *err;
			gchar *debug_info;

			switch (GST_MESSAGE_TYPE(msg)) {
			case GST_MESSAGE_ERROR:
				gst_message_parse_error(msg, &err, &debug_info);
				g_printerr(
					"Error received from element %s: %s\n",
					GST_OBJECT_NAME(msg->src),
					err->message);
				g_printerr("Debugging information: %s\n",
					   debug_info ? debug_info : "none");
				g_clear_error(&err);
				g_free(debug_info);
				terminate = TRUE;
				break;
			case GST_MESSAGE_EOS:
				g_print("End-Of-Stream reached.\n");
				terminate = TRUE;
				break;
			case GST_MESSAGE_STATE_CHANGED:
				/* We are only interested in state-changed messages from the pipeline */
				if (GST_MESSAGE_SRC(msg) ==
				    GST_OBJECT(data.pipeline)) {
					GstState old_state;
					GstState new_state;
					GstState pending_state;
					gst_message_parse_state_changed(
						msg, &old_state, &new_state,
						&pending_state);
					g_print("Pipeline state changed from %s to %s:\n",
						gst_element_state_get_name(
							old_state),
						gst_element_state_get_name(
							new_state));
				}
				break;
			default:
				/* We should not reach here */
				g_printerr("Unexpected message received.\n");
				break;
			}
			gst_message_unref(msg);
		}
	} while (!terminate);

	/* Free resources */
	gst_object_unref(bus);
	gst_element_set_state(data.pipeline, GST_STATE_NULL);
	gst_object_unref(data.pipeline);
	return 0;
}

/* This function will be called by the pad-added signal */
static void pad_added_handler(GstElement *src, GstPad *new_pad,
			      CustomData *data)
{
	GstPad *videosink_pad =
		gst_element_get_static_pad(data->convert_video, "sink");

	GstPad *sink_pad =
		gst_element_get_static_pad(data->convert_audio, "sink");

	GstPadLinkReturn ret;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad),
		GST_ELEMENT_NAME(src));

	/* Check the new pad's type */
	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);

	if (g_str_has_prefix(new_pad_type, "video/x-raw")) {
		/* If our converter is already linked, we have nothing to do here */
		if (gst_pad_is_linked(videosink_pad)) {
			g_print("We are already linked. Ignoring.\n");
			goto exit;
		}

		ret = gst_pad_link(new_pad, videosink_pad);
		if (GST_PAD_LINK_FAILED(ret)) {
			g_print("Type is '%s' but link failed.\n",
				new_pad_type);
		} else {
			g_print("Link succeeded (type '%s').\n", new_pad_type);
		}
	}

	if (g_str_has_prefix(new_pad_type, "audio/x-raw")) {
		if (gst_pad_is_linked(sink_pad)) {
			g_print("We are already linked. Ignoring.\n");
			goto exit;
		}

		/* Attempt the link */
		ret = gst_pad_link(new_pad, sink_pad);
		if (GST_PAD_LINK_FAILED(ret)) {
			g_print("Type is '%s' but link failed.\n",
				new_pad_type);
		} else {
			g_print("Link succeeded (type '%s').\n", new_pad_type);
		}
	}

exit:
	/* Unreference the new pad's caps, if we got them */
	if (new_pad_caps != NULL) {
		gst_caps_unref(new_pad_caps);
	}

	/* Unreference the sink pad */
	gst_object_unref(sink_pad);
}
