// If required, split & store users data file into multiple APP2 profile segments. 
// The first APP2 segment contains the color profile data, followed by the users data file.
// Additional segments start with the 18 byte JPG APP2 profile header, followed by the data file.
void segmentDataFile(std::vector<uint8_t>&Profile_Vec, std::vector<uint8_t>&File_Vec) {
	constexpr uint8_t
		SEGMENT_HEADER_LENGTH = 18,
		JPG_HEADER_LENGTH     = 20,
		APP2_SIG_LENGTH       = 2; // FFE2.

	const uint32_t COLOR_PROFILE_WITH_DATA_FILE_VEC_SIZE = static_cast<uint32_t>(Profile_Vec.size());

	uint32_t segment_data_size = 65519;
	uint8_t value_bit_length = 16;	

	if (segment_data_size + JPG_HEADER_LENGTH + SEGMENT_HEADER_LENGTH >= COLOR_PROFILE_WITH_DATA_FILE_VEC_SIZE) { 
		// Data file is small enough to fit all within the first/main APP2 segment, appended to the color profile data.
		constexpr uint8_t
			SEGMENT_HEADER_SIZE_INDEX = 0x16, // Two byte JPG APP2 profile header segment size field index.
			COLOR_PROFILE_SIZE_INDEX  = 0x28, // Four byte ICC profile size field index.
			COLOR_PROFILE_SIZE_DIFF   = 16;	  // Size difference between SEGMENT_SIZE & COLOR_PROFILE_SIZE.
		
		const uint32_t 
			SEGMENT_SIZE 	   = COLOR_PROFILE_WITH_DATA_FILE_VEC_SIZE - (JPG_HEADER_LENGTH + APP2_SIG_LENGTH),
			COLOR_PROFILE_SIZE = SEGMENT_SIZE - COLOR_PROFILE_SIZE_DIFF;

		valueUpdater(Profile_Vec, SEGMENT_HEADER_SIZE_INDEX, SEGMENT_SIZE, value_bit_length);
		valueUpdater(Profile_Vec, COLOR_PROFILE_SIZE_INDEX, COLOR_PROFILE_SIZE, value_bit_length);
		Profile_Vec[0x24] = Profile_Vec[0x25] = 1; // Change the default segment sequence and segment total values to 1. For X/Twitter compatibility.
		File_Vec = std::move(Profile_Vec);
	} else { 
		// Data file is too large for the first APP2 profile segment. Create additional segments as needed, to store the data file.
		uint32_t 
			segments_required_approx_val = COLOR_PROFILE_WITH_DATA_FILE_VEC_SIZE / segment_data_size,
			segment_remainder_size	     = COLOR_PROFILE_WITH_DATA_FILE_VEC_SIZE % segment_data_size,
			byte_index 		     = 0;
		
		constexpr uint8_t 
			SEGMENT_HEADER[SEGMENT_HEADER_LENGTH] { 0xFF, 0xE2, 0xFF, 0xFF, 0x49, 0x43, 0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x00, 0x00, 0xFF },
			SEGMENTS_TOTAL_VAL_INDEX = 0x8A;  // Index location within color profile data area, to store total value of APP2 profile segments (-1). For jdvout.
		
		// Write total number of APP2 profile segments (minus the first one) within the index position of the first profile segment. For jdvout.
		valueUpdater(Profile_Vec, SEGMENTS_TOTAL_VAL_INDEX, segments_required_approx_val, value_bit_length);

		// Create vector of vectors to store the individual segment vectors.
		std::vector<std::vector<uint8_t>> Segments_Arr_Vec;

		segment_data_size += JPG_HEADER_LENGTH + SEGMENT_HEADER_LENGTH; // Just for the first segment.

		uint8_t segments_sequence_value_index = 0x0F;
		uint16_t segments_sequence_value = 1;

		std::vector<uint8_t> Segment_Vec;
		Segment_Vec.reserve(segment_data_size + segment_remainder_size);

		while (segments_required_approx_val--) {	
			if (byte_index) {
				std::copy(std::begin(SEGMENT_HEADER), std::end(SEGMENT_HEADER), std::back_inserter(Segment_Vec));
			}
			valueUpdater(Segment_Vec, segments_sequence_value_index, segments_sequence_value, value_bit_length);
			while (segment_data_size--) {
				Segment_Vec.emplace_back(Profile_Vec[byte_index++]);
			}
    			Segments_Arr_Vec.emplace_back(Segment_Vec);
			segment_data_size = 65519;
			segments_sequence_value++;
			Segment_Vec.clear();	
		}
		if (segment_remainder_size) {
			constexpr uint8_t SEGMENT_REMAINDER_DIFF = 16;
			uint8_t segment_remainder_size_index = 2;
			std::copy(std::begin(SEGMENT_HEADER), std::end(SEGMENT_HEADER), std::back_inserter(Segment_Vec));			

			valueUpdater(Segment_Vec, segment_remainder_size_index, segment_remainder_size + SEGMENT_REMAINDER_DIFF, value_bit_length);
			valueUpdater(Segment_Vec, segments_sequence_value_index, segments_sequence_value, value_bit_length);
			while (segment_remainder_size--) {
				Segment_Vec.emplace_back(Profile_Vec[byte_index++]);	
			}
			Segments_Arr_Vec.emplace_back(Segment_Vec);
		}

		std::vector<uint8_t>().swap(Profile_Vec);
		File_Vec.reserve(segment_data_size * segments_sequence_value);
		for (auto& vec : Segments_Arr_Vec) {
        		File_Vec.insert(File_Vec.end(), vec.begin(), vec.end());
			std::vector<uint8_t>().swap(vec);
    		}
		std::vector<std::vector<uint8_t>>().swap(Segments_Arr_Vec);
		
		constexpr uint8_t MASTODON_SEGMENTS_LIMIT = 100;  
		constexpr uint32_t MASTODON_IMAGE_UPLOAD_LIMIT = 16777216;
					   
		if (segments_sequence_value > MASTODON_SEGMENTS_LIMIT && MASTODON_IMAGE_UPLOAD_LIMIT > COLOR_PROFILE_WITH_DATA_FILE_VEC_SIZE) {
			std::cout << "\n**Warning**\n\nEmbedded image is not compatible with Mastodon. Image file exceeds platform's segments limit.\n";
		}
	}
	value_bit_length = 32; 

	constexpr uint8_t DEFLATED_DATA_FILE_SIZE_INDEX = 0x90;  
	constexpr uint16_t PROFILE_SIZE = 912; // Includes JPG header, profile/segment header and color profile data.
	
	// Write the compressed file size of the data file, which now includes multiple segments with the 18 byte profile/segment headers,
	// minus profile size, within index position of the profile data section. Value used by jdvout.	
	valueUpdater(File_Vec, DEFLATED_DATA_FILE_SIZE_INDEX, static_cast<uint32_t>(File_Vec.size()) - PROFILE_SIZE, value_bit_length);
}
