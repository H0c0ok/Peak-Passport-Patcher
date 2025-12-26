#include <iostream>
#include <Windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <cerrno>

namespace fs = std::filesystem;

struct CustomByte {
	uint8_t HexValue;
	bool IsUnkown;
};

const size_t bodySize = 111;               // isLocked method lenght: 0x6F
std::streamoff FinalOffset = 0x0;
const std::vector<std::string> ExpectedFuncPrefix = { "0x02", "0x??", "0xc1", "0x05", "0x00", "0x04", "0x2c", "0x13" };
std::vector<CustomByte> CustomHexValues = {};
const size_t SizeOfMethodToRead = ExpectedFuncPrefix.size();


int main(int argc, char* argv[]) {

	std::string PeakDllFilePath = "";
	if (argc >= 2) {
		PeakDllFilePath = argv[1];
	}
	else {
		std::cout << "Please locate Assembly-CSharp.dll file in <...steamapps\\common\\PEAK\\PEAK_Data\\Managed> folder" << '\n';
		std::cout << "Next time u can just drag'n'drop dll file into this program, it'll patch it automatically" << '\n';
		std::cout << "Paste path to file: ";
		std::getline(std::cin, PeakDllFilePath);

		PeakDllFilePath.erase(
			std::remove(PeakDllFilePath.begin(), PeakDllFilePath.end(), '\"'),
			PeakDllFilePath.end());
	}

	std::cout << "[?] Entered path: " << PeakDllFilePath << " [?]" << '\n';
	Sleep(200);

	if (!fs::exists(PeakDllFilePath)) {
		std::cerr << "[-] Provided path DOES NOT contain a file [-]" << std::endl;
		system("pause");
		return 1;
	}


	for (auto& value : ExpectedFuncPrefix) {
		CustomByte temp = {};
		if ((value != "0x??") && (value != "??")) {
			temp.IsUnkown = false;
			temp.HexValue = static_cast<uint8_t>(std::stoul(value, nullptr, 16));
			CustomHexValues.push_back(temp);
			continue;
		}
		temp.IsUnkown = true;
		temp.HexValue = 0x00;
		CustomHexValues.push_back(temp);
	}

	std::fstream PeakDllFile;

	try {
		PeakDllFile.open(PeakDllFilePath.c_str(), std::ios::binary | std::ios::in | std::ios::out);
	}
	catch (...) {
		std::cout << "[-] Unable to open file, suggest closing all programs that might reading target file [-]" << std::endl;
		return 2;
	}
	
	if (!PeakDllFile.is_open()) {
		std::cerr << "[-] Unable to open file: " << PeakDllFilePath << " with error: " << errno << " [-]" << '\n';
		PeakDllFile.close();
		system("pause");
		return 3;
	}

	std::vector<uint8_t> isLockedNewBody(bodySize, 0x00);
	isLockedNewBody[0] = 0x20;  // ldc.i4
	isLockedNewBody[1] = 0x00;
	isLockedNewBody[2] = 0x00;
	isLockedNewBody[3] = 0x00;
	isLockedNewBody[4] = 0x00;
	isLockedNewBody[5] = 0x2A;  // ret

	std::vector<uint8_t> ReadedBuffer(SizeOfMethodToRead);
	bool IsSignatureMatch = false;
	auto PeakDllFileSize = fs::file_size(PeakDllFilePath);
	std::cout << "[?] Assembly-Sharp.dll size: 0x" << std::hex << PeakDllFileSize << " [?]" << '\n';

	for (auto offset = 0; offset < PeakDllFileSize; offset++) {
		PeakDllFile.seekp(offset);
		PeakDllFile.read(reinterpret_cast<char*>(ReadedBuffer.data()), SizeOfMethodToRead);

		IsSignatureMatch = (ReadedBuffer.size() >= SizeOfMethodToRead);
		for (size_t i = 0; IsSignatureMatch && i < SizeOfMethodToRead; i++) {

			if (ReadedBuffer[i] != CustomHexValues[i].HexValue) {
				if (!CustomHexValues[i].IsUnkown) {
					IsSignatureMatch = false;
					break;
				}
			}
		}
		if (!IsSignatureMatch) {
			continue;
		}
		std::cout << "[+]  Mask: ";
		for (auto& value : ExpectedFuncPrefix) {
			std::cout << value << " ";
		}
		std::cout << "\n[+] Found: ";
		for (uint8_t i = 0; i < SizeOfMethodToRead; i++) {
			if (ReadedBuffer[i] < 10) {
				std::cout << "0x0" << std::hex << (int)ReadedBuffer[i] << " ";
				continue;
			}
			std::cout << "0x" << std::hex << (int)ReadedBuffer[i] << " ";

		}
		std::cout << "[+]\n[+] Found (mr.) proper bytes of isLocked method at offset: 0x" << std::hex << offset << " [+]" << '\n';
		FinalOffset = offset;
		break;
	}
	if (!IsSignatureMatch) {
		PeakDllFile.close();
		std::cerr << "[-] Unable to locate methods bytes [-]" << std::endl;
		system("pause");
		return 4;
	}
	PeakDllFile.seekp(FinalOffset);
	PeakDllFile.write(reinterpret_cast<const char*>(isLockedNewBody.data()), static_cast<std::streamsize>(isLockedNewBody.size()));

	if (!PeakDllFile.good()) {
		PeakDllFile.close();
		std::wcerr << "[-] Error on writing data, code: " << GetLastError() << " [-]" << std::endl;
		system("pause");
		return 4;
	}
	PeakDllFile.close();
	std::cout << "[+] Success in editing isLocked method, now all skins are open [+]" << std::endl;
	system("pause");
	return 0;
}