void startJdv(const std::string& IMAGE_FILE_NAME, std::string& data_file_name, bool isRedditOption) {

	const size_t 
		TMP_IMAGE_FILE_SIZE = std::filesystem::file_size(IMAGE_FILE_NAME),
		TMP_DATA_FILE_SIZE = std::filesystem::file_size(data_file_name),
		COMBINED_FILE_SIZE = TMP_DATA_FILE_SIZE + TMP_IMAGE_FILE_SIZE;

	constexpr uint_fast32_t
		MAX_FILE_SIZE = 209715200, 
		MAX_FILE_SIZE_REDDIT = 20971520, 
		LARGE_FILE_SIZE = 52428800;

	constexpr uint_fast8_t JPG_MIN_FILE_SIZE = 134;
	
	if (COMBINED_FILE_SIZE > MAX_FILE_SIZE
		 || (isRedditOption && COMBINED_FILE_SIZE > MAX_FILE_SIZE_REDDIT)
		 || JPG_MIN_FILE_SIZE > TMP_IMAGE_FILE_SIZE) {		
		std::cerr << "\nFile Size Error: " 
			<< (JPG_MIN_FILE_SIZE > TMP_IMAGE_FILE_SIZE
        			? "Image is too small to be a valid JPG image"
	        		: "Combined size of image and data file exceeds the maximum limit of "
        	    		+ std::string(isRedditOption 
                			? "20MB"
	                		: "200MB"))
			<< ".\n\n";

    		std::exit(EXIT_FAILURE);
	}
	
	std::ifstream
		image_file_ifs(IMAGE_FILE_NAME, std::ios::binary),
		data_file_ifs(data_file_name, std::ios::binary);

	if (!image_file_ifs || !data_file_ifs) {
		std::cerr << "\nRead File Error: Unable to read " << (!image_file_ifs 
			? "image file" 
			: "data file") 
		<< ".\n\n";

		std::exit(EXIT_FAILURE);
	}

	std::vector<uint_fast8_t>Image_Vec((std::istreambuf_iterator<char>(image_file_ifs)), std::istreambuf_iterator<char>());

	uint_fast32_t image_file_size = static_cast<uint_fast32_t>(Image_Vec.size());

	constexpr uint_fast8_t
		JPG_SIG[3] 	{ 0xFF, 0xD8, 0xFF },
		JPG_END_SIG[2] 	{ 0xFF, 0xD9 },
		DQT_SIG[2]  	{ 0xFF, 0xDB },
		EXIF_SIG[8] 	{ 0x45, 0x78, 0x69, 0x66, 0x00, 0x00, 0x49, 0x49 },
		XPACKET_SIG[11] { 0x78, 0x70, 0x61, 0x63, 0x6B, 0x65, 0x74, 0x20, 0x65, 0x6E, 0x64 },
		ICC_PROFILE_SIG[11] { 0x49, 0x43, 0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45 };

	if (!std::equal(std::begin(JPG_SIG), std::end(JPG_SIG), std::begin(Image_Vec)) || !std::equal(std::begin(JPG_END_SIG), std::end(JPG_END_SIG), std::end(Image_Vec) - 2)) {
        		std::cerr << "\nImage File Error: This is not a valid JPG image.\n\n";
			std::exit(EXIT_FAILURE);
    	}

	const uint_fast32_t ICC_PROFILE_POS = static_cast<uint_fast32_t>(std::search(Image_Vec.begin(), Image_Vec.end(), std::begin(ICC_PROFILE_SIG), std::end(ICC_PROFILE_SIG)) - Image_Vec.begin());
	if (Image_Vec.size() > ICC_PROFILE_POS) {
		Image_Vec.erase(Image_Vec.begin(), Image_Vec.begin() + ICC_PROFILE_POS);
	}

	const uint_fast32_t XPACKET_SIG_POS = static_cast<uint_fast32_t>(std::search(Image_Vec.begin(), Image_Vec.end(), std::begin(XPACKET_SIG), std::end(XPACKET_SIG)) - Image_Vec.begin());
	if (Image_Vec.size() > XPACKET_SIG_POS) {
		Image_Vec.erase(Image_Vec.begin(), Image_Vec.begin() + (XPACKET_SIG_POS + 0x11));
	}

	const uint_fast32_t EXIF_POS = static_cast<uint_fast32_t>(std::search(Image_Vec.begin(), Image_Vec.end(), std::begin(EXIF_SIG), std::end(EXIF_SIG)) - Image_Vec.begin());
	if (Image_Vec.size() > EXIF_POS) {
		const uint_fast16_t EXIF_BLOCK_SIZE = (static_cast<uint_fast16_t>(Image_Vec[EXIF_POS - 2]) << 8) | static_cast<uint_fast16_t>(Image_Vec[EXIF_POS - 1]);
		Image_Vec.erase(Image_Vec.begin(), Image_Vec.begin() + EXIF_BLOCK_SIZE - 2);
	}

	const uint_fast16_t DQT_POS = static_cast<uint_fast16_t>(std::search(Image_Vec.begin(), Image_Vec.end(), std::begin(DQT_SIG), std::end(DQT_SIG)) - Image_Vec.begin());

	Image_Vec.erase(Image_Vec.begin(), Image_Vec.begin() + DQT_POS);

	if (isRedditOption) {
		Image_Vec.insert(Image_Vec.begin(), std::begin(JPG_SIG), std::end(JPG_SIG));
	}

	image_file_size = static_cast<uint_fast32_t>(Image_Vec.size());

	const uint_fast16_t LAST_SLASH_POS = static_cast<uint_fast16_t>(data_file_name.find_last_of("\\/"));

	if (LAST_SLASH_POS <= data_file_name.length()) {
		const std::string_view NO_SLASH_NAME(data_file_name.c_str() + (LAST_SLASH_POS + 1), data_file_name.length() - (LAST_SLASH_POS + 1));
		data_file_name = NO_SLASH_NAME;
	}

	constexpr uint_fast8_t MAX_FILE_NAME_LENGTH = 23;

	const uint_fast16_t DATA_FILE_NAME_LENGTH = static_cast<uint_fast16_t>(data_file_name.length());

	if (DATA_FILE_NAME_LENGTH > TMP_DATA_FILE_SIZE || DATA_FILE_NAME_LENGTH > MAX_FILE_NAME_LENGTH) {
    		std::cerr << "\nData File Error: " 
              		<< (DATA_FILE_NAME_LENGTH > MAX_FILE_NAME_LENGTH 
                	  ? "Length of data filename is too long.\n\nFor compatibility requirements, length of data filename must be under 24 characters"
                  	  : "Size of data file is too small.\n\nFor compatibility requirements, data file size must be greater than the length of the filename")
              		<< ".\n\n";

    	 	std::exit(EXIT_FAILURE);
	}

	std::cout << (TMP_DATA_FILE_SIZE > LARGE_FILE_SIZE ? "\nPlease wait. Larger files will take longer to process.\n" : "");

	std::vector<uint_fast8_t>File_Vec((std::istreambuf_iterator<char>(data_file_ifs)), std::istreambuf_iterator<char>());

	std::reverse(File_Vec.begin(), File_Vec.end());

	uint_fast32_t deflated_file_size = deflateFile(File_Vec);

	constexpr uint_fast8_t PROFILE_HEADER_LENGTH = 18;

	if (image_file_size + deflated_file_size + (deflated_file_size / 65535 * PROFILE_HEADER_LENGTH + PROFILE_HEADER_LENGTH) > (isRedditOption ? MAX_FILE_SIZE_REDDIT : MAX_FILE_SIZE)) {
		std::cerr << "\nImage File Error:\n\nThe combined size of the image file + data file + profile headers, exceeds the maximum limit for this program.\n\n";
		std::exit(EXIT_FAILURE);
	}
		
	std::vector<uint_fast8_t>Profile_Vec = {
			0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xE2, 0xFF, 0xFF,
			0x49, 0x43, 0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x04, 0x30,
			0x00, 0x00, 0x6D, 0x6E, 0x74, 0x72, 0x52, 0x47, 0x42, 0x20, 0x58, 0x59, 0x5A, 0x20, 0x07, 0xE0, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x61, 0x63, 0x73, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xF6, 0xD6, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xD3, 0x2D, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x09, 0x64, 0x65, 0x73, 0x63, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x24, 0x72, 0x58, 0x59, 0x5A, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00,
			0x00, 0x14, 0x67, 0x58, 0x59, 0x5A, 0x00, 0x00, 0x01, 0x28, 0x00, 0x00,	0x00, 0x14, 0x62, 0x58, 0x59, 0x5A, 0x00, 0x00, 0x01, 0x3C, 0x00, 0x00,
			0x00, 0x14, 0x77, 0x74, 0x70, 0x74, 0x00, 0x00, 0x01, 0x50, 0x00, 0x00, 0x00, 0x14, 0x72, 0x54, 0x52, 0x43, 0x00, 0x00, 0x01, 0x64, 0x00, 0x00,
			0x00, 0x28, 0x67, 0x54, 0x52, 0x43, 0x00, 0x00, 0x01, 0x64, 0x00, 0x00, 0x00, 0x28, 0x62, 0x54, 0x52, 0x43, 0x00, 0x00, 0x01, 0x64, 0x00, 0x00,
			0x00, 0x28, 0x63, 0x70, 0x72, 0x74, 0x00, 0x00, 0x01, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x6D, 0x6C, 0x75, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x65, 0x6E, 0x55, 0x53, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x73, 0x00, 0x52, 0x00, 0x47,
			0x00, 0x42, 0x58, 0x59, 0x5A, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0xA2, 0x00, 0x00, 0x38, 0xF5, 0x00, 0x00, 0x03, 0x90, 0x58, 0x59,
			0x5A, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x99, 0x00, 0x00, 0xB7, 0x85, 0x00, 0x00, 0x18, 0xDA, 0x58, 0x59, 0x5A, 0x20, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x24, 0xA0, 0x00, 0x00, 0x0F, 0x84, 0x00, 0x00,	0xB6, 0xCF, 0x58, 0x59, 0x5A, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xF6, 0xD6, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xD3, 0x2D, 0x70, 0x61, 0x72, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02,
			0x66, 0x66, 0x00, 0x00, 0xF2, 0xA7, 0x00, 0x00, 0x0D, 0x59, 0x00, 0x00,	0x13, 0xD0, 0x00, 0x00, 0x0A, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x07, 0x0A, 0x07, 0x07, 0x07, 0x0A, 0x0D, 0x0A,
			0x0A, 0x0A, 0x0A, 0x0D, 0x10, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x10, 0x14,	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
			0x14, 0x14, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0xFF,
			0xDB, 0x00, 0x43, 0x01, 0x05, 0x05, 0x05, 0x08, 0x07, 0x08, 0x0E, 0x07, 0x07, 0x0E, 0x20, 0x16, 0x12, 0x16, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
			0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
			0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0xFF, 0xC2, 0x00, 0x11,
			0x08, 0x04, 0x00, 0x04, 0x00, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xFF, 0xC4, 0x00, 0x1C, 0x00, 0x00, 0x01, 0x05, 0x01,
			0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x04, 0x05, 0x06, 0x03, 0x07, 0x08, 0xFF, 0xC4, 0x00,
			0x1A, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x02, 0x04, 0x05,
			0x06, 0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0xB4, 0x6A, 0x3E, 0xEA, 0x5E, 0x90 };

	Profile_Vec.reserve(deflated_file_size + Profile_Vec.size() + 65535);

	encryptFile(Profile_Vec, File_Vec, data_file_name);

	File_Vec.clear();
	File_Vec.shrink_to_fit();

	insertProfileHeaders(Profile_Vec, File_Vec, deflated_file_size);

	isRedditOption 
		? Image_Vec.insert(Image_Vec.end() - 2, File_Vec.begin() + PROFILE_HEADER_LENGTH, File_Vec.end()) 
		: Image_Vec.insert(Image_Vec.begin(), File_Vec.begin(), File_Vec.end());

	srand((unsigned)time(NULL));  

	const std::string 
		TIME_VALUE = std::to_string(rand()),
		EMBEDDED_IMAGE_FILE_NAME = "jrif_" + TIME_VALUE.substr(0, 5) + ".jpg";

	std::ofstream file_ofs(EMBEDDED_IMAGE_FILE_NAME, std::ios::binary);

	if (!file_ofs) {
		std::cerr << "\nWrite Error: Unable to write to file.\n\n";
		std::exit(EXIT_FAILURE);
	}
	
	uint_fast32_t 
		EMBEDDED_IMAGE_SIZE = static_cast<uint_fast32_t>(Image_Vec.size()),
		REDDIT_SIZE = 20971520;

	file_ofs.write((char*)&Image_Vec[0], EMBEDDED_IMAGE_SIZE);

	std::cout << "\nSaved file-embedded JPG image: " + EMBEDDED_IMAGE_FILE_NAME + '\x20' + std::to_string(EMBEDDED_IMAGE_SIZE) + " Bytes.\n";

	if (isRedditOption && REDDIT_SIZE >= EMBEDDED_IMAGE_SIZE) {
		std::cout << "\n**Important**\n\nDue to your option selection, for compatibility reasons\nyou should only post this file-embedded JPG image on Reddit.\n\n";
	} else {
		std::cout << "\nComplete!\n\nYou can now post your file-embedded JPG image on the relevant supported platforms.\n\n";	
	}	
}
