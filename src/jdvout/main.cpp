//	JPG Data Vehicle (jdvout v1.0.3) Created by Nicholas Cleasby (@CleasbyCode) 10/04/2023
//
//	To compile program (Linux):
// 	$ g++ main.cpp -O2 -lz -s -o jdvout

// 	Run it:
// 	$ ./jdvout

#include "jdvout.h"

int main(int argc, char** argv) {
	if (argc !=2) {
		std::cout << "\nUsage: jdvout <file_embedded_image>\n\t\bjdvout --info\n\n";
	} else if (std::string(argv[1]) == "--info") {
		displayInfo();
	} else {
		const std::string IMAGE_FILE_NAME = std::string(argv[1]);

		const std::regex REG_EXP("(\\.[a-zA-Z_0-9\\.\\\\\\s\\-\\/]+)?[a-zA-Z_0-9\\.\\\\\\s\\-\\/]+?(\\.[a-zA-Z0-9]+)?");

		std::string file_extension = IMAGE_FILE_NAME.length() > 3 ? IMAGE_FILE_NAME.substr(IMAGE_FILE_NAME.length() - 4) : IMAGE_FILE_NAME;

		file_extension = file_extension == "jpeg" || file_extension == "jiff" ? ".jpg" : file_extension;

		if (file_extension != ".jpg" || !regex_match(IMAGE_FILE_NAME, REG_EXP)) {
			std::cerr << (file_extension != ".jpg" 
				? "\nFile Type Error: Invalid file extension found. Expecting only '.jpg'"
				: "\nInvalid Input Error: Characters not supported by this program found within file name arguments") 
			<< ".\n\n";

			std::exit(EXIT_FAILURE);
		}
		startJdv(IMAGE_FILE_NAME);	
	} 
	return 0;
}