
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <tuple>
#include <vector>
#include <string>
#include <unistd.h>
#include <3ds.h>
#include <json.hpp>
#include <sys/stat.h>
#define SECOND(x) (x*1000ULL*1000ULL*1000ULL)
PrintConsole topScreen, bottomScreen;

std::vector<std::tuple<u32,u16,u8>> sections = {
	std::make_tuple(0x00050000, 0x2, 0xC ),
	std::make_tuple(0x00050001, 0x2, 0xC ),
	std::make_tuple(0x00050002, 0x38, 0xC ),
	std::make_tuple(0x00050003, 0x20, 0xC ),
	std::make_tuple(0x00050004, 0x20, 0xC ),
	std::make_tuple(0x00050005, 0x20, 0xE ),
	std::make_tuple(0x00050006, 0x2, 0xC ),
	std::make_tuple(0x00050007, 0x4, 0xC ),
	std::make_tuple(0x00050008, 0x10C, 0xC ),
	std::make_tuple(0x00050009, 0x8, 0xC )
};
u64 sectionSize=0;

bool fileExists (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

unsigned long fileSize (const std::string& name) {
  struct stat buffer;   
  if (stat (name.c_str(), &buffer)==0) {
    return buffer.st_size;
  }
  return 0;
}

void fucked() {
	std::cout << "\n\nPress [Start] to exit\n\n";	
	while (1) {
		hidScanInput();
		if (hidKeysDown() & KEY_START) { 
			amExit();
			cfguExit();
			gfxExit();
			exit(0);
		}
	}
}
void wait() {
	//cout << "\n\nPress [Start] to exit\n\n";	
	while (1) {
		hidScanInput();
		if (hidKeysDown() & KEY_START) { 
			return;
		}
	}
}
u64 getSectionSize(u32 sectionStart, u32 sectionEnd) {
	u64 size = 0;
	for (std::vector<std::tuple<u32,u16,u8>>::iterator section=sections.begin();section != sections.end(); ++section) {
		u32 blckId = std::get<0>(*section);
		u32 blckSize = std::get<1>(*section);
		if (blckId >= sectionStart && blckId < sectionEnd) {
			size += blckSize;
		}
	}
	return size;
}
Result getConfigData(u32 size, u32 blckId, u8 flags, void *out) {
	switch(flags) {
		case 2:
			return CFGU_GetConfigInfoBlk2(size, blckId, out);
		
		case 4:
			return CFG_GetConfigInfoBlk4(size,blckId,out);
		case 8:
		case 0xC:
		default:
			return CFG_GetConfigInfoBlk8(size,blckId,out);
	}
}
Result setConfigData(u32 size, u32 blckId, u8 flags, void *in) {
	switch(flags) {
		case 4:
			return CFG_SetConfigInfoBlk4(size,blckId,in);
		case 8:
		case 0xC:
		default:
			return CFG_SetConfigInfoBlk8(size,blckId,in);
	}
}
bool saveCurrent(std::string filename) {
	u8* screenData = (u8*)calloc(1,sectionSize);
	u8* currentData = screenData;
	for (std::vector<std::tuple<u32,u16,u8>>::iterator section=sections.begin();section != sections.end(); ++section) {
		getConfigData(std::get<1>(*section), std::get<0>(*section), std::get<2>(*section), currentData);
		currentData+=std::get<1>(*section);
	}
	std::ofstream f(filename.c_str());
	f.write((char*)screenData,sectionSize);
	f.close();
	free(screenData);
	return (f.good());
}
int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);
	consoleSelect(&bottomScreen);

	std::cout << "Initializing CFG services\n";
	if (R_FAILED(cfguInit())) {
		std::cout << "Failed to initialize CFG services\n";
		fucked();
	}
	std::cout << "Initializing AM services\n";
	if (R_FAILED(amInit())) {
		std::cout << "Failed to initialize APT services\n";
		fucked();
	}
	sectionSize = getSectionSize(0x00050000,0x00060000);
	u8* screenData = (u8*)calloc(1,sectionSize);
	u8* currentData = screenData;
	bool fileExistsAndSameSize = fileSize("sdmc:/screendata.bin") == sectionSize && fileExists("sdmc:/screendata.bin");
	std::cout << "Press Y to save current settings\n";
	if (fileExistsAndSameSize) {
		std::cout << "Press A to load saved settings\n";
	}
	std::cout << "Press [Start] to exit\n";
	while (1) {
		hidScanInput();
		if (hidKeysDown() & KEY_START) { 
			free(screenData);
			amExit();
			cfguExit();
			gfxExit();
			return 0;
		}else if(hidKeysDown() & KEY_Y) {
			saveCurrent("sdmc:/screendata.bin");
			break;
		}else if(hidKeysDown() & KEY_A && fileExistsAndSameSize) {
			bool fail = false;
			std::ifstream f("sdmc:/screendata.bin");
			f.read((char*)screenData,sectionSize);
			if (f.bad()) {
				fail = true;
			}
			f.close();
			if (!fail) {
				saveCurrent("sdmc:/backup_screendata.bin");
				for (std::vector<std::tuple<u32,u16,u8>>::iterator section=sections.begin();section != sections.end(); ++section) {
					if (R_FAILED(setConfigData(std::get<1>(*section), std::get<0>(*section), std::get<2>(*section), currentData))) {
						std::cout << "Failed to write saved data to" << std::hex << std::get<0>(*section) << "\n";
					}
					currentData+=std::get<1>(*section);
				}
			}else{
				std::cout << "Unable to read data from screendata.bin\n";
				break;
			}
			if (R_FAILED(CFG_UpdateConfigSavegame())) {
				std::cout << "Failed to update all settings\n";
			}else{
				std::cout << "\nDone!\n";
			}
			break;
		}
	
	}

	fucked();
	amExit();
	cfguExit();
	gfxExit();

	return 0;
}
