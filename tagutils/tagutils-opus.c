//=========================================================================
// FILENAME	: tagutils-opus.c
// DESCRIPTION	: Opus metadata reader
//=========================================================================

static int
_get_opusfileinfo(char *filename, struct song_metadata *psong)
{
	OggOpusFile *opusfile;
	const OpusTags *tags;
	char **comment;
	int *commentlen;
	int j, e;


	opusfile = op_open_file (filename, &e);
	if(!opusfile)
	{
		DPRINTF(E_WARN, L_SCANNER,
			"Error opening input file \"%s\": %s\n", filename, opus_strerror(e));
		return -1;
	}

	DPRINTF(E_MAXDEBUG, L_SCANNER, "Processing file \"%s\"...\n", filename);

	psong->song_length = op_pcm_total (opusfile, -1);
	if (psong->song_length < 0)
	{
		DPRINTF(E_WARN, L_SCANNER,
				"Unable to obtain length of %s\n", filename);
		psong->song_length = 0;
	} else
		/* Sample rate is always 48k, so length in ms is just samples/48 */
		psong->song_length /= 48;

	/* Note that this gives only the first link's channel count. */
	psong->channels = op_channel_count (opusfile, -1);

	psong->samplerate = 48000;
	psong->bitrate = op_bitrate (opusfile, -1);

	tags = op_tags (opusfile, -1);

	if (!tags)
	{
		DPRINTF(E_WARN, L_SCANNER, "Unable to obtain tags from %s\n",
				filename);
		return -1;
	}

	comment = tags->user_comments;
	commentlen = tags->comment_lengths; 

	for (j = 0; j < tags->comments; j++)
		vc_scan (psong, *(comment++), *(commentlen++));


	DPRINTF(E_MAXDEBUG, L_SCANNER, "Amount of pictures in %s is %d ...\n", filename, opus_tags_query_count(tags, "METADATA_BLOCK_PICTURE"));
	for (int count = 0; count < opus_tags_query_count(tags, "METADATA_BLOCK_PICTURE"); count++)
	{
		OpusPictureTag image_tag;
		const char *raw_tag;
		int err;

		raw_tag = opus_tags_query(tags, "METADATA_BLOCK_PICTURE", count);
		if (raw_tag)
			err = opus_picture_tag_parse(&image_tag, raw_tag);
		else
		{
			DPRINTF(E_WARN, L_SCANNER, "Tag query error in %s \n", filename);
			continue;
		}
		if (err)
		{
			DPRINTF(E_WARN, L_SCANNER, "Parse picture tag error in %s \n", filename);
			continue;
		}
		else
			DPRINTF(E_MAXDEBUG, L_SCANNER, "Found some image data in %s with type=%d and mime_type %s \n", filename, image_tag.type, image_tag.mime_type);
		
		if ( image_tag.type == 3 && !strcmp(image_tag.mime_type,"image/jpeg") )
		{
			DPRINTF(E_MAXDEBUG, L_SCANNER, "Found front cover image data in JPG format in %s...\n", filename);
			psong->image = malloc(image_tag.data_length);
			memcpy(psong->image, image_tag.data, image_tag.data_length);
			psong->image_size = image_tag.data_length;
			break;
		}
	}
			

	op_free (opusfile);
	return 0;
}
